#pragma once

#if __has_include(<sqlite3.h>)
#include <sqlite3.h>

#include <cassert>
#include <cstring>
#include <functional>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <pnq/log.h>
#include <pnq/sqlite/database.h>

namespace pnq
{
    namespace sqlite
    {
        /// SQLite prepared statement wrapper with RAII semantics.
        class Statement final
        {
        public:
            /// Create statement with SQL.
            Statement(Database& db, std::string_view sql)
                : m_lock{db.mutex()},
                  m_db{db},
                  m_stmt{nullptr},
                  m_param_index{1},
                  m_done{false}
            {
                bind_sql(sql);
            }

            /// Create statement without SQL (call bind_sql later).
            explicit Statement(Database& db)
                : m_lock{db.mutex()},
                  m_db{db},
                  m_stmt{nullptr},
                  m_param_index{1},
                  m_done{false}
            {
            }

            ~Statement()
            {
                if (m_stmt)
                {
                    sqlite3_finalize(m_stmt);
                    m_stmt = nullptr;
                }
            }

            PNQ_DECLARE_NON_COPYABLE(Statement)

            /// Bind SQL to statement (only if not provided in constructor).
            bool bind_sql(std::string_view sql)
            {
                assert(!m_stmt);
                assert(m_sql.empty());

                m_sql = sql;
                m_param_index = 1;
                m_done = false;

                int rc;
                do
                {
                    rc = sqlite3_prepare_v2(m_db.m_db, m_sql.data(), static_cast<int>(m_sql.length()), &m_stmt, nullptr);
                    if (rc == SQLITE_BUSY)
                    {
                        std::this_thread::yield();
                    }
                    else if (rc != SQLITE_OK)
                    {
                        m_stmt = nullptr;
                        return m_db.format_error(__LINE__, rc, "sqlite3_prepare_v2({}) failed", m_sql);
                    }
                } while (rc == SQLITE_BUSY);

                return true;
            }

            /// Reset statement for reuse with new parameters.
            void reset()
            {
                if (m_stmt)
                {
                    sqlite3_reset(m_stmt);
                    sqlite3_clear_bindings(m_stmt);
                    m_param_index = 1;
                    m_done = false;
                    m_string_storage.clear();
                }
            }

            /// Check if statement is valid.
            bool is_valid() const { return m_stmt != nullptr; }

            /// Check if query returned no results.
            bool is_empty() const { return m_done; }

            /// Get the database reference.
            Database& database() const { return m_db; }

            /// Get number of columns in result.
            size_t column_count() const
            {
                return sqlite3_column_count(m_stmt);
            }

            /// Get column name by index.
            std::string column_name(int index) const
            {
                return sqlite3_column_name(m_stmt, index);
            }

            /// Get column type by index.
            int column_type(int index) const
            {
                return sqlite3_column_type(m_stmt, index);
            }

            // --- Parameter binding ---

            bool bind_null()
            {
                int rc = sqlite3_bind_null(m_stmt, m_param_index++);
                if (rc != SQLITE_OK)
                    return m_db.raise_error(__LINE__, rc, "sqlite3_bind_null() failed");
                return true;
            }

            bool bind(std::nullptr_t)
            {
                return bind_null();
            }

            bool bind(std::string_view text)
            {
                // Store copy since sqlite needs pointer to remain valid
                m_string_storage.push_back(std::string{text});
                const auto& stored = m_string_storage.back();

                int rc = sqlite3_bind_text(m_stmt, m_param_index++, stored.c_str(), static_cast<int>(stored.length()), nullptr);
                if (rc != SQLITE_OK)
                    return m_db.format_error(__LINE__, rc, "sqlite3_bind_text({}) failed", stored);
                return true;
            }

            bool bind_nullable(const std::string& text)
            {
                if (text.empty())
                    return bind_null();
                return bind(std::string_view{text});
            }

            bool bind(int64_t value)
            {
                int rc = sqlite3_bind_int64(m_stmt, m_param_index++, value);
                if (rc != SQLITE_OK)
                    return m_db.format_error(__LINE__, rc, "sqlite3_bind_int64({}) failed", value);
                return true;
            }

            bool bind(int32_t value)
            {
                int rc = sqlite3_bind_int(m_stmt, m_param_index++, value);
                if (rc != SQLITE_OK)
                    return m_db.format_error(__LINE__, rc, "sqlite3_bind_int({}) failed", value);
                return true;
            }

            bool bind(double value)
            {
                int rc = sqlite3_bind_double(m_stmt, m_param_index++, value);
                if (rc != SQLITE_OK)
                    return m_db.format_error(__LINE__, rc, "sqlite3_bind_double({}) failed", value);
                return true;
            }

