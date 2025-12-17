#pragma once

#include <chrono>
#include <optional>
#include <thread>
#include <vector>

#include <pnq/pnq.h>
#include <pnq/win32/wstr_param.h>

namespace pnq
{
    namespace win32
    {
        /// Service configuration data returned by Service::query_config().
        struct ServiceConfig
        {
            std::string name;
            std::string display_name;
            std::string description;
            std::string binary_path;
            std::string account;
            std::vector<std::string> dependencies;
            DWORD start_type = 0;
            DWORD service_type = 0;
        };
        /// Base RAII wrapper for SC_HANDLE (service control handles).
        /// Logs errors on CloseServiceHandle failure.
        class ServiceHandle
        {
        public:
            ServiceHandle() : m_handle{} {}

            explicit ServiceHandle(SC_HANDLE handle) : m_handle{handle} {}

            ~ServiceHandle()
            {
                close();
            }

            ServiceHandle(const ServiceHandle&) = delete;
            ServiceHandle& operator=(const ServiceHandle&) = delete;

            ServiceHandle(ServiceHandle&& other) noexcept
                : m_handle{other.m_handle}
            {
                other.m_handle = nullptr;
            }

            ServiceHandle& operator=(ServiceHandle&& other) noexcept
            {
                if (this != &other)
                {
                    close();
                    m_handle = other.m_handle;
                    other.m_handle = nullptr;
                }
                return *this;
            }

            operator SC_HANDLE() const { return m_handle; }

            explicit operator bool() const { return m_handle != nullptr; }

            SC_HANDLE get() const { return m_handle; }

            SC_HANDLE release()
            {
                SC_HANDLE h = m_handle;
                m_handle = nullptr;
                return h;
            }

            void reset(SC_HANDLE handle = nullptr)
            {
                close();
                m_handle = handle;
            }

            void close()
            {
                if (m_handle)
                {
                    if (!CloseServiceHandle(m_handle))
                        PNQ_LOG_LAST_ERROR("CloseServiceHandle failed");
                    m_handle = nullptr;
                }
            }

        protected:
            SC_HANDLE m_handle;
        };

        class Service;

        /// RAII wrapper for Service Control Manager (SCM).
        /// Opens connection to SCM with specified access rights.
        class SCM final : public ServiceHandle
        {
        public:
            /// Open SCM with default SC_MANAGER_CONNECT access.
            SCM()
            {
                m_handle = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
                if (!m_handle)
                    PNQ_LOG_LAST_ERROR("OpenSCManager failed");
            }

            /// Open SCM with specified access rights.
            explicit SCM(DWORD desired_access)
            {
                m_handle = OpenSCManagerW(nullptr, nullptr, desired_access);
                if (!m_handle)
                    PNQ_LOG_LAST_ERROR("OpenSCManager failed");
            }

            /// Open SCM on remote machine.
            SCM(std::string_view machine_name, DWORD desired_access)
            {
                m_handle = OpenSCManagerW(wstr_param(machine_name), nullptr, desired_access);
                if (!m_handle)
                    PNQ_LOG_LAST_ERROR("OpenSCManager('{}') failed", machine_name);
            }

            /// Open a service by name.
            /// @param service_name UTF-8 encoded service name
            /// @param desired_access access rights for the service
            /// @return Service wrapper (check with operator bool)
            Service open_service(std::string_view service_name, DWORD desired_access) const;

            /// Create a new service.
            /// @param service_name internal service name
            /// @param display_name display name shown in services.msc
            /// @param binary_path path to service executable
            /// @param service_type SERVICE_WIN32_OWN_PROCESS, etc.
            /// @param start_type SERVICE_AUTO_START, SERVICE_DEMAND_START, etc.
            /// @param desired_access access rights for returned handle (default SERVICE_ALL_ACCESS)
            /// @return Service wrapper (check with operator bool)
            Service create_service(
                std::string_view service_name,
                std::string_view display_name,
                std::string_view binary_path,
                DWORD service_type = SERVICE_WIN32_OWN_PROCESS,
                DWORD start_type = SERVICE_DEMAND_START,
                DWORD desired_access = SERVICE_ALL_ACCESS) const;

