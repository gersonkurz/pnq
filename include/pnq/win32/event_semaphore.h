#pragma once

#include <pnq\win32\security_attributes.h>
#include <pnq\win32\handle.h>

namespace pnq
{
    namespace win32
    {
        class EventSemaphore final
        {
        public:

            /// Default constructor
            EventSemaphore()
                :
                m_handle{ ::CreateEvent(nullptr, FALSE, FALSE, nullptr) }
            {
                if (!m_handle.is_valid())
                {
                    //logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, "CreateEvent() failed");
                }
            }

            explicit EventSemaphore(std::string_view name)
            {
                SecurityAttributes sa;
                const auto wide_name{ string::encode_as_utf16(name) };

                if (!m_handle.set(
                    ::CreateEventW(
                        sa.full_access_for_everyone(),
                        FALSE,
                        FALSE,
                        wide_name.data())))
                {
                    const auto result = GetLastError();
                    if (ERROR_ALREADY_EXISTS == result)
                    {
                        if (!m_handle.set(
                            ::OpenEventW(MUTEX_ALL_ACCESS, FALSE, wide_name.data())))
                        {
                            //PNQ_REPORT_LAST_ERRORF("OpenEventW({}) failed", name);
                        }
                    }
                    else
                    {
                        //PNQ_REPORT_LAST_ERRORF("CreateEventW({}) failed", name);
                    }
                }
            }

            ~EventSemaphore() = default;
            EventSemaphore(const EventSemaphore&) = delete;
            EventSemaphore& operator=(const EventSemaphore&) = delete;
            EventSemaphore(EventSemaphore&&) = delete;
            EventSemaphore& operator=(EventSemaphore&&) = delete;

            auto wait() const
            {
                return m_handle.wait();
            }

            auto wait_with_timeout(std::chrono::milliseconds timespan) const
            {
                return m_handle.wait_with_timeout(timespan);
            }

            /// Resets this event semaphore
            void reset() const
            {
                if (!::ResetEvent(m_handle))
                {
                    //PNQ_REPORT_LAST_ERROR("ResetEvent() failed");
                }
            }

            /// Sets this event semaphore
            void set() const
            {
                if (!::SetEvent(m_handle))
                {
                    //PNQ_REPORT_LAST_ERROR("SetEvent() failed");
                }
            }
        private:
            Handle m_handle;

        };
    }
} 
