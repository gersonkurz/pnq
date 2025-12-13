#pragma once

#include <algorithm>
#include <format>
#include <locale>
#include <string>

#include <pnq/string.h>
#include <pnq/string_writer.h>

namespace pnq
{
    /// Console output with UTF-8 support and color codes.
    namespace console
    {
        // Color escape codes - use ESC + attribute byte
        #define CONSOLE_FOREGROUND_BRIGHT_BLACK "\x1b\x00"
        #define CONSOLE_FOREGROUND_BRIGHT_BLUE "\x1b\x09"
        #define CONSOLE_FOREGROUND_BRIGHT_CYAN "\x1b\x0b"
        #define CONSOLE_FOREGROUND_BLUE "\x1b\x01"
        #define CONSOLE_FOREGROUND_CYAN "\x1b\x03"
        #define CONSOLE_FOREGROUND_GRAY "\x1b\x07"
        #define CONSOLE_FOREGROUND_GREEN "\x1b\x02"
        #define CONSOLE_FOREGROUND_MAGENTA "\x1b\x05"
        #define CONSOLE_FOREGROUND_RED "\x1b\x04"
        #define CONSOLE_FOREGROUND_YELLOW "\x1b\x06"
        #define CONSOLE_FOREGROUND_BRIGHT_GRAY "\x1b\x08"
        #define CONSOLE_FOREGROUND_BRIGHT_GREEN "\x1b\x0a"
        #define CONSOLE_FOREGROUND_BRIGHT_INTENSITY "\x1b\x08"
        #define CONSOLE_FOREGROUND_BRIGHT_MAGENTA "\x1b\x0d"
        #define CONSOLE_FOREGROUND_BRIGHT_RED "\x1b\x0c"
        #define CONSOLE_FOREGROUND_BRIGHT_WHITE "\x1b\x0f"
        #define CONSOLE_FOREGROUND_BRIGHT_YELLOW "\x1b\x0e"
        #define CONSOLE_STANDARD "\x1b\xFF"

        /// Internal console state.
        struct console_context
        {
            HANDLE hConsoleOutput;
            HANDLE hConsoleInput;
            bool has_ensured_process_has_console;
            bool has_tried_and_failed_to_get_console;
            WORD wOldColorAttrs;
            bool has_retrieved_old_color_attrs;
            bool write_output_has_failed_once;
        };

        /// Get the global console context singleton.
        inline console_context &get_context()
        {
            static console_context the_console_context{
                INVALID_HANDLE_VALUE,
                INVALID_HANDLE_VALUE,
                false, false, 0, false, false
            };
            return the_console_context;
        }