            /// Create a new service with full configuration.
            /// @param config service configuration
            /// @param desired_access access rights for returned handle (default SERVICE_ALL_ACCESS)
            /// @return Service wrapper (check with operator bool)
            Service create_service(const ServiceConfig& config, DWORD desired_access = SERVICE_ALL_ACCESS) const;
        };

        /// RAII wrapper for Windows Service handle.
        /// Provides methods to start, stop, and query service status.
        class Service final : public ServiceHandle
        {
        public:
            Service() = default;

            Service(SC_HANDLE handle, std::string name)
                : ServiceHandle(handle), m_name{std::move(name)} {}

            Service(Service&& other) noexcept
                : ServiceHandle(std::move(other)), m_name{std::move(other.m_name)} {}

            Service& operator=(Service&& other) noexcept
            {
                ServiceHandle::operator=(std::move(other));
                m_name = std::move(other.m_name);
                return *this;
            }

            /// Get the service name.
            const std::string& name() const { return m_name; }

            /// Start the service.
            /// @param argc number of arguments
            /// @param argv service arguments (may be nullptr)
            /// @return true if started or already running
            bool start(DWORD argc = 0, LPCWSTR* argv = nullptr) const
            {
                if (!StartServiceW(m_handle, argc, argv))
                {
                    DWORD err = GetLastError();
                    if (err != ERROR_SERVICE_ALREADY_RUNNING)
                    {
                        PNQ_LOG_WIN_ERROR(err, "StartService('{}') failed", m_name);
                        return false;
                    }
                }
                return true;
            }

            /// Stop the service.
            /// @return true if stop signal sent successfully
            bool stop() const
            {
                SERVICE_STATUS status{};
                if (!ControlService(m_handle, SERVICE_CONTROL_STOP, &status))
                {
                    DWORD err = GetLastError();
                    if (err != ERROR_SERVICE_NOT_ACTIVE)
                    {
                        PNQ_LOG_WIN_ERROR(err, "ControlService('{}', STOP) failed", m_name);
                        return false;
                    }
                }
                return true;
            }

            /// Query current service status.
            /// @param status receives the service status
            /// @return true if query succeeded
            bool query_status(SERVICE_STATUS& status) const
            {
                if (!QueryServiceStatus(m_handle, &status))
                {
                    PNQ_LOG_LAST_ERROR("QueryServiceStatus('{}') failed", m_name);
                    return false;
                }
                return true;
            }

            /// Query current service status with process info.
            /// @param status receives the extended service status
            /// @return true if query succeeded
            bool query_status(SERVICE_STATUS_PROCESS& status) const
            {
                DWORD needed = 0;
                if (!QueryServiceStatusEx(m_handle, SC_STATUS_PROCESS_INFO,
                        reinterpret_cast<LPBYTE>(&status), sizeof(status), &needed))
                {
                    PNQ_LOG_LAST_ERROR("QueryServiceStatusEx('{}') failed", m_name);
                    return false;
                }
                return true;
            }

            /// Check if service is running.
            /// @return true if service is in SERVICE_RUNNING state
            bool is_running() const
            {
                SERVICE_STATUS status{};
                if (!query_status(status))
                    return false;
                return status.dwCurrentState == SERVICE_RUNNING;
            }

            /// Check if service is stopped.
            /// @return true if service is in SERVICE_STOPPED state
            bool is_stopped() const
            {
                SERVICE_STATUS status{};
                if (!query_status(status))
                    return false;
                return status.dwCurrentState == SERVICE_STOPPED;
            }

            /// Get current service state.
            /// @return service state (SERVICE_RUNNING, SERVICE_STOPPED, etc.) or 0 on error
            DWORD current_state() const
            {
                SERVICE_STATUS status{};
                if (!query_status(status))
                    return 0;
                return status.dwCurrentState;
            }

            /// Wait until service reaches stopped state.
            /// Uses dwWaitHint from service status to determine poll interval.
            /// @param timeout maximum time to wait
            /// @return true if service is stopped, false on timeout
            bool wait_until_stopped(std::chrono::milliseconds timeout = std::chrono::seconds{30}) const
            {
                return wait_for_state(SERVICE_STOPPED, timeout);
            }

