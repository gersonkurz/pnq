#pragma once

#include <filesystem>
#include <format>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <pnq/windows_errors.h>

namespace pnq
{
    namespace logging
    {
        /// Log a Windows error with context.
        /// @param context function or location identifier (use PNQ_FUNCTION_CONTEXT)
        /// @param error_code Windows error code (GetLastError() or HRESULT)
        /// @param message description of what failed
        inline void report_windows_error(const char *context, DWORD error_code, std::string_view message)
        {
            spdlog::error("[{}] {}: {}", context, message, windows::hresult_as_string(static_cast<HRESULT>(error_code)));
        }

        /// Initialize logging with MSVC debug output sink.
        /// Sets up spdlog with OutputDebugString for Visual Studio debugger.
        /// @param app_name application name for the logger
        /// @param enable_console if true, also log to stdout with colors
        /// @return shared pointer to the configured logger
        inline std::shared_ptr<spdlog::logger> initialize_logging(std::string_view app_name, bool enable_console = false)
        {
            std::vector<spdlog::sink_ptr> sinks;

            auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
            msvc_sink->set_level(spdlog::level::warn);
            sinks.push_back(msvc_sink);

            if (enable_console)
            {
                auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                console_sink->set_level(spdlog::level::info);
                sinks.push_back(console_sink);
            }

            auto logger = std::make_shared<spdlog::logger>(std::string(app_name), sinks.begin(), sinks.end());
            logger->set_level(spdlog::level::debug);

            spdlog::drop_all();
            spdlog::set_default_logger(logger);

            return logger;
        }

        /// Add console (stdout) sink to existing logger.
        /// @param level minimum log level for console output
        inline void enable_console_logging(spdlog::level::level_enum level = spdlog::level::info)
        {
            auto logger = spdlog::default_logger();
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(level);
            logger->sinks().push_back(console_sink);
        }

        /// Add rotating file sink to existing logger.
        /// Forces rotation on startup so each run gets a fresh log file.
        /// Keeps up to 10 backup files of 10MB each.
        /// @param logFilePath path to the log file
        inline void reconfigure_logging_for_file(const std::string &logFilePath)
        {
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

            logger->sinks().push_back(file_sink);
        }
    } // namespace logging
} // namespace pnq
