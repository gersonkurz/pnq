#pragma once

#include <filesystem>
#include <format>
#include <string>
#include <vector>

#include <pnq/log.h>
#include <pnq/windows_errors.h>

#ifdef PNQ_USE_QUILL

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/RotatingFileSink.h>

namespace pnq::logging
{
    /// Log level abstraction
    enum class level
    {
        trace,
        debug,
        info,
        warn,
        error,
        critical
    };

    namespace detail
    {
        inline quill::LogLevel to_quill_level(level lvl)
        {
            switch (lvl)
            {
            case level::trace:    return quill::LogLevel::TraceL1;
            case level::debug:    return quill::LogLevel::Debug;
            case level::info:     return quill::LogLevel::Info;
            case level::warn:     return quill::LogLevel::Warning;
            case level::error:    return quill::LogLevel::Error;
            case level::critical: return quill::LogLevel::Critical;
            default:              return quill::LogLevel::Info;
            }
        }
    }

    /// Log a Windows error with context (simple string message).
    inline void report_windows_error(const char* context, DWORD error_code, std::string_view message)
    {
        LOG_ERROR(default_logger(), "[{}] {}: {}", context, message,
            windows::hresult_as_string(static_cast<HRESULT>(error_code)));
    }

    /// Log a Windows error with context (format string with arguments).
    template<typename... Args>
    inline void report_windows_error(const char* context, DWORD error_code, std::format_string<Args...> fmt, Args&&... args)
    {
        LOG_ERROR(default_logger(), "[{}] {}: {}", context, std::format(fmt, std::forward<Args>(args)...),
            windows::hresult_as_string(static_cast<HRESULT>(error_code)));
    }

    /// Initialize logging with Quill backend.
    /// @param app_name application name for the logger
    /// @param enable_console if true, also log to stdout with colors
    /// @return pointer to the configured logger
    inline quill::Logger* initialize_logging(std::string_view app_name, bool enable_console = false)
    {
        // Start the backend thread
        quill::BackendOptions backend_options;
        quill::Backend::start(backend_options);

        std::vector<std::shared_ptr<quill::Sink>> sinks;

        if (enable_console)
        {
            auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console");
            sinks.push_back(console_sink);
        }

        quill::Logger* logger;
        if (sinks.empty())
        {
            // Create logger with no sinks initially (will be added later)
            logger = quill::Frontend::create_or_get_logger(
                "default",
                quill::Frontend::create_or_get_sink<quill::ConsoleSink>("null_console"));
        }
        else
        {
            logger = quill::Frontend::create_or_get_logger("default", std::move(sinks));
        }

        logger->set_log_level(quill::LogLevel::Debug);
        return logger;
    }

    /// Add console sink to existing logger.
    inline void enable_console_logging(level lvl = level::info)
    {
        auto logger = default_logger();
        auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console");
        logger->add_sink(console_sink);
        // Note: Quill doesn't support per-sink log levels the same way as spdlog
        (void)lvl;
    }

    /// Add rotating file sink to existing logger.
    inline void reconfigure_logging_for_file(const std::string& logFilePath)
    {
        auto logger = default_logger();

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

        quill::RotatingFileSinkConfig file_config;
        file_config.set_open_mode('w');
        file_config.set_filename(logFilePath);
        file_config.set_rotation_max_file_size(1024 * 1024 * 10); // 10MB
        file_config.set_max_backup_files(10);

        auto file_sink = quill::Frontend::create_or_get_sink<quill::RotatingFileSink>(
            "file_sink", file_config);
        logger->add_sink(file_sink);
    }
}

#else // spdlog (default)

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

namespace pnq::logging
{
    /// Log level abstraction
    enum class level
    {
        trace,
        debug,
        info,
        warn,
        error,
        critical
    };

    namespace detail
    {
        inline spdlog::level::level_enum to_spdlog_level(level lvl)
        {
            switch (lvl)
            {
            case level::trace:    return spdlog::level::trace;
            case level::debug:    return spdlog::level::debug;
            case level::info:     return spdlog::level::info;
            case level::warn:     return spdlog::level::warn;
            case level::error:    return spdlog::level::err;
            case level::critical: return spdlog::level::critical;
            default:              return spdlog::level::info;
            }
        }
    }

    /// Log a Windows error with context (simple string message).
    inline void report_windows_error(const char* context, DWORD error_code, std::string_view message)
    {
        spdlog::error("[{}] {}: {}", context, message, windows::hresult_as_string(static_cast<HRESULT>(error_code)));
    }

    /// Log a Windows error with context (format string with arguments).
    template<typename... Args>
    inline void report_windows_error(const char* context, DWORD error_code, std::format_string<Args...> fmt, Args&&... args)
    {
        spdlog::error("[{}] {}: {}", context, std::format(fmt, std::forward<Args>(args)...),
            windows::hresult_as_string(static_cast<HRESULT>(error_code)));
    }

    /// Initialize logging with MSVC debug output sink.
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
    inline void enable_console_logging(level lvl = level::info)
    {
        auto logger = spdlog::default_logger();
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(detail::to_spdlog_level(lvl));
        logger->sinks().push_back(console_sink);
    }

    /// Add rotating file sink to existing logger.
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
}

#endif