            /// Wait until service reaches running state.
            /// Uses dwWaitHint from service status to determine poll interval.
            /// @param timeout maximum time to wait
            /// @return true if service is running, false on timeout
            bool wait_until_running(std::chrono::milliseconds timeout = std::chrono::seconds{30}) const
            {
                return wait_for_state(SERVICE_RUNNING, timeout);
            }

            /// Query full service configuration.
            /// @return ServiceConfig if successful, nullopt on error
            std::optional<ServiceConfig> query_config() const
            {
                DWORD needed = 0;
                QueryServiceConfigW(m_handle, nullptr, 0, &needed);
                if (needed == 0)
                {
                    PNQ_LOG_LAST_ERROR("QueryServiceConfig('{}') failed", m_name);
                    return std::nullopt;
                }

                std::vector<BYTE> buffer(needed);
                auto* config = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(buffer.data());

                if (!QueryServiceConfigW(m_handle, config, needed, &needed))
                {
                    PNQ_LOG_LAST_ERROR("QueryServiceConfig('{}') failed", m_name);
                    return std::nullopt;
                }

                ServiceConfig result;
                result.name = m_name;
                result.display_name = config->lpDisplayName ? string::encode_as_utf8(config->lpDisplayName) : "";
                result.binary_path = config->lpBinaryPathName ? string::encode_as_utf8(config->lpBinaryPathName) : "";
                result.account = config->lpServiceStartName ? string::encode_as_utf8(config->lpServiceStartName) : "";
                result.start_type = config->dwStartType;
                result.service_type = config->dwServiceType;

                // Parse dependencies (double-null-terminated string)
                if (config->lpDependencies)
                {
                    const wchar_t* dep = config->lpDependencies;
                    while (*dep)
                    {
                        result.dependencies.push_back(string::encode_as_utf8(dep));
                        dep += wcslen(dep) + 1;
                    }
                }

                // Query description separately
                result.description = query_description();

                return result;
            }

            /// Query service description.
            /// @return description string, empty on error
            std::string query_description() const
            {
                DWORD needed = 0;
                QueryServiceConfig2W(m_handle, SERVICE_CONFIG_DESCRIPTION, nullptr, 0, &needed);
                if (needed == 0)
                    return {};

                std::vector<BYTE> buffer(needed);
                if (!QueryServiceConfig2W(m_handle, SERVICE_CONFIG_DESCRIPTION, buffer.data(), needed, &needed))
                    return {};

                auto* desc = reinterpret_cast<SERVICE_DESCRIPTIONW*>(buffer.data());
                return (desc && desc->lpDescription) ? string::encode_as_utf8(desc->lpDescription) : "";
            }

            /// Set service description.
            /// @param description new description text
            /// @return true if successful
            bool set_description(std::string_view description) const
            {
                std::wstring wide_desc = string::encode_as_utf16(description);
                SERVICE_DESCRIPTIONW desc;
                desc.lpDescription = const_cast<LPWSTR>(wide_desc.c_str());

                if (!ChangeServiceConfig2W(m_handle, SERVICE_CONFIG_DESCRIPTION, &desc))
                {
                    PNQ_LOG_LAST_ERROR("ChangeServiceConfig2('{}', DESCRIPTION) failed", m_name);
                    return false;
                }
                return true;
            }

            /// Change service configuration.
            /// Pass SERVICE_NO_CHANGE for DWORD params or nullptr/empty for string params to keep existing values.
            /// @return true if successful
            bool change_config(
                DWORD service_type,
                DWORD start_type,
                DWORD error_control,
                std::string_view binary_path = {},
                std::string_view load_order_group = {},
                std::string_view dependencies = {},
                std::string_view account = {},
                std::string_view password = {},
                std::string_view display_name = {}) const
            {
                auto to_lpcwstr = [](std::string_view sv, std::wstring& storage) -> LPCWSTR {
                    if (sv.empty()) return nullptr;
                    storage = string::encode_as_utf16(sv);
                    return storage.c_str();
                };

                std::wstring w_binary, w_group, w_deps, w_account, w_password, w_display;

                if (!ChangeServiceConfigW(
                    m_handle,
                    service_type,
                    start_type,
                    error_control,
                    to_lpcwstr(binary_path, w_binary),
                    to_lpcwstr(load_order_group, w_group),
                    nullptr, // tag id
                    to_lpcwstr(dependencies, w_deps),
                    to_lpcwstr(account, w_account),
                    to_lpcwstr(password, w_password),
                    to_lpcwstr(display_name, w_display)))
                {
                    PNQ_LOG_LAST_ERROR("ChangeServiceConfig('{}') failed", m_name);
                    return false;
                }
                return true;
            }

