#pragma once

#include <string>
#include <vector>

#include <pnq/string.h>

namespace pnq
{
    namespace environment_variables
    {
        /// Get the value of an environment variable.
        /// @param name variable name
        /// @param value receives the value if found
        /// @return true if variable exists, false otherwise
        inline bool get(std::string_view name, std::string &value)
        {
            const auto wide_name = string::encode_as_utf16(name);
            const auto bytes_needed = GetEnvironmentVariableW(wide_name.c_str(), nullptr, 0);
            if (bytes_needed == 0)
                return false;

            std::vector<wchar_t> buffer(bytes_needed);
            if (GetEnvironmentVariableW(wide_name.c_str(), buffer.data(), static_cast<DWORD>(buffer.size())))
            {
                value = string::encode_as_utf8(buffer.data());
                return true;
            }
            return false;
        }

    } // namespace environment_variables
} // namespace pnq
