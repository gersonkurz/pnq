#pragma once

/// @file pnq/registry/key.h
/// @brief Live registry key access - RAII wrapper for Windows registry API

#include <pnq/registry/types.h>
#include <pnq/registry/value.h>
#include <pnq/registry/iterators.h>
#include <pnq/string.h>
#include <pnq/logging.h>
#include <pnq/win32/wstr_param.h>
#include <pnq/pnq.h>

#include <unordered_map>
#include <format>

namespace pnq
{
    namespace registry
    {
        // =====================================================================
        // Known Hives
        // =====================================================================

        /// Map of registry hive names to HKEY handles.
        /// Includes both long names (HKEY_LOCAL_MACHINE) and short aliases (HKLM).
        inline const std::unordered_map<std::string, HKEY>& known_hives()
        {
            static const std::unordered_map<std::string, HKEY> hives{
                {"HKEY_CLASSES_ROOT", HKEY_CLASSES_ROOT},
                {"HKEY_CURRENT_USER", HKEY_CURRENT_USER},
                {"HKEY_USERS", HKEY_USERS},
                {"HKEY_LOCAL_MACHINE", HKEY_LOCAL_MACHINE},
                {"HKEY_PERFORMANCE_DATA", HKEY_PERFORMANCE_DATA},
                {"HKEY_PERFORMANCE_TEXT", HKEY_PERFORMANCE_TEXT},
                {"HKEY_PERFORMANCE_NLSTEXT", HKEY_PERFORMANCE_NLSTEXT},
                {"HKEY_CURRENT_USER_LOCAL_SETTINGS", HKEY_CURRENT_USER_LOCAL_SETTINGS},
                {"HKEY_CURRENT_CONFIG", HKEY_CURRENT_CONFIG},
                {"HKEY_DYN_DATA", HKEY_DYN_DATA},
                // Short aliases
                {"HKCR", HKEY_CLASSES_ROOT},
                {"HKCU", HKEY_CURRENT_USER},
                {"HKLM", HKEY_LOCAL_MACHINE},
                {"HKU", HKEY_USERS},
                {"HKCC", HKEY_CURRENT_CONFIG},
            };
            return hives;
        }

        /// Parse a registry path and return the hive handle and relative path.
        /// @param full_path Full registry path like "HKEY_LOCAL_MACHINE\\SOFTWARE\\MyApp"
        /// @param relative_path [out] Path relative to hive (e.g., "SOFTWARE\\MyApp")
        /// @return HKEY for the hive, or nullptr if not found
        inline HKEY parse_hive(std::string_view full_path, std::string& relative_path)
        {
            relative_path.clear();

            for (const auto& [name, hkey] : known_hives())
            {
                // Check for "HIVE\path"
                if (full_path.size() > name.size() && full_path[name.size()] == '\\')
                {
                    if (string::starts_with_nocase(full_path, name))
                    {
                        relative_path = full_path.substr(name.size() + 1);
                        return hkey;
                    }
                }
                // Check for exact match (hive root only)
                if (string::equals_nocase(full_path, name))
                {
                    relative_path.clear();
                    return hkey;
                }
            }

            spdlog::warn("'{}' is not a valid registry path", full_path);
            return nullptr;
        }

        // =====================================================================
        // Key Class
        // =====================================================================

        /// RAII wrapper for a Windows registry key handle.
        ///
        /// Provides read/write access to registry values and subkey enumeration.
        /// Automatically closes the handle on destruction.
        class key final
        {
        public:
            /// Construct a key wrapper for the given registry path.
            /// @param path Full registry path (e.g., "HKEY_LOCAL_MACHINE\\SOFTWARE\\MyApp")
            explicit key(std::string_view path)
                : m_path{path},
                  m_key{nullptr},
                  m_is_root_key{false},
                  m_has_write_permissions{false}
            {
            }

            ~key()
            {
                close();
            }

            PNQ_DECLARE_NON_COPYABLE(key)

            // =================================================================
            // Open/Close
            // =================================================================

