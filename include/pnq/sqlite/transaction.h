#pragma once

#if __has_include(<sqlite3.h>)
#include <sqlite3.h>

#include <string_view>
#include <utility>

#include <pnq/log.h>
#include <pnq/sqlite/statement.h>

namespace pnq
{
    namespace sqlite
    {
        /// RAII transaction wrapper - auto-rollback on destruction if not committed.
        class Transaction final
        {
        public:
            explicit Transaction(Database& db)
                : m_db{db},
                  m_active{false}
            {
                if (m_db.is_valid())
                {
                    if (m_db.execute("BEGIN TRANSACTION;"))
                    {
                        m_active = true;
                    }
                }
                else
                {
                    PNQ_LOG_ERROR("sqlite::Transaction: Database is not valid, cannot begin transaction.");
                }
            }

            ~Transaction()
            {
                if (m_active)
                {
                    m_db.execute("ROLLBACK;");
                }
            }

            PNQ_DECLARE_NON_COPYABLE(Transaction)

            /// Check if transaction is active.
            explicit operator bool() const { return m_active; }

            /// Commit the transaction.
            /// @return true on success
            bool commit()
            {
                if (!m_active)
                    return false;
                m_active = false;
                return m_db.execute("COMMIT;");
            }

            /// Rollback the transaction.
            /// @return always false (signals transaction was aborted)
            bool rollback()
            {
                if (m_active)
                {
                    m_active = false;
                    m_db.execute("ROLLBACK;");
                }
                return false;
            }

            /// Execute a statement within the transaction.
            template <typename... Args>
            bool execute(std::string_view sql, Args&&... args)
            {
                if (!m_db.is_valid())
                    return false;

                Statement stmt{m_db, sql};

                bool all_ok = (stmt.bind(std::forward<Args>(args)) && ...);
                return all_ok && stmt.execute();
            }

        private:
            Database& m_db;
            bool m_active;
        };

    } // namespace sqlite
} // namespace pnq

#endif // __has_include(<sqlite3.h>)
