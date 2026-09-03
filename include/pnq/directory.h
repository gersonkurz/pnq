#pragma once

#include <string>
#include <pnq/pnq.h>

namespace pnq
{
    /// Directory-related operations.
    namespace directory
    {
        /// Check if a directory exists.
        ///
        /// A false answer can mean "not there", "there but not a directory", or "could not
        /// look". Callers that need to tell those apart read GetLastError(), which this function
        /// always sets: ERROR_SUCCESS when the directory exists, ERROR_FILE_NOT_FOUND or
        /// ERROR_PATH_NOT_FOUND when nothing is there, ERROR_DIRECTORY when the path names a
        /// file, and the underlying failure (typically ERROR_ACCESS_DENIED) when the query could
        /// not be answered.
        ///
        /// @param directory path to check
        /// @return true if path exists and is a directory
        inline bool exists(std::string_view directory)
        {
            const auto dwAttrib = ::GetFileAttributesW(string::encode_as_utf16(directory).c_str());
            if (dwAttrib == INVALID_FILE_ATTRIBUTES)
            {
                const auto error = ::GetLastError();
                if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
                    PNQ_LOG_WIN_ERROR(error, "GetFileAttributes('{}') failed", directory);

                ::SetLastError(error); // logging clobbers it
                return false;
            }

            if ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                ::SetLastError(ERROR_DIRECTORY);
                return false;
            }

            ::SetLastError(ERROR_SUCCESS);
            return true;
        }

        /// Get the Windows system directory (e.g. C:\Windows\System32).
        inline std::string system()
        {
            wchar_t buffer[MAX_PATH];
            if (::GetSystemDirectoryW(buffer, std::size(buffer)))
            {
                return string::encode_as_utf8(buffer);
            }
            PNQ_LOG_LAST_ERROR("GetSystemDirectoryW failed");
            return ".";
        }

        /// Get the Windows directory (e.g. C:\Windows).
        inline std::string windows()
        {
            wchar_t buffer[MAX_PATH];
            if (::GetWindowsDirectoryW(buffer, std::size(buffer)))
            {
                return string::encode_as_utf8(buffer);
            }
            PNQ_LOG_LAST_ERROR("GetWindowsDirectoryW failed");
            return ".";
        }

        /// Get the current working directory.
        inline std::string current()
        {
            wchar_t buffer[MAX_PATH];
            if (::GetCurrentDirectoryW(std::size(buffer), buffer))
            {
                return string::encode_as_utf8(buffer);
            }
            PNQ_LOG_LAST_ERROR("GetCurrentDirectoryW failed");
            return ".";
        }

        /// Get the directory containing the current executable.
        inline std::string application()
        {
            wchar_t buffer[MAX_PATH];
            if (!GetModuleFileNameW(nullptr, buffer, std::size(buffer)))
            {
                PNQ_LOG_LAST_ERROR("GetModuleFileNameW failed");
                return ".";
            }
            if (const auto last_sep = wcsrchr(buffer, L'\\'))
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
