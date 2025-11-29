#pragma once

#include <string>
#include <format>
#include <pnq/logging.h>

namespace pnq
{
    /// <summary>
    /// Groups common file-related operations
    /// </summary>
    namespace file
    {
        /// <summary>
        /// Given a filename, return the extension (e.g. '.exe' for 'notepad.exe')
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        std::string_view get_extension(std::string_view name)
        {
            const auto pos{name.rfind('.')};
            if (pos != std::string_view::npos)
            {
                return name.substr(pos);
            }
            return "";
        }

        /// <summary>
        /// Checks if file exists
        /// </summary>
        /// <param name="path">filename to check</param>
        /// <returns>true if file exists, or false otherwise</returns>
        bool exists(std::string_view path)
        {
            const auto wide_path{string::encode_as_utf16(path)};
            const auto dwAttributes{::GetFileAttributes(wide_path.data())};
            if (dwAttributes != INVALID_FILE_ATTRIBUTES)
                return true; // file exists, attributes can be read

            const auto hResult{static_cast<HRESULT>(GetLastError())};
            if (hResult == ERROR_FILE_NOT_FOUND)
                return false;

            if (hResult == ERROR_PATH_NOT_FOUND)
                return false;

            logging::report_windows_error(hResult, PNQ_FUNCTION_CONTEXT, std::format("GetFileAttributes({}) failed", path));
            return false;
        }

        /// <summary>
        /// Delete this file
        /// </summary>
        /// <param name="pathname"></param>
        /// <returns></returns>
        bool remove(std::string_view pathname)
        {
            const auto wide_pathname{string::encode_as_utf16(pathname)};
            ::SetFileAttributesW(wide_pathname.c_str(), FILE_ATTRIBUTE_NORMAL);
            if (!::DeleteFileW(wide_pathname.data()))
            {
                logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, std::format("DeleteFileW({}) failed", pathname));
                return false;
            }
            return true;
        }
    } // namespace file
} // namespace pnq