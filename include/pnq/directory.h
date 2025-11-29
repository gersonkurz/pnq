#pragma once

#include <string>
#include <pnq/directory.h>
#include <pnq/logging.h>

namespace pnq
{
    namespace directory
    {
        inline bool exists(std::string_view directory)
        {
            const auto dwAttrib{GetFileAttributesW(string::encode_as_utf16(directory).c_str())};

            return (dwAttrib != INVALID_FILE_ATTRIBUTES) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
        }

        inline std::string system()
        {
            wchar_t buffer[MAX_PATH];

            if (::GetSystemDirectoryW(buffer, std::size(buffer)))
            {
                return string::encode_as_utf8(buffer);
            }
            logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, "GetSystemDirectoryW() failed");
            return ".";
        }

        inline std::string windows()
        {
            wchar_t buffer[MAX_PATH];

            if (::GetWindowsDirectoryW(buffer, std::size(buffer)))
            {
                return string::encode_as_utf8(buffer);
            }
            logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, "GetWindowsDirectoryW() failed");
            return ".";
        }

        inline std::string current()
        {
            wchar_t buffer[MAX_PATH];

            if (::GetCurrentDirectoryW(std::size(buffer), buffer))
            {
                return string::encode_as_utf8(buffer);
            }
            logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, "GetCurrentDirectoryW() failed");
            return ".";
        }

        inline std::string application()
        {
            wchar_t buffer[MAX_PATH];
            if (!GetModuleFileNameW(nullptr, buffer, std::size(buffer)))
            {
                logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, "GetModuleFileNameW() failed");
                return ".";
            }
            if (const auto last_sep{wcsrchr(buffer, L'\\')})
            {
                *last_sep = 0;
            }
            else
            {
                buffer[0] = 0;
            }
            return string::encode_as_utf8(buffer);
        }
    } // namespace directory
} // namespace pnq