            /// Delete the service.
            /// @return true if deleted or already marked for deletion
            bool remove() const
            {
                if (!DeleteService(m_handle))
                {
                    DWORD err = GetLastError();
                    if (err != ERROR_SERVICE_MARKED_FOR_DELETE)
                    {
                        PNQ_LOG_WIN_ERROR(err, "DeleteService('{}') failed", m_name);
                        return false;
                    }
                }
                return true;
            }

        private:
            bool wait_for_state(DWORD target_state, std::chrono::milliseconds timeout) const
            {
                auto deadline = std::chrono::steady_clock::now() + timeout;
                DWORD last_checkpoint = 0;

                while (std::chrono::steady_clock::now() < deadline)
                {
                    SERVICE_STATUS_PROCESS ssp{};
                    if (!query_status(ssp))
                        return false;

                    if (ssp.dwCurrentState == target_state)
                        return true;

                    // Use wait hint clamped to 1-10 seconds per Microsoft recommendation
                    DWORD wait_ms = ssp.dwWaitHint;
                    if (wait_ms < 1000) wait_ms = 1000;
                    if (wait_ms > 10000) wait_ms = 10000;

                    // Track checkpoint progress (could reset deadline here if desired)
                    if (ssp.dwCheckPoint > last_checkpoint)
                        last_checkpoint = ssp.dwCheckPoint;

                    std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
                }

                SERVICE_STATUS_PROCESS ssp{};
                return query_status(ssp) && ssp.dwCurrentState == target_state;
            }

            std::string m_name;
        };

        inline Service SCM::open_service(std::string_view service_name, DWORD desired_access) const
        {
            SC_HANDLE svc = OpenServiceW(m_handle, wstr_param(service_name), desired_access);
            if (!svc)
                PNQ_LOG_LAST_ERROR("OpenService('{}') failed", service_name);
            return Service(svc, std::string{service_name});
        }

        inline Service SCM::create_service(
            std::string_view service_name,
            std::string_view display_name,
            std::string_view binary_path,
            DWORD service_type,
            DWORD start_type,
            DWORD desired_access) const
        {
            SC_HANDLE svc = CreateServiceW(
                m_handle,
                wstr_param(service_name),
                wstr_param(display_name),
                desired_access,
                service_type,
                start_type,
                SERVICE_ERROR_NORMAL,
                wstr_param(binary_path),
                nullptr,  // load order group
                nullptr,  // tag id
                nullptr,  // dependencies
                nullptr,  // account (LocalSystem)
                nullptr); // password

            if (!svc)
                PNQ_LOG_LAST_ERROR("CreateService('{}') failed", service_name);
            return Service(svc, std::string{service_name});
        }

        inline Service SCM::create_service(const ServiceConfig& config, DWORD desired_access) const
        {
            // Build dependencies as double-null-terminated string
            std::wstring deps;
            for (const auto& dep : config.dependencies)
            {
                deps += string::encode_as_utf16(dep);
                deps += L'\0';
            }

            SC_HANDLE svc = CreateServiceW(
                m_handle,
                wstr_param(config.name),
                wstr_param(config.display_name),
                desired_access,
                config.service_type ? config.service_type : SERVICE_WIN32_OWN_PROCESS,
                config.start_type ? config.start_type : SERVICE_DEMAND_START,
                SERVICE_ERROR_NORMAL,
                wstr_param(config.binary_path),
                nullptr,  // load order group
                nullptr,  // tag id
                deps.empty() ? nullptr : deps.c_str(),
                config.account.empty() ? nullptr : wstr_param(config.account),
                nullptr); // password

            if (!svc)
            {
                PNQ_LOG_LAST_ERROR("CreateService('{}') failed", config.name);
                return Service(nullptr, config.name);
            }

            Service service(svc, config.name);

            // Set description if provided
            if (!config.description.empty())
                service.set_description(config.description);

            return service;
        }

    } // namespace win32
} // namespace pnq
