/// @file app_init.h
/// @brief Application initialization and lifecycle management.
#pragma once

#include <filesystem>

#include <crtdbg.h>
#include <spdlog/spdlog.h>
#include <pnq/config/toml_backend.h>
#include <pnq/config/section.h>
#include <pnq/logging.h>
#include <pnq/path.h>
#include <pnq/pnq.h>

namespace pnq
{
    namespace utils
    {
        /// Application bootstrap class.
        ///
        /// Instantiate at the start of main() to initialize:
        /// 1. CRT memory leak detection (debug builds)
        /// 2. Logging with MSVC debug output (and optionally console)
        /// 3. Configuration from TOML file
        /// 4. File logging with configured path
        class AppInit final
        {
        public:
            /// Initialize application.
            /// @param app_name application name (used for paths and logging)
            /// @param config_section optional config section to load
            /// @param enable_console_logging if true, also log to stdout with colors
            AppInit(std::string_view app_name, config::Section *config_section = nullptr, bool enable_console_logging = false)
            {
#ifdef _DEBUG
                // Setup memory leak detection
                int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
                flag |= _CRTDBG_LEAK_CHECK_DF;
                flag |= _CRTDBG_ALLOC_MEM_DF;
                _CrtSetDbgFlag(flag);
                _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
                _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
                _CrtSetBreakAlloc(-1);
#endif
                // Initialize logging with MSVC debug output (and optionally console)
                auto logger = logging::initialize_logging(app_name, enable_console_logging);
                logger->info("{} starting up", app_name);

                // Get AppData path and determine config/log paths
                m_app_data_path = path::get_roaming_app_data(app_name);
                m_config_path = m_app_data_path / (std::string(app_name) + ".toml");
                logger->info("AppData path: {}", m_app_data_path.string());
                logger->info("Loading configuration from: {}", m_config_path.string());

                // Load configuration
                m_backend = new config::TomlBackend{m_config_path.string()};

                if (config_section)
                {
                    config_section->load(*m_backend);

                    // Configure file logging
                    const auto log_file_path = (m_app_data_path / (std::string(app_name) + ".log")).string();
                    logger->info("Log file path: {}", log_file_path);
                    logging::reconfigure_logging_for_file(log_file_path);
                }
            }

            ~AppInit()
            {
                delete m_backend;
            }

            /// Get the configuration backend.
            config::TomlBackend *backend() const { return m_backend; }

            /// Get the app data path.
            const std::filesystem::path &app_data_path() const { return m_app_data_path; }

            /// Get the config file path.
            const std::filesystem::path &config_path() const { return m_config_path; }

        private:
            PNQ_DECLARE_NON_COPYABLE(AppInit)

            config::TomlBackend *m_backend = nullptr;
            std::filesystem::path m_app_data_path;
            std::filesystem::path m_config_path;
        };
    } // namespace utils
} // namespace pnq

