#pragma once

/// @file pnq/registry/value.h
/// @brief Registry value class - represents a named value with type and data

#include <pnq/registry/types.h>
#include <pnq/string.h>
#include <pnq/wstring.h>
#include <pnq/pnq.h>

#include <cassert>
#include <cstring>

namespace pnq
{
    namespace registry
    {
        /// A registry value with name, type, and data.
        ///
        /// Supports all standard Windows registry types: REG_SZ, REG_EXPAND_SZ,
        /// REG_MULTI_SZ, REG_DWORD, REG_QWORD, REG_BINARY, etc.
        ///
        /// Data is stored internally as UTF-16LE bytes (matching Windows registry format).
        class value final
        {
        public:
            /// Default constructor creates an unnamed value with unknown type.
            value()
                : m_type{REG_TYPE_UNKNOWN},
                  m_remove_flag{false}
            {
            }

            /// Construct a named value with unknown type.
            /// @param name Value name (empty string for default value)
            explicit value(std::string_view name)
                : m_name{name},
                  m_type{REG_TYPE_UNKNOWN},
                  m_remove_flag{false}
            {
            }

            /// Construct a value with name, type, and raw data.
            /// @param name Value name
            /// @param type Windows registry type (REG_SZ, REG_DWORD, etc.)
            /// @param data Raw byte data
            /// @param data_size Actual size of data to use (may be less than data.size())
            value(std::string_view name, uint32_t type, const bytes& data, uint32_t data_size)
                : m_name{name},
                  m_type{type},
                  m_data{data},
                  m_remove_flag{false}
            {
                if (data_size < m_data.size())
                {
                    m_data.resize(data_size);
                }
            }

            /// Copy constructor.
            value(const value& other) = default;

            /// Copy assignment.
            value& operator=(const value& other) = default;

            /// Move constructor.
            value(value&& other) noexcept = default;

            /// Move assignment.
            value& operator=(value&& other) noexcept = default;

            ~value() = default;

            // =================================================================
            // Queries
            // =================================================================

            /// Check if this is the default (unnamed) value for a key.
            /// @return true if name is empty
            bool is_default_value() const
            {
                return m_name.empty();
            }

            /// Get the value name.
            const std::string& name() const
            {
                return m_name;
            }

            /// Get the registry type.
            uint32_t type() const
            {
                return m_type;
            }

            /// Check if this value should be removed (for diff/merge operations).
            bool remove_flag() const
            {
                return m_remove_flag;
            }

            /// Set the remove flag.
            void set_remove_flag(bool flag)
            {
                m_remove_flag = flag;
            }

            // =================================================================
            // Type Setters
            // =================================================================

            /// Set as REG_NONE (empty value).
            void set_none()
            {
                m_data.clear();
                m_type = REG_NONE;
            }

            /// Set as REG_DWORD.
            /// @param val 32-bit unsigned integer
            void set_dword(uint32_t val)
            {
                m_data.resize(sizeof(uint32_t));
                m_type = REG_DWORD;
                std::memcpy(m_data.data(), &val, sizeof(uint32_t));
            }

            /// Set as REG_QWORD.
            /// @param val 64-bit unsigned integer
            void set_qword(uint64_t val)
            {
                m_data.resize(sizeof(uint64_t));
                m_type = REG_QWORD;
                std::memcpy(m_data.data(), &val, sizeof(uint64_t));
            }

            /// Set as REG_SZ (null-terminated Unicode string).
            /// @param val UTF-8 string to store
            void set_string(std::string_view val)
            {
                assign_from_utf8_string(val, REG_SZ);
            }

            /// Set as REG_EXPAND_SZ (expandable string with environment variables).
            /// @param val UTF-8 string to store
            void set_expanded_string(std::string_view val)
            {
                assign_from_utf8_string(val, REG_EXPAND_SZ);
            }

            /// Set as REG_MULTI_SZ (list of null-terminated strings).
            /// @param strings Vector of UTF-8 strings
            void set_multi_string(const std::vector<std::string>& strings)
            {
                m_data.clear();
                m_type = REG_MULTI_SZ;

                for (const auto& str : strings)
                {
                    const std::wstring wval = string::encode_as_utf16(str);
                    const auto* p = reinterpret_cast<const std::uint8_t*>(wval.c_str());
                    const size_t len_bytes = sizeof(wchar_t) * (wval.size() + 1); // include null terminator

                    m_data.insert(m_data.end(), p, p + len_bytes);
                }

                // Final double-null terminator
                m_data.push_back(0);
                m_data.push_back(0);
            }

