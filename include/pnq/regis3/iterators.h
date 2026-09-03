#pragma once

/// @file pnq/regis3/iterators.h
/// @brief Registry key and value iterators for enumerating live registry content

#include <pnq/regis3/types.h>
#include <pnq/regis3/value.h>
#include <pnq/string.h>
#include <pnq/pnq.h>

#include <vector>
#include <string>
#include <cassert>

namespace pnq
{
    namespace regis3
    {
        // =====================================================================
        // Value Iterator
        // =====================================================================

        /// Iterator for enumerating registry values in a key.
        /// Uses lazy evaluation - values are read on demand during iteration.
        class value_iterator final
        {
        public:
            /// Construct an iterator at the given index.
            /// @param hkey Registry key handle (0 marks end iterator)
            /// @param status Where to record why enumeration stopped (may be nullptr)
            explicit value_iterator(HKEY hkey, LSTATUS* status = nullptr)
                : m_key{hkey},
                  m_index{0},
                  m_status{status}
            {
            }

            /// Inequality comparison (for range-based for).
            bool operator!=(const value_iterator& other) const
            {
                if (m_key)
                {
                    // Lazy read - only fetch data when actually iterating
                    const_cast<value_iterator*>(this)->ensure_valid_data();
                }
                return (m_key != other.m_key) || (m_index != other.m_index);
            }

            /// Dereference to get current value.
            const value& operator*() const
            {
                return m_value;
            }

            /// Pre-increment to move to next value.
            const value_iterator& operator++()
            {
                if (m_key)
                {
                    ++m_index;
                }
                return *this;
            }

        private:
            /// Ensure current value data is loaded.
            void ensure_valid_data()
            {
                assert(m_key != 0);
                if (!read_value())
                {
                    m_key = 0;
                    m_index = 0;
                }
            }

            /// Read the current value from registry.
            /// @return true if successful, false if no more values
            bool read_value()
            {
                DWORD name_len = truncate_cast<DWORD>(m_wname.size());
                DWORD type = 0;
                DWORD data_len = truncate_cast<DWORD>(m_data.size());

                // Initialize buffers if needed
                if (name_len == 0)
                {
                    name_len = 256;
                    m_wname.resize(name_len);
                }
                if (data_len == 0)
                {
                    data_len = 1024;
                    m_data.resize(data_len);
                }

                while (name_len < 32768)
                {
                    DWORD actual_name_len = name_len;
                    DWORD actual_data_len = data_len;

                    LSTATUS result = ::RegEnumValueW(
                        m_key,
                        m_index,
                        m_wname.data(),
                        &actual_name_len,
                        nullptr,
                        &type,
                        m_data.data(),
                        &actual_data_len);

                    if (result == ERROR_SUCCESS)
                    {
                        std::string name = string::encode_as_utf8({m_wname.data(), actual_name_len});
                        m_value = value{name, type, m_data, actual_data_len};
                        return true;
                    }

                    if (result == ERROR_MORE_DATA)
                    {
                        // Grow buffers and retry
                        name_len *= 2;
                        data_len *= 2;
                        m_wname.resize(name_len);
                        m_data.resize(data_len);
                    }
                    else
                    {
                        // ERROR_NO_MORE_ITEMS is the normal end, anything else truncated us
                        record_stop(result);
                        return false;
                    }
                }

                record_stop(ERROR_MORE_DATA); // name outgrew the maximum buffer
                return false;
            }

            /// Remember why enumeration stopped, and log it unless it was the natural end.
            void record_stop(LSTATUS result)
            {
                if (m_status)
                    *m_status = result;
                if (result != ERROR_NO_MORE_ITEMS)
                    PNQ_LOG_WIN_ERROR(result, "RegEnumValue at index {} failed", m_index);
            }

        private:
            HKEY m_key;
            DWORD m_index;
            value m_value;
            std::vector<wchar_t> m_wname;
            bytes m_data;
            LSTATUS* m_status;
        };

        /// Enumerator wrapper providing begin()/end() for value iteration.
        /// Non-copyable: the iterators point at m_status, so the enumerator must outlive them.
        class value_enumerator final
        {
        public:
            /// Construct enumerator for a key handle.
            explicit value_enumerator(HKEY hkey)
                : m_key{hkey},
                  m_status{ERROR_NO_MORE_ITEMS}
            {
            }

            PNQ_DECLARE_NON_COPYABLE(value_enumerator)

            /// Get iterator at beginning.
            value_iterator begin() const
            {
                return value_iterator{m_key, &m_status};
            }

            /// Get end iterator.
            value_iterator end() const
            {
                return value_iterator{0};
            }

            /// Why enumeration stopped. ERROR_NO_MORE_ITEMS means it ran to its natural end,
            /// anything else means it was truncated and the loop saw only a subset.
            /// Only meaningful once the loop has finished.
            LSTATUS last_status() const
            {
                return m_status;
            }

