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

            logging::report_windows_error(PNQ_FUNCTION_CONTEXT, error, std::format("GetFileAttributes({}) failed", path));
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
                logging::report_windows_error(PNQ_FUNCTION_CONTEXT, GetLastError(), std::format("DeleteFileW({}) failed", pathname));
                return false;
            }
            return true;
        }
    } // namespace file
} // namespace pnq