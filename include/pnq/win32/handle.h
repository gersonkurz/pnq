#pragma once

#include <pnq/wstring.h>

#include <pnq/logging.h>
#include <pnq/windows_errors.h>

namespace pnq
{
	namespace win32
    {
        class Handle final
        {
        public:

            Handle()
                :
                m_handle{}
            {
            }

            explicit Handle(HANDLE hHandle)
                :
                m_handle{ hHandle }
            {
            }

        	~Handle()
            {
                close();
            }

            Handle(const Handle&) = delete;
            Handle& operator=(const Handle&) = delete;
            Handle(Handle&&) = delete;
            Handle& operator=(Handle&&) = delete;

            operator HANDLE() const
            {
                return m_handle;
            }

            bool wait_with_timeout(std::chrono::milliseconds timespan) const
            {
                const auto dwResult
                {
                    ::WaitForSingleObjectEx(m_handle, static_cast<DWORD>(timespan.count()), TRUE)
                };
                
                if (dwResult == WAIT_OBJECT_0)
                    return true;

                if (dwResult != WAIT_TIMEOUT)
                    //logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, "WaitForSingleObjectEx() failed");
                
                return false;
            }

            bool wait() const
            {
                const auto dwResult{ ::WaitForSingleObjectEx(m_handle, INFINITE, TRUE) };
                if (dwResult == WAIT_OBJECT_0)
                {
                    return true;
                }
                //logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, "WaitForSingleObjectEx() failed");
                return false;
            }

            
            static bool is_valid(HANDLE h)
            {
                return (h != 0) and (h != INVALID_HANDLE_VALUE);
            }

            bool is_valid() const
            {
                return is_valid(m_handle);
            }

            void clear()
            {
                m_handle = {};
            }

            bool set(HANDLE h)
            {
                m_handle = h;
                return is_valid();
            }

            void close()
            {
                if (is_valid())
                {
                    if (!::CloseHandle(m_handle))
                    {
                        //logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, std::format("CloseHandle({}) failed",
                       //     (void*)m_handle));
                    }
                    m_handle = {};
                }
            }

        private:
            HANDLE m_handle;
        };
    }
}

