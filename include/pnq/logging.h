#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

namespace pnq
{
    namespace logging
    {
        inline void report_windows_error(DWORD error_code, const char *context, std::string_view message)
        {
            spdlog::error("[{}] {}: Windows error code {}", context, message, error_code);
        }

        inline std::shared_ptr<spdlog::logger> initialize_logging(std::string_view app_name)
        {
            // MSVC OutputDebugString sink for Visual Studio debugger only
            // NO console sink - console output should be done via console.h in pservc
            auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
            msvc_sink->set_level(spdlog::level::warn);

            std::vector<spdlog::sink_ptr> sinks{msvc_sink};
            auto logger = std::make_shared<spdlog::logger>(std::string(app_name), sinks.begin(), sinks.end());
            logger->set_level(spdlog::level::debug);

            // Explicitly drop the default logger to prevent any console output
            spdlog::drop_all();
            spdlog::set_default_logger(logger);

            return logger;
        }

        inline void reconfigure_logging_for_file(const std::string &logFilePath)
        {
            // Get existing logger
            auto logger = spdlog::default_logger();

            // Force rotation on startup to create new log file
            std::filesystem::path log_path(logFilePath);
            if (std::filesystem::exists(log_path) && std::filesystem::file_size(log_path) > 0)
            {
                std::filesystem::path log_dir = log_path.parent_path();
                std::string log_stem = log_path.stem().string();
                std::string log_ext = log_path.extension().string();

                // Delete oldest log if it exists
                std::filesystem::path oldest = log_dir / (log_stem + ".10" + log_ext);
                if (std::filesystem::exists(oldest))
                {
                    std::filesystem::remove(oldest);
                }

                // Shift all backup logs up (9 -> 10, 8 -> 9, etc.)
                for (int i = 9; i >= 1; --i)
                {
                    std::filesystem::path old_path = log_dir / std::format("{}.{}{}", log_stem, i, log_ext);
                    std::filesystem::path new_path = log_dir / std::format("{}.{}{}", log_stem, i + 1, log_ext);
                    if (std::filesystem::exists(old_path))
                    {
                        std::filesystem::rename(old_path, new_path);
                    }
                }

                // Move current log to .1
                std::filesystem::rename(log_path, log_dir / (log_stem + ".1" + log_ext));
            }

            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFilePath, 1024 * 1024 * 10, 10);
            file_sink->set_level(spdlog::level::debug);

            // Add file sink to existing logger
            logger->sinks().push_back(file_sink);
        }
    } // namespace logging
} // namespace pnq
