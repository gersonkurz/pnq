#pragma once

#include <pnq/pnq.h>
#include <pnq/win32/wstr_param.h>

namespace pnq
{
    namespace win32
    {
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

        private:
            std::string m_name;
        };

        inline Service SCM::open_service(std::string_view service_name, DWORD desired_access) const
        {
            SC_HANDLE svc = OpenServiceW(m_handle, wstr_param(service_name), desired_access);
            if (!svc)
                PNQ_LOG_LAST_ERROR("OpenService('{}') failed", service_name);
            return Service(svc, std::string{service_name});
        }

    } // namespace win32
} // namespace pnq
