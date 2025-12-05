#pragma once

#include <string>
#include <format>
#include <pnq/logging.h>

namespace pnq
{
    /// File-related operations.
    namespace file
    {
        /// Get the file extension including the dot.
        /// @param name filename or path
        /// @return extension (e.g. ".exe") or empty if none
        inline std::string_view get_extension(std::string_view name)
        {
            const auto pos = name.rfind('.');
            if (pos != std::string_view::npos)
            {
                return name.substr(pos);
            }
            return {};
        }

        /// Get the file extension normalized to lowercase.
        /// @param name filename or path
        /// @return lowercase extension (e.g. ".txt") or empty if none
        inline std::string get_extension_normalized(std::string_view name)
        {
            return string::lowercase(get_extension(name));
        }

        /// Check if a file exists.
        /// @param path file path to check
        /// @return true if file exists
        inline bool exists(std::string_view path)
        {
            const auto wide_path = string::encode_as_utf16(path);
            const auto dwAttributes = ::GetFileAttributesW(wide_path.c_str());
            if (dwAttributes != INVALID_FILE_ATTRIBUTES)
                return true;

            const auto error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
                return false;

            PNQ_LOG_WIN_ERROR(error, "GetFileAttributes('{}') failed", path);
            return false;
        }

        /// Delete a file.
        /// Clears read-only attribute before deletion.
        /// @param pathname file to delete
        /// @return true if deleted successfully
        inline bool remove(std::string_view pathname)
        {
            const auto wide_pathname = string::encode_as_utf16(pathname);
            ::SetFileAttributesW(wide_pathname.c_str(), FILE_ATTRIBUTE_NORMAL);
            if (!::DeleteFileW(wide_pathname.c_str()))
            {
                PNQ_LOG_LAST_ERROR("DeleteFileW('{}') failed", pathname);
                return false;
            }
            return true;
        }

        /// Match text against a glob pattern (case-insensitive).
        /// @param pattern Glob pattern with wildcards: * matches any sequence, ? matches any single char
        /// @param text Text to match against the pattern
        /// @return true if text matches pattern
        inline bool match(std::string_view pattern, std::string_view text)
        {
            size_t pi = 0, ti = 0;
            size_t star_pi = std::string_view::npos;
            size_t star_ti = 0;

            while (ti < text.size())
            {
                if (pi < pattern.size() && (pattern[pi] == '?' || ::tolower(pattern[pi]) == ::tolower(text[ti])))
                {
                    ++pi;
                    ++ti;
                }
                else if (pi < pattern.size() && pattern[pi] == '*')
                {
                    star_pi = pi++;
                    star_ti = ti;
                }
                else if (star_pi != std::string_view::npos)
                {
                    pi = star_pi + 1;
                    ti = ++star_ti;
                }
                else
                {
                    return false;
                }
            }

            while (pi < pattern.size() && pattern[pi] == '*')
                ++pi;

            return pi == pattern.size();
        }
    } // namespace file
} // namespace pnq