            /// Open the key for reading.
            /// @return true if successful
            bool open_for_reading()
            {
                if (m_key != nullptr)
                    return true;

                std::string relative_path;
                HKEY hive = parse_hive(m_path, relative_path);
                if (!hive)
                    return false;

                // If this is a root key (no relative path)
                if (relative_path.empty())
                {
                    m_key = hive;
                    m_is_root_key = true;
                    m_has_write_permissions = true;  // root keys have implicit access
                    return true;
                }

                HKEY hkey = nullptr;
                LSTATUS result = ::RegOpenKeyExW(
                    hive,
                    wstr_param{relative_path},
                    0,
                    KEY_READ,
                    &hkey);

                if (result != ERROR_SUCCESS)
                {
                    spdlog::warn("RegOpenKeyEx({}) for reading failed: {}", relative_path, result);
                    return false;
                }

                m_key = hkey;
                m_is_root_key = false;
                m_has_write_permissions = false;
                return true;
            }

            /// Open the key for writing. Creates the key if it doesn't exist.
            /// @return true if successful
            bool open_for_writing()
            {
                if (m_has_write_permissions)
                    return true;

                std::string relative_path;
                HKEY hive = parse_hive(m_path, relative_path);
                if (!hive)
                    return false;

                // Root key
                if (relative_path.empty())
                {
                    m_key = hive;
                    m_is_root_key = true;
                    m_has_write_permissions = true;
                    return true;
                }

                HKEY hkey = nullptr;
                LSTATUS result = ::RegOpenKeyExW(
                    hive,
                    wstr_param{relative_path},
                    0,
                    KEY_READ | KEY_WRITE | DELETE,
                    &hkey);

                if (result == ERROR_FILE_NOT_FOUND)
                {
                    // Key doesn't exist - create it
                    DWORD disposition = 0;
                    result = ::RegCreateKeyExW(
                        hive,
                        wstr_param{relative_path},
                        0,
                        nullptr,
                        0,
                        KEY_READ | KEY_WRITE | DELETE,
                        nullptr,
                        &hkey,
                        &disposition);

                    if (result != ERROR_SUCCESS)
                    {
                        logging::report_windows_error(
                            PNQ_FUNCTION_CONTEXT,
                            result,
                            std::format("RegCreateKeyEx({}) failed", m_path));
                        return false;
                    }
                }
                else if (result != ERROR_SUCCESS)
                {
                    logging::report_windows_error(
                        PNQ_FUNCTION_CONTEXT,
                        result,
                        std::format("RegOpenKeyEx({}) for writing failed", m_path));
                    return false;
                }

                // Close existing handle if we had read-only access
                if (m_key && !m_is_root_key)
                {
                    ::RegCloseKey(m_key);
                }

                m_key = hkey;
                m_is_root_key = false;
                m_has_write_permissions = true;
                return true;
            }

            /// Close the key handle.
            void close()
            {
                if (!m_is_root_key && m_key)
                {
                    LSTATUS result = ::RegCloseKey(m_key);
                    if (result != ERROR_SUCCESS)
                    {
                        logging::report_windows_error(
                            PNQ_FUNCTION_CONTEXT,
                            result,
                            "RegCloseKey() failed");
                    }
                }
                m_key = nullptr;
                m_has_write_permissions = false;
            }

            /// Check if the key is currently open.
            bool is_open() const
            {
                return m_key != nullptr;
            }

            // =================================================================
            // Value Read/Write
            // =================================================================

            /// Read a value from this key.
            /// @param name Value name (empty for default value)
            /// @param val [out] Value to populate
            /// @return true if successful
            bool get(std::string_view name, value& val) const
            {
                if (!const_cast<key*>(this)->open_for_reading())
                    return false;

                DWORD type = 0;
                bytes data;
                DWORD data_size = 1024;
                data.resize(data_size);

                while (data_size < 32768)
                {
                    DWORD actual_size = data_size;
                    LSTATUS result = ::RegQueryValueExW(
                        m_key,
                        wstr_param{name},
                        nullptr,
                        &type,
                        data.data(),
                        &actual_size);

                    if (result == ERROR_SUCCESS)
                    {
                        val = value{name, type, data, actual_size};
                        return true;
                    }

                    if (result == ERROR_MORE_DATA)
                    {
                        data_size *= 2;
                        data.resize(data_size);
                    }
                    else
                    {
                        return false;
                    }
                }
                return false;
            }

