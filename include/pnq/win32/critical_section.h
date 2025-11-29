#pragma once

#include <pnq/pnq.h>

namespace pnq
{
    /*!
     *  \addtogroup Win32
     *  @{
     */
    namespace win32
    {

        /// A critical section. This class cannot be inherited..
        class CriticalSection final
        {
        public:

            /// Default constructor.
            CriticalSection()
                : m_cs{}
            {
                InitializeCriticalSection(&m_cs);
            }

            /// Destructor.
            ~CriticalSection()
            {
                DeleteCriticalSection(&m_cs);
            }

            CriticalSection(const CriticalSection&) = delete;
            CriticalSection& operator=(const CriticalSection&) = delete;
            CriticalSection(CriticalSection&&) = delete;
            CriticalSection& operator=(CriticalSection&&) = delete;

            /// Acquires this critical section
            void acquire()
            {
                EnterCriticalSection(&m_cs);
            }

            ///-------------------------------------------------------------------------------------------------
            /// Attempts to acquire this critical section
            ///
            /// \returns True if it succeeds, false if it fails.

            bool try_acquire()
            {
                return TryEnterCriticalSection(&m_cs) != 0;
            }

            /// Releases this critical section
            void release()
            {
                LeaveCriticalSection(&m_cs);
            }
            
        private:
            /// The critical section OS-level data
            CRITICAL_SECTION m_cs;
        };
    }
}
