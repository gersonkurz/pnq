/// @file base_app.h
/// @brief Application initialization and lifecycle management.
///
/// BaseApp handles startup sequence: memory leak detection, logging
/// initialization, config loading, and log file configuration.
#pragma once

#include <filesystem>
#include <crtdbg.h>
#include <pnq/config/toml_backend.h>
#include <pnq/config/section.h>
#include <pnq/logging.h>
#include <spdlog/spdlog.h>
#include <pnq/path.h>

namespace pnq
{
    namespace utils
    {
        /// @brief Application bootstrap class.
        ///
        /// Instantiate at the start of main() to initialize:
        /// 1. CRT memory leak detection (debug builds)
        /// 2. Logging with console/debug output
        /// 3. Configuration from TOML file
        /// 4. File logging with configured path
        /// 5. Log level from configuration
        class AppInit final
        {
        public:
            AppInit(std::string_view app_name, config::Section* pConfigSection = nullptr)
            {
                // setup memory leak detection
                int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
                flag |= _CRTDBG_LEAK_CHECK_DF;
                flag |= _CRTDBG_ALLOC_MEM_DF;
                _CrtSetDbgFlag(flag);
                _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
                _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
                // Change this to leaking allocation's number to break there
                _CrtSetBreakAlloc(-1);

                // Step 1: Initialize logging with console/debug output only
                auto logger = logging::initialize_logging(app_name);
                logger->info("{} starting up", app_name);

                // Step 2: Get AppData path and determine config/log paths
                m_appDataPath = path::get_roaming_app_data();
                m_configPath = m_appDataPath / (std::string(app_name) + ".toml");
                logger->info("AppData path: {}", m_appDataPath.string());
                logger->info("Loading configuration from: {}", m_configPath.string());

                // Step 3: Load configuration
                m_pBackend = new config::TomlBackend{m_configPath.string()};

                if(pConfigSection)
                {
                    pConfigSection->load(*m_pBackend);

                    // Step 4: Configure file logging
                    const auto logFilePath = (m_appDataPath / (std::string(app_name) + ".log")).string();
                    logger->info("Log file path: {}", logFilePath);
                    logging::reconfigure_logging_for_file(logFilePath);

                    // Step 5: Apply log level from config
                    // const auto logLevel = pConfigSection->logging.logLevel.get();
                    // logger->set_level(spdlog::level::from_str(logLevel));
                    // logger->info("Log level set to: {}", logLevel);
                }
            }

            ~AppInit()
            {
                delete m_pBackend;
            }

            config::TomlBackend *m_pBackend = nullptr; ///< Configuration backend.
            std::filesystem::path m_appDataPath;        ///< %ROAMINGAPPDATA%/appname path.
            std::filesystem::path m_configPath;         ///< Path to appname.toml.
        };
    } // namespace utils
} // namespace pserv

