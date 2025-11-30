#pragma once

#include <string>
#include <pnq/platform.h>
#include <pnq/binary_file.h>
#include <pnq/string.h>

#ifdef PNQ_PLATFORM_WINDOWS
#include <Windows.h>
#endif

namespace pnq
{
    namespace text_file
    {
        constexpr uint8_t UTF16LE_BOM[] = {0xFF, 0xFE};
        constexpr uint8_t UTF8_BOM[] = {0xEF, 0xBB, 0xBF};

        /// Get the platform-native line ending.
        constexpr const char* line_ending()
        {
#ifdef PNQ_PLATFORM_WINDOWS
            return "\r\n";
#else
            return "\n";
#endif
        }

        /// Normalize line endings to LF (Unix style).
        /// Converts \r\n and standalone \r to \n.
        inline std::string normalize_line_endings(std::string_view text)
        {
            std::string result;
            result.reserve(text.size());

            for (size_t i = 0; i < text.size(); ++i)
            {
                if (text[i] == '\r')
                {
                    result += '\n';
                    // Skip following \n if present (CRLF -> LF)
                    if (i + 1 < text.size() && text[i + 1] == '\n')
                        ++i;
                }
                else
                {
                    result += text[i];
                }
            }
            return result;
        }

        /// Convert LF line endings to platform-native.
        /// On Windows: \n -> \r\n. On other platforms: no-op.
        inline std::string to_platform_line_endings(std::string_view text)
        {
#ifdef PNQ_PLATFORM_WINDOWS
            std::string result;
            result.reserve(text.size() + text.size() / 20); // rough estimate for extra \r

            for (char c : text)
            {
                if (c == '\n')
                    result += "\r\n";
                else
                    result += c;
            }
            return result;
#else
            return std::string(text);
#endif
        }

        /// Read a text file, auto-detecting encoding via BOM.
        /// Converts UTF-16LE to UTF-8 if needed. Files without BOM are assumed UTF-8.
        /// Line endings are normalized to LF (\n).
        /// @param filename path to the file
        /// @param normalize_lines if true (default), normalize line endings to LF
        /// @return file contents as UTF-8 string, or empty on failure
        inline std::string read_auto(std::string_view filename, bool normalize_lines = true)
        {
            bytes data;
            if (!BinaryFile::read(filename, data))
                return {};

            std::string result;
            if (data.size() >= 3 && memcmp(data.data(), UTF8_BOM, 3) == 0)
            {
                // UTF-8 with BOM - skip BOM
                result = std::string(reinterpret_cast<const char *>(data.data() + 3), data.size() - 3);
            }
            else if (data.size() >= 2 && memcmp(data.data(), UTF16LE_BOM, 2) == 0)
            {
                // UTF-16LE - convert to UTF-8
                std::wstring_view wide(reinterpret_cast<const wchar_t *>(data.data() + 2), (data.size() - 2) / sizeof(wchar_t));
                result = string::encode_as_utf8(wide);
            }
            else
            {
                // No BOM - assume UTF-8
                result = std::string(reinterpret_cast<const char *>(data.data()), data.size());
            }

            return normalize_lines ? normalize_line_endings(result) : result;
        }

        /// Create a UTF-8 encoded text file, optionally including a BOM.
        /// @param filename name of the text file to create
        /// @param text UTF-8 text to write (assumes LF line endings)
        /// @param include_bom whether to include UTF-8 BOM (default: true)
        /// @param use_platform_line_endings if true, convert LF to platform-native (default: true)
        /// @return true on success, false on failure
        inline bool write_utf8(std::string_view filename, std::string_view text, bool include_bom = true, bool use_platform_line_endings = true)
        {
            BinaryFile output;
            if (!output.create_for_writing(filename))
                return false;

            if (include_bom)
            {
                output.write({UTF8_BOM, std::size(UTF8_BOM)});
            }

            if (use_platform_line_endings)
            {
                std::string converted = to_platform_line_endings(text);
                return output.write(std::string_view{converted});
            }
            return output.write(text);
        }

#ifdef PNQ_PLATFORM_WINDOWS
        /// Create an ANSI (system codepage) encoded text file.
        /// @param filename name of the text file to create
        /// @param text UTF-8 text to convert and write (assumes LF line endings)
        /// @param use_platform_line_endings if true, convert LF to CRLF (default: true)
        /// @return true on success, false on failure
        inline bool write_ansi(std::string_view filename, std::string_view text, bool use_platform_line_endings = true)
        {
            BinaryFile output;
            if (!output.create_for_writing(filename))
                return false;

            std::string to_write = use_platform_line_endings ? to_platform_line_endings(text) : std::string(text);

            // Convert UTF-8 -> UTF-16 -> CP_ACP
            std::wstring wide = string::encode_as_utf16(to_write);
            std::string ansi = string::encode_to_codepage(wide, CP_ACP);
            return output.write(std::string_view{ansi});
        }

        /// Create a UTF-16LE encoded text file, optionally including a BOM.
        /// @param filename name of the text file to create
        /// @param text UTF-16 text to write
        /// @param include_bom whether to include UTF-16LE BOM (default: true)
        /// @return true on success, false on failure
        inline bool write_utf16(std::string_view filename, std::wstring_view text, bool include_bom = true)
        {
            BinaryFile output;
            if (!output.create_for_writing(filename))
                return false;

            if (include_bom)
            {
                output.write({UTF16LE_BOM, std::size(UTF16LE_BOM)});
            }
            return output.write(memory_view{reinterpret_cast<const BYTE *>(text.data()), text.length() * sizeof(WCHAR)});
        }
#endif // PNQ_PLATFORM_WINDOWS
    } // namespace text_file
} // namespace pnq