            /// Write a value to this key.
            /// @param name Value name (empty for default value)
            /// @param val Value to write
            /// @return true if successful
            bool set(std::string_view name, const value& val)
            {
                if (!open_for_writing())
                    return false;

                if (val.remove_flag())
                {
                    // Delete the value
                    LSTATUS result = ::RegDeleteValueW(m_key, wstr_param{name});
                    if (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND)
                        return true;

                    logging::report_windows_error(
                        PNQ_FUNCTION_CONTEXT,
                        result,
                        std::format("RegDeleteValue({}) failed", name));
                    return false;
                }
                else
                {
                    // Set the value
                    const bytes& data = val.get_binary();
                    LSTATUS result = ::RegSetValueExW(
                        m_key,
                        wstr_param{name},
                        0,
                        val.type(),
                        data.data(),
                        truncate_cast<DWORD>(data.size()));

                    if (result == ERROR_SUCCESS)
                        return true;

                    logging::report_windows_error(
                        PNQ_FUNCTION_CONTEXT,
                        result,
                        std::format("RegSetValueEx({}) failed", name));
                    return false;
                }
            }

            // =================================================================
            // Convenience Methods
            // =================================================================

            /// Get a string value.
            std::string get_string(std::string_view name, std::string_view default_value = {}) const
            {
                value v;
                if (get(name, v))
                {
                    return v.get_string(default_value);
                }
                return std::string{default_value};
            }

            /// Get a DWORD value.
            uint32_t get_dword(std::string_view name, uint32_t default_value = 0) const
            {
                value v;
                if (get(name, v))
                {
                    return v.get_dword(default_value);
                }
                return default_value;
            }

            /// Set a string value (REG_SZ).
            bool set_string(std::string_view name, std::string_view val)
            {
                value v{name};
                v.set_string(val);
                return set(name, v);
            }

            /// Set a DWORD value.
            bool set_dword(std::string_view name, uint32_t val)
            {
                value v{name};
                v.set_dword(val);
                return set(name, v);
            }

            // =================================================================
            // Subkey Operations
            // =================================================================

            /// Delete a subkey and all its contents.
            /// @param name Subkey name relative to this key
            /// @return true if successful
            bool delete_subkey(std::string_view name)
            {
                if (!open_for_writing())
                    return false;

                LSTATUS result = ::RegDeleteTreeW(m_key, wstr_param{name});
                if (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND)
                    return true;

                logging::report_windows_error(
                    PNQ_FUNCTION_CONTEXT,
                    result,
                    std::format("RegDeleteTree({}) failed", name));
                return false;
            }

            /// Delete a registry key recursively by full path.
            /// @param path Full registry path
            /// @return true if successful
            static bool delete_recursive(std::string_view path)
            {
                auto [parent, child] = string::split_at_last_occurence(path, '\\');
                if (child.empty())
                    return false;

                return key{parent}.delete_subkey(child);
            }

            // =================================================================
            // Enumeration
            // =================================================================

            /// Enumerate all values in this key.
            /// @return Enumerator for range-based for loops
            value_enumerator enum_values() const
            {
                const_cast<key*>(this)->open_for_reading();
                return value_enumerator{m_key};
            }

            /// Enumerate all subkeys of this key.
            /// @return Enumerator for range-based for loops
            key_enumerator enum_keys() const
            {
                const_cast<key*>(this)->open_for_reading();
                return key_enumerator{m_key, m_path};
            }

            // =================================================================
            // Accessors
            // =================================================================

            /// Get the full registry path.
            const std::string& path() const
            {
                return m_path;
            }

            /// Get just the key name (last component of path).
            std::string name() const
            {
                return std::string{string::split_at_last_occurence(m_path, '\\').second};
            }

            /// Get the underlying HKEY handle.
            HKEY handle() const
            {
                return m_key;
            }

        private:
            std::string m_path;
            HKEY m_key;
            bool m_is_root_key;
            bool m_has_write_permissions;
        };

    } // namespace registry
} // namespace pnq