            /// Set raw binary data with specified type.
            /// @param new_type Registry type constant
            /// @param data Raw byte data
            void set_binary_type(uint32_t new_type, const bytes& data)
            {
                m_data = data;
                m_type = new_type;

                // Convert certain types from raw bytes to native representation
                if (m_type == REG_EXPAND_SZ || m_type == REG_SZ)
                {
                    // Already in correct format (UTF-16LE with null terminator)
                    // Strip trailing nulls for consistency
                    while (m_data.size() >= 2 &&
                           m_data[m_data.size() - 1] == 0 &&
                           m_data[m_data.size() - 2] == 0)
                    {
                        m_data.pop_back();
                        m_data.pop_back();
                    }
                    // Re-add single null terminator
                    m_data.push_back(0);
                    m_data.push_back(0);
                }
                else if (m_type == REG_DWORD && m_data.size() == 4)
                {
                    // Already correct size
                }
                else if (m_type == REG_QWORD && m_data.size() == 8)
                {
                    // Already correct size
                }
            }

            /// Set escaped DWORD value (variable name stored as string).
            /// Used for $$VARIABLE$$ syntax in .REG files.
            /// @param variable_ref The variable reference string (e.g., "$$MYVAR$$")
            void set_escaped_dword_value(std::string_view variable_ref)
            {
                assign_from_utf8_string(variable_ref, REG_ESCAPED_DWORD);
            }

            /// Set escaped QWORD value (variable name stored as string).
            /// @param variable_ref The variable reference string
            void set_escaped_qword_value(std::string_view variable_ref)
            {
                assign_from_utf8_string(variable_ref, REG_ESCAPED_QWORD);
            }

            // =================================================================
            // Type Getters
            // =================================================================

            /// Get as DWORD.
            /// @param default_value Value to return if type doesn't match
            /// @return The DWORD value or default_value
            uint32_t get_dword(uint32_t default_value = 0) const
            {
                if (m_type != REG_DWORD)
                    return default_value;
                if (m_data.size() < sizeof(uint32_t))
                    return default_value;

                uint32_t result;
                std::memcpy(&result, m_data.data(), sizeof(uint32_t));
                return result;
            }

            /// Get as QWORD.
            /// @param default_value Value to return if type doesn't match
            /// @return The QWORD value or default_value
            uint64_t get_qword(uint64_t default_value = 0) const
            {
                if (m_type != REG_QWORD)
                    return default_value;
                if (m_data.size() < sizeof(uint64_t))
                    return default_value;

                uint64_t result;
                std::memcpy(&result, m_data.data(), sizeof(uint64_t));
                return result;
            }

            /// Get as string (for REG_SZ, REG_EXPAND_SZ).
            /// @param default_value Value to return if type doesn't match
            /// @return UTF-8 encoded string or default_value
            std::string get_string(std::string_view default_value = {}) const
            {
                if (!is_string_type(m_type))
                    return std::string{default_value};

                if (m_data.empty())
                    return std::string{default_value};

                // Data is UTF-16LE, convert to UTF-8
                const auto* wptr = reinterpret_cast<const wchar_t*>(m_data.data());
                size_t wlen = m_data.size() / sizeof(wchar_t);

                // Strip null terminator if present
                while (wlen > 0 && wptr[wlen - 1] == L'\0')
                    --wlen;

                return string::encode_as_utf8({wptr, wlen});
            }

            /// Get as multi-string (for REG_MULTI_SZ).
            /// @return Vector of UTF-8 strings
            std::vector<std::string> get_multi_string() const
            {
                std::vector<std::string> result;

                if (m_type != REG_MULTI_SZ || m_data.empty())
                    return result;

                const auto* wptr = reinterpret_cast<const wchar_t*>(m_data.data());
                const size_t total_wchars = m_data.size() / sizeof(wchar_t);

                size_t start = 0;
                for (size_t i = 0; i < total_wchars; ++i)
                {
                    if (wptr[i] == L'\0')
                    {
                        if (i > start)
                        {
                            result.push_back(string::encode_as_utf8({wptr + start, i - start}));
                        }
                        start = i + 1;

                        // Double null means end of list
                        if (i + 1 < total_wchars && wptr[i + 1] == L'\0')
                            break;
                    }
                }

                return result;
            }

            /// Get raw binary data.
            /// @return Reference to internal byte storage
            const bytes& get_binary() const
            {
                return m_data;
            }

            /// Convert value to byte array representation.
            /// Used for comparison and export operations.
            /// @return Byte representation of the value
            bytes as_byte_array() const
            {
                return m_data;
            }

        private:
            /// Store a UTF-8 string as UTF-16LE with null terminator.
            void assign_from_utf8_string(std::string_view val, uint32_t type)
            {
                const std::wstring wval = string::encode_as_utf16(val);
                const size_t len_bytes = sizeof(wchar_t) * (wval.size() + 1); // +1 for null terminator

                m_data.resize(len_bytes);
                m_type = type;
                std::memcpy(m_data.data(), wval.c_str(), len_bytes);
            }

        private:
            friend class key;
            friend class key_entry;
            friend class regfile_exporter;

            /// Value name (empty for default value).
            std::string m_name;

            /// Registry type (REG_SZ, REG_DWORD, etc.).
            uint32_t m_type;

            /// Raw value data (UTF-16LE for strings, native byte order for integers).
            bytes m_data;

            /// Flag indicating this value should be removed rather than added.
            bool m_remove_flag;
        };

    } // namespace registry
} // namespace pnq