        private:
            HKEY m_key;
            mutable LSTATUS m_status;
        };

        // =====================================================================
        // Key Iterator
        // =====================================================================

        /// Iterator for enumerating subkeys in a registry key.
        /// Uses lazy evaluation - subkey names are read on demand.
        class key_iterator final
        {
        public:
            /// Construct an iterator.
            /// @param hkey Registry key handle (0 marks end iterator)
            /// @param parent_name Full path of parent key (for constructing subkey paths)
            /// @param status Where to record why enumeration stopped (may be nullptr)
            key_iterator(HKEY hkey, std::string_view parent_name, LSTATUS* status = nullptr)
                : m_key{hkey},
                  m_index{0},
                  m_parent_name{parent_name},
                  m_status{status}
            {
            }

            /// Inequality comparison.
            bool operator!=(const key_iterator& other) const
            {
                if (m_key)
                {
                    const_cast<key_iterator*>(this)->ensure_valid_data();
                }
                return (m_key != other.m_key) || (m_index != other.m_index);
            }

            /// Dereference to get full path of current subkey.
            const std::string& operator*() const
            {
                return m_path;
            }

            /// Pre-increment to move to next subkey.
            const key_iterator& operator++()
            {
                ++m_index;
                return *this;
            }

        private:
            void ensure_valid_data()
            {
                assert(m_key != 0);
                if (!read_key_name())
                {
                    m_key = 0;
                    m_index = 0;
                }
            }

            /// Read the current subkey name from registry.
            bool read_key_name()
            {
                DWORD name_len = truncate_cast<DWORD>(m_wname.size());
                DWORD class_len = truncate_cast<DWORD>(m_wclass.size());

                if (name_len == 0)
                {
                    name_len = 256;
                    m_wname.resize(name_len);
                }
                if (class_len == 0)
                {
                    class_len = 1024;
                    m_wclass.resize(class_len);
                }

                FILETIME ignored;

                while (name_len < 32768)
                {
                    DWORD actual_name_len = name_len;
                    DWORD actual_class_len = class_len;

                    LSTATUS result = ::RegEnumKeyExW(
                        m_key,
                        m_index,
                        m_wname.data(),
                        &actual_name_len,
                        nullptr,
                        m_wclass.data(),
                        &actual_class_len,
                        &ignored);

                    if (result == ERROR_SUCCESS)
                    {
                        std::string name = string::encode_as_utf8({m_wname.data(), actual_name_len});
                        m_path = std::string{m_parent_name} + "\\" + name;
                        return true;
                    }

                    if (result == ERROR_MORE_DATA)
                    {
                        name_len *= 2;
                        class_len *= 2;
                        m_wname.resize(name_len);
                        m_wclass.resize(class_len);
                    }
                    else
                    {
                        // ERROR_NO_MORE_ITEMS is the normal end, anything else truncated us
                        record_stop(result);
                        return false;
                    }
                }

                record_stop(ERROR_MORE_DATA); // name outgrew the maximum buffer
                return false;
            }

            /// Remember why enumeration stopped, and log it unless it was the natural end.
            void record_stop(LSTATUS result)
            {
                if (m_status)
                    *m_status = result;
                if (result != ERROR_NO_MORE_ITEMS)
                    PNQ_LOG_WIN_ERROR(result, "RegEnumKeyEx({}) at index {} failed", m_parent_name, m_index);
            }

        private:
            HKEY m_key;
            DWORD m_index;
            std::string m_parent_name;
            std::string m_path;
            std::vector<wchar_t> m_wname;
            std::vector<wchar_t> m_wclass;
            LSTATUS* m_status;
        };

        /// Enumerator wrapper providing begin()/end() for subkey iteration.
        /// Non-copyable: the iterators point at m_status, so the enumerator must outlive them.
        class key_enumerator final
        {
        public:
            /// Construct enumerator for a key handle.
            /// @param hkey Registry key handle
            /// @param parent_name Full path of the parent key
            key_enumerator(HKEY hkey, std::string_view parent_name)
                : m_key{hkey},
                  m_parent_name{parent_name},
                  m_status{ERROR_NO_MORE_ITEMS}
            {
            }

            PNQ_DECLARE_NON_COPYABLE(key_enumerator)

            key_iterator begin() const
            {
                return key_iterator{m_key, m_parent_name, &m_status};
            }

            key_iterator end() const
            {
                return key_iterator{0, m_parent_name};
            }

            /// Why enumeration stopped. ERROR_NO_MORE_ITEMS means it ran to its natural end,
            /// anything else means it was truncated and the loop saw only a subset.
            /// Only meaningful once the loop has finished.
            LSTATUS last_status() const
            {
                return m_status;
            }

        private:
            HKEY m_key;
            std::string m_parent_name;
            mutable LSTATUS m_status;
        };

    } // namespace regis3
} // namespace pnq
