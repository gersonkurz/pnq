#pragma once

/// @file log.h
/// @brief Unified logging macros supporting spdlog and Quill backends.
///
/// Define PNQ_USE_QUILL before including this header to use Quill instead of spdlog.
/// Both backends use std::format syntax for format strings.

#ifdef PNQ_USE_QUILL

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/RotatingFileSink.h>

namespace pnq::logging
{
    /// Get the default logger for the application.
    inline quill::Logger* default_logger()
    {
        static quill::Logger* logger = quill::Frontend::get_logger("default");
        return logger;
    }
}

#define PNQ_LOG_TRACE(...) LOG_TRACE_L1(pnq::logging::default_logger(), __VA_ARGS__)
#define PNQ_LOG_DEBUG(...) LOG_DEBUG(pnq::logging::default_logger(), __VA_ARGS__)
#define PNQ_LOG_INFO(...)  LOG_INFO(pnq::logging::default_logger(), __VA_ARGS__)
#define PNQ_LOG_WARN(...)  LOG_WARNING(pnq::logging::default_logger(), __VA_ARGS__)
#define PNQ_LOG_ERROR(...) LOG_ERROR(pnq::logging::default_logger(), __VA_ARGS__)
#define PNQ_LOG_CRITICAL(...) LOG_CRITICAL(pnq::logging::default_logger(), __VA_ARGS__)

#else // spdlog (default)

#include <spdlog/spdlog.h>

#define PNQ_LOG_TRACE(...) spdlog::trace(__VA_ARGS__)
#define PNQ_LOG_DEBUG(...) spdlog::debug(__VA_ARGS__)
#define PNQ_LOG_INFO(...)  spdlog::info(__VA_ARGS__)
#define PNQ_LOG_WARN(...)  spdlog::warn(__VA_ARGS__)
#define PNQ_LOG_ERROR(...) spdlog::error(__VA_ARGS__)
#define PNQ_LOG_CRITICAL(...) spdlog::critical(__VA_ARGS__)

#endif