            bool bind(const std::vector<uint8_t>& blob)
            {
                int rc = sqlite3_bind_blob(m_stmt, m_param_index++, blob.data(), static_cast<int>(blob.size()), SQLITE_STATIC);
                if (rc != SQLITE_OK)
                    return m_db.raise_error(__LINE__, rc, "sqlite3_bind_blob() failed");
                return true;
            }

            template <typename T>
            bool bind_nullable(const std::optional<T>& value)
            {
                if (value.has_value())
                    return bind(value.value());
                return bind_null();
            }

            /// Bind a value from another statement's column.
            bool bind_from(const Statement& source, int column_index)
            {
                if (!source.is_valid())
                    return false;

                switch (source.column_type(column_index))
                {
                case SQLITE_INTEGER:
                    return bind(source.get_int64(column_index));
                case SQLITE_FLOAT:
                    return bind(source.get_double(column_index));
                case SQLITE_TEXT:
                    return bind(source.get_text(column_index));
                case SQLITE_BLOB:
                    return bind(source.get_blob(column_index));
                case SQLITE_NULL:
                    return bind_null();
                default:
                    PNQ_LOG_ERROR("Unsupported column type {} in bind_from", source.column_type(column_index));
                    return false;
                }
            }

            // --- Result retrieval ---

            bool is_null(int index) const
            {
                return sqlite3_column_type(m_stmt, index) == SQLITE_NULL;
            }

            double get_double(int index) const
            {
                return sqlite3_column_double(m_stmt, index);
            }

            int64_t get_int64(int index) const
            {
                return sqlite3_column_int64(m_stmt, index);
            }

            int32_t get_int32(int index) const
            {
                return sqlite3_column_int(m_stmt, index);
            }

            std::string get_text(int index) const
            {
                auto p = reinterpret_cast<const char*>(sqlite3_column_text(m_stmt, index));
                return p ? std::string{p} : std::string{};
            }

            std::vector<uint8_t> get_blob(int index) const
            {
                auto p = reinterpret_cast<const uint8_t*>(sqlite3_column_blob(m_stmt, index));
                int size = sqlite3_column_bytes(m_stmt, index);

                std::vector<uint8_t> result;
                if (size > 0 && p)
                {
                    result.resize(size);
                    std::memcpy(result.data(), p, size);
                }
                return result;
            }

            // --- Execution ---

            /// Execute statement (for INSERT/UPDATE/DELETE or SELECT with results).
            bool execute()
            {
                m_done = false;
                for (;;)
                {
                    int rc = sqlite3_step(m_stmt);
                    if (rc == SQLITE_ROW)
                        return true;
                    if (rc == SQLITE_DONE)
                    {
                        m_done = true;
                        return true;
                    }
                    return m_db.format_error(__LINE__, rc, "Statement::execute({}) failed", m_sql);
                }
            }

            /// Get next result row.
            /// @return true if row available, false if done or error
            bool next()
            {
                if (m_done)
                    return false;

                for (;;)
                {
                    int rc = sqlite3_step(m_stmt);
                    if (rc == SQLITE_DONE)
                    {
                        m_done = true;
                        return false;
                    }
                    if (rc == SQLITE_ROW)
                        return true;

                    m_done = true;
                    return m_db.format_error(__LINE__, rc, "Statement::next({}) failed", m_sql);
                }
            }

            /// Execute a query with callback for each row.
            template <typename Callback, typename... Args>
            bool query(Callback&& callback, const std::string& sql, Args&&... args)
            {
                if (!bind_sql(sql))
                    return false;

                // Bind all parameters
                bool all_ok = (bind(std::forward<Args>(args)) && ...);
                if (!all_ok)
                    return false;

                if (!is_valid())
                    return false;

                while (next())
                {
                    if (!callback())
                        return false;
                }
                return true;
            }

        private:
            std::list<std::string> m_string_storage; // stable pointers for sqlite
            std::lock_guard<std::recursive_mutex> m_lock;
            Database& m_db;
            sqlite3_stmt* m_stmt;
            int m_param_index;
            std::string m_sql;
            bool m_done;
        };

        // Now we can implement Database::table_exists since Statement is defined
        inline bool Database::table_exists(std::string_view name)
        {
            Statement stmt{*this, "SELECT name FROM sqlite_master WHERE type='table' AND name=?;"};
            stmt.bind(name);
            if (!stmt.execute())
                return false;
            return !stmt.is_empty();
        }

    } // namespace sqlite
} // namespace pnq

#endif // __has_include(<sqlite3.h>)
