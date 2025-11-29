#pragma once

#include <string>
#include <unordered_map>
#include <pnq/string.h>

namespace pnq
{
    namespace environment_variables
    {
        /// Get the value of a specific environment variable, allowing for the call to fail.
        ///
        /// @param name name of variable
        /// @param value resulting content
        /// @return true if the variable was queried, or false otherwise
        inline bool get(std::string_view name, std::string &value)
        {
            std::vector<wchar_t> buffer;
            const auto wide_var_name{string::encode_as_utf16(name)};
            if (const auto bytes_needed{GetEnvironmentVariableW(wide_var_name.c_str(), nullptr, 0)})
            {
                buffer.resize(bytes_needed + 1);
                if (GetEnvironmentVariableW(wide_var_name.c_str(), buffer.data(), static_cast<DWORD>(buffer.size())))
                {
                    value = string::encode_as_utf8(buffer.data());
                    return true;
                }
            }
            return false;
        }

    } // namespace environment_variables

} // namespace pnq
