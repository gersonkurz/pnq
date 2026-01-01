#pragma once

#if __has_include(<sqlite3.h>)
#include <sqlite3.h>

#include <format>
#include <mutex>
#include <string>
#include <string_view>

#include <pnq/log.h>
#include <pnq/pnq.h>

namespace pnq
{
    namespace sqlite
    {
        /// Convert SQLite result code to string.
        inline std::string result_code_as_string(int rc)
        {
#define PNQ_SQLITE_RC_CASE(code) case SQLITE_##code: return #code
            switch (rc)
            {
                PNQ_SQLITE_RC_CASE(OK);
                PNQ_SQLITE_RC_CASE(ERROR);
                PNQ_SQLITE_RC_CASE(INTERNAL);
                PNQ_SQLITE_RC_CASE(PERM);
                PNQ_SQLITE_RC_CASE(ABORT);
                PNQ_SQLITE_RC_CASE(BUSY);
                PNQ_SQLITE_RC_CASE(LOCKED);
                PNQ_SQLITE_RC_CASE(NOMEM);
                PNQ_SQLITE_RC_CASE(READONLY);
                PNQ_SQLITE_RC_CASE(INTERRUPT);
                PNQ_SQLITE_RC_CASE(IOERR);
                PNQ_SQLITE_RC_CASE(CORRUPT);
                PNQ_SQLITE_RC_CASE(NOTFOUND);
                PNQ_SQLITE_RC_CASE(FULL);
                PNQ_SQLITE_RC_CASE(CANTOPEN);
                PNQ_SQLITE_RC_CASE(PROTOCOL);
                PNQ_SQLITE_RC_CASE(EMPTY);
                PNQ_SQLITE_RC_CASE(SCHEMA);
                PNQ_SQLITE_RC_CASE(TOOBIG);
                PNQ_SQLITE_RC_CASE(CONSTRAINT);
                PNQ_SQLITE_RC_CASE(MISMATCH);
                PNQ_SQLITE_RC_CASE(MISUSE);
                PNQ_SQLITE_RC_CASE(NOLFS);
                PNQ_SQLITE_RC_CASE(AUTH);
                PNQ_SQLITE_RC_CASE(FORMAT);
                PNQ_SQLITE_RC_CASE(RANGE);
                PNQ_SQLITE_RC_CASE(NOTADB);
                PNQ_SQLITE_RC_CASE(ROW);
                PNQ_SQLITE_RC_CASE(DONE);
            default:
                return std::to_string(rc);
            }
#undef PNQ_SQLITE_RC_CASE
        }

        /// SQLite database wrapper with RAII semantics.
        class Database final
        {
        public:
            Database() : m_db{nullptr} {}

            ~Database() { close(); }

            PNQ_DECLARE_NON_COPYABLE(Database)

            /// Get SQLite version string.
            const char* version() const
            {
                return SQLITE_VERSION " [" SQLITE_SOURCE_ID "]";
            }

            /// Open a database file.
            /// @param filename path to database file (created if doesn't exist)
            /// @return true on success
            bool open(std::string_view filename)
            {
                close();
                int rc = sqlite3_open_v2(filename.data(), &m_db,
                    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);

                if (rc != SQLITE_OK)
                {
                    format_error(__LINE__, rc, "sqlite3_open({}) failed", filename);
                    m_db = nullptr;
                    return false;
                }
                sqlite3_busy_timeout(m_db, 60000);
                return true;
            }

            /// Close the database.
            void close()
            {
                if (m_db)
                {
                    sqlite3_close(m_db);
                    m_db = nullptr;
                }
            }

            /// Execute a SQL statement directly (no results).
            /// @param statement SQL to execute
            /// @return true on success
            bool execute(std::string_view statement)
            {
                PNQ_LOG_DEBUG("sqlite::Database executing SQL: {}", statement);
                char* error_message = nullptr;
                int rc = sqlite3_exec(m_db, statement.data(), nullptr, 0, &error_message);
                if (rc != SQLITE_OK)
                {
                    format_error(__LINE__, rc, "sqlite3_exec({}) failed with {}", statement, error_message);
                    sqlite3_free(error_message);
                    return false;
                }
                return true;
            }

            /// Check if database is open and valid.
            bool is_valid() const { return m_db != nullptr; }

            /// Check if a table exists.
            bool table_exists(std::string_view name);

            /// Get the rowid of the last inserted row.
            int64_t last_insert_rowid() const
            {
                return sqlite3_last_insert_rowid(m_db);
            }

            /// Get number of rows changed by last statement.
            int changes_count() const
            {
                return sqlite3_changes(m_db);
            }

            /// Get the last error message.
            const std::string& last_error() const { return m_last_error; }

            /// Get the maximum number of parameters allowed.
            int max_params() const
            {
                return sqlite3_limit(m_db, SQLITE_LIMIT_VARIABLE_NUMBER, -1);
            }

            /// Get the database mutex for thread-safe operations.
            std::recursive_mutex& mutex() { return m_mutex; }

        private:
            friend class Statement;

            bool raise_error(int line, int rc, std::string_view message)
            {
                const char* msg = sqlite3_errmsg(m_db);

                std::string output;
                output.append(result_code_as_string(rc));
                output.append(": ");
                if (msg)
                    output.append(msg);
                output.append(" in ");
                output.append(__FILE__);
                output.append("(");
                output.append(std::to_string(line));
                output.append("): ");
                output.append(message);
                m_last_error = output;
                PNQ_LOG_ERROR("{}", output);
                return false;
            }

            template <typename... Args>
            bool format_error(int line, int rc, std::string_view text, Args&&... args)
            {
                return raise_error(line, rc, std::vformat(text, std::make_format_args(args...)));
            }

        private:
            sqlite3* m_db;
            mutable std::recursive_mutex m_mutex;
            mutable std::string m_last_error;
        };

    } // namespace sqlite
} // namespace pnq

#endif // __has_include(<sqlite3.h>)