        /// Encode wide string to console output codepage.
        inline std::string encode_as_output_bytes(std::wstring_view text)
        {
            if (text.empty())
                return {};

            const UINT output_cp = ::GetConsoleOutputCP();
            int required_size = WideCharToMultiByte(output_cp, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
            if (required_size <= 0)
                return {};

            if (required_size < 1024)
            {
                char buffer[1024];
                int rc = WideCharToMultiByte(output_cp, 0, text.data(), static_cast<int>(text.size()), buffer, required_size, nullptr, nullptr);
                if (rc > 0)
                    return std::string(buffer, rc);
            }

            std::string result(required_size, '\0');
            int rc = WideCharToMultiByte(output_cp, 0, text.data(), static_cast<int>(text.size()), result.data(), required_size, nullptr, nullptr);
            if (rc > 0)
                return result;

            return {};
        }

#ifdef _CONSOLE
        /// Ensure process has a console (no-op for console apps).
        inline bool ensure_process_has_console()
        {
            return true;
        }
#else
        /// Ensure process has a console, attaching to parent if needed.
        inline bool ensure_process_has_console()
        {
            auto &cc = get_context();

            if (cc.has_tried_and_failed_to_get_console)
                return false;

            if (cc.has_ensured_process_has_console || GetConsoleWindow())
                return true;

            if (!AttachConsole(ATTACH_PARENT_PROCESS))
            {
                DWORD error = GetLastError();
                if (error == ERROR_ACCESS_DENIED)
                {
                    // The process is already attached to a console
                    cc.has_ensured_process_has_console = true;
                    return true;
                }
                cc.has_tried_and_failed_to_get_console = true;
                return false;
            }
            cc.has_ensured_process_has_console = true;
            return true;
        }
#endif

        /// Ensure we have a valid output handle.
        inline bool ensure_output_handle()
        {
            auto &cc = get_context();

            if (cc.hConsoleOutput != INVALID_HANDLE_VALUE)
                return true;

            ensure_process_has_console();

            cc.hConsoleOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
            if (INVALID_HANDLE_VALUE == cc.hConsoleOutput)
            {
                return false;
            }
            // Set locale so std::format understands {:L} properly
            std::locale::global(std::locale(""));
            return true;
        }

        /// Write UTF-16 string to console.
        inline bool write(std::wstring_view utf16_encoded_string)
        {
            auto &cc = get_context();

            if (utf16_encoded_string.empty())
                return true;

            if (cc.write_output_has_failed_once || !ensure_output_handle())
                return false;

            DWORD chars_written = 0;
            const auto output_bytes = encode_as_output_bytes(utf16_encoded_string);

            if (!::WriteFile(cc.hConsoleOutput, output_bytes.data(), static_cast<DWORD>(output_bytes.size()), &chars_written, nullptr))
            {
                cc.write_output_has_failed_once = true;
                return false;
            }
            return true;
        }

        /// Internal: write UTF-8 string without color processing.
        inline bool do_write_output_as_unicode(std::string_view utf8_encoded_string)
        {
            auto &cc = get_context();
            const auto utf16_encoded_string = string::encode_as_utf16(utf8_encoded_string);
            const auto output_bytes = encode_as_output_bytes(utf16_encoded_string);

            DWORD chars_written = 0;
            if (!::WriteFile(cc.hConsoleOutput, output_bytes.data(), static_cast<DWORD>(output_bytes.size()), &chars_written, nullptr))
            {
                cc.write_output_has_failed_once = true;
                return false;
            }
            return true;
        }

        /// Write UTF-8 string to console with color code processing.
        /// Color codes are ESC (0x1b) followed by attribute byte.
        inline bool write(std::string_view utf8_encoded_string)
        {
            auto &cc = get_context();

            if (utf8_encoded_string.empty())
                return true;

            if (cc.write_output_has_failed_once || !ensure_output_handle())
                return false;

            const char *p = utf8_encoded_string.data();
            const char *end = p + utf8_encoded_string.size();

            while (p < end)
            {
                const char *q = std::find(p, end, '\x1b');
                if (q == end)
                {
                    return do_write_output_as_unicode(std::string_view(p, end - p));
                }

                if (!cc.has_retrieved_old_color_attrs)
                {
                    CONSOLE_SCREEN_BUFFER_INFO csbiInfo{};
                    GetConsoleScreenBufferInfo(cc.hConsoleOutput, &csbiInfo);
                    cc.wOldColorAttrs = csbiInfo.wAttributes;
                    cc.has_retrieved_old_color_attrs = true;
                }

                if (q > p)
                {
                    do_write_output_as_unicode(std::string_view(p, q - p));
                }

                // Need at least 2 bytes for escape sequence
                if (q + 1 >= end)
                    break;

                if (q[1] == '\xFF')
                {
                    SetConsoleTextAttribute(cc.hConsoleOutput, cc.wOldColorAttrs);
                }
                else
                {
                    SetConsoleTextAttribute(cc.hConsoleOutput, static_cast<WORD>(static_cast<unsigned char>(q[1])));
                }

                p = q + 2;
            }
            return true;
        }

        /// Write text followed by newline.
        template <typename T>
        inline bool write_line(T text)
        {
            string::Writer temp;
            temp.append(text);
            temp.append(string::newline);
            return write(temp.as_string());
        }

        /// Format and write to console.
        template <typename... Args>
        inline bool format(std::string_view text, Args &&...args)
        {
            return write(std::vformat(text, std::make_format_args(args...)));
        }

        /// Format and write a line to console.
        template <typename... Args>
        inline bool format_line(std::string_view text, Args &&...args)
        {
            return write_line(std::vformat(text, std::make_format_args(args...)));
        }
    } // namespace console
} // namespace pnq
