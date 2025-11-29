#pragma once

#include <spdlog/spdlog.h>

namespace pnq
{
    namespace logging
    {
        inline void report_windows_error(DWORD error_code, const char* context, std::string_view message)
        {
            spdlog::error("[{}] {}: Windows error code {}", context, message, error_code);
        }
    }
}
