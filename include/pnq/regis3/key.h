#pragma once

/// @file pnq/regis3/key.h
/// @brief Live registry key access - RAII wrapper for Windows registry API

#include <pnq/regis3/types.h>
#include <pnq/regis3/value.h>
#include <pnq/regis3/iterators.h>
#include <pnq/pnq.h>
#include <pnq/win32/wstr_param.h>

#include <aclapi.h>
#include <sddl.h>

#include <unordered_map>

namespace pnq
{
    namespace regis3
    {
        // =====================================================================
        // Known Hives
        // =====================================================================

        /// Map of registry hive names to HKEY handles.
        /// Includes both long names (HKEY_LOCAL_MACHINE) and short aliases (HKLM).
        inline const std::unordered_map<std::string, HKEY> &known_hives()
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
        inline HKEY parse_hive(std::string_view full_path, std::string &relative_path)
        {
            relative_path.clear();

            for (const auto &[name, hkey] : known_hives())
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

            PNQ_LOG_WARN("'{}' is not a valid registry path", full_path);
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
                    m_has_write_permissions = true; // root keys have implicit access
                    return true;
                }

                HKEY hkey = nullptr;
                LSTATUS result = ::RegOpenKeyExW(hive, wstr_param{relative_path}, 0, KEY_READ, &hkey);

                if (result != ERROR_SUCCESS)
                {
                    PNQ_LOG_WARN("RegOpenKeyEx({}) for reading failed: {}", relative_path, result);
                    return false;
                }

                m_key = hkey;
                m_is_root_key = false;
                m_has_write_permissions = false;
                return true;
            }

            /// Set permissive DACL on a single registry key (Everyone: Full Control).
            /// Does NOT apply to existing subkeys. Use set_permissive_sddl_recursive for that.
            static bool set_permissive_sddl(const std::string &key_path)
            {
                std::string relative_path;
                HKEY hive = parse_hive(key_path, relative_path);
                if (!hive)
                    return false;

                return set_permissive_sddl_on_key(hive, relative_path);
            }

            /// Set permissive DACL on a registry key and all existing subkeys (Everyone: Full Control).
            static bool set_permissive_sddl_recursive(const std::string &key_path)
            {
                std::string relative_path;
                HKEY hive = parse_hive(key_path, relative_path);
                if (!hive)
                    return false;

                // Collect all subkey paths (includes the root path)
                std::vector<std::string> all_paths;
                all_paths.push_back(relative_path);
                if (!collect_subkeys_for_sddl(hive, relative_path, all_paths))
                    return false;

                // Apply DACL to each key
                bool all_succeeded = true;
                for (const auto &path : all_paths)
                {
                    if (!set_permissive_sddl_on_key(hive, path))
                        all_succeeded = false;
                }
                return all_succeeded;
            }

            /// Take ownership of a single registry key and grant full control to current user.
            static bool take_ownership(const std::string &key_path)
            {
                std::string relative_path;
                HKEY hive = parse_hive(key_path, relative_path);
                if (!hive)
                    return false;

                return take_ownership_and_grant_access(hive, relative_path);
            }

            /// Take ownership of a registry key and all subkeys, granting full control to current user.
            static bool take_ownership_recursive(const std::string &key_path)
            {
                std::string relative_path;
                HKEY hive = parse_hive(key_path, relative_path);
                if (!hive)
                    return false;

                // Collect all subkey paths first (requires read access)
                std::vector<std::string> all_paths;
                all_paths.push_back(relative_path);
                if (!collect_subkeys_for_sddl(hive, relative_path, all_paths))
                    return false;

                // Take ownership of each key
                bool all_succeeded = true;
                for (const auto &path : all_paths)
                {
                    if (!take_ownership_and_grant_access(hive, path))
                        all_succeeded = false;
                }
                return all_succeeded;
            }

        private:
            /// Apply permissive DACL to a single key by hive and relative path.
            static bool set_permissive_sddl_on_key(HKEY hive, const std::string &relative_path)
            {
                // Open with WRITE_DAC to modify the DACL
                HKEY hkey = nullptr;
                LSTATUS result = ::RegOpenKeyExW(hive, wstr_param{relative_path}, 0, WRITE_DAC, &hkey);
                if (result != ERROR_SUCCESS)
                {
                    PNQ_LOG_WIN_ERROR(result, "RegOpenKeyEx('{}') with WRITE_DAC failed", relative_path);
                    return false;
                }

                // SDDL: D:(A;OICI;GA;;;WD) = Allow Everyone Generic All, Object Inherit + Container Inherit
                PSECURITY_DESCRIPTOR pSD = nullptr;
                if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:(A;OICI;GA;;;WD)", SDDL_REVISION_1, &pSD, nullptr))
                {
                    PNQ_LOG_LAST_ERROR("ConvertStringSecurityDescriptorToSecurityDescriptorW() failed");
                    ::RegCloseKey(hkey);
                    return false;
                }

                BOOL daclPresent = FALSE, daclDefaulted = FALSE;
                PACL pDacl = nullptr;
                if (!::GetSecurityDescriptorDacl(pSD, &daclPresent, &pDacl, &daclDefaulted))
                {
                    PNQ_LOG_LAST_ERROR("GetSecurityDescriptorDacl() failed");
                    ::LocalFree(pSD);
                    ::RegCloseKey(hkey);
                    return false;
                }

                result = ::SetSecurityInfo(hkey, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION, nullptr, nullptr, pDacl, nullptr);
                ::LocalFree(pSD);
                ::RegCloseKey(hkey);

                if (result != ERROR_SUCCESS)
                {
                    PNQ_LOG_WIN_ERROR(result, "SetSecurityInfo() failed for '{}'", relative_path);
                    return false;
                }
                return true;
            }

            /// Recursively collect all subkey paths (breadth-first, for SDDL application).
            static bool collect_subkeys_for_sddl(HKEY hive, const std::string &relative_path, std::vector<std::string> &out_paths)
            {
                HKEY hkey = nullptr;
                LSTATUS result = ::RegOpenKeyExW(hive, wstr_param{relative_path}, 0, KEY_READ, &hkey);
                if (result != ERROR_SUCCESS)
                {
                    PNQ_LOG_WIN_ERROR(result, "RegOpenKeyEx('{}') failed during SDDL enumeration", relative_path);
                    return false;
                }

                std::vector<std::string> subkey_names;
                DWORD index = 0;
                wchar_t name_buffer[256];
                while (true)
                {
                    DWORD name_size = 256;
                    result = ::RegEnumKeyExW(hkey, index++, name_buffer, &name_size, nullptr, nullptr, nullptr, nullptr);
                    if (result == ERROR_NO_MORE_ITEMS)
                        break;
                    if (result != ERROR_SUCCESS)
                    {
                        PNQ_LOG_WIN_ERROR(result, "RegEnumKeyEx failed");
                        ::RegCloseKey(hkey);
                        return false;
                    }
                    subkey_names.push_back(string::encode_as_utf8({name_buffer, name_size}));
                }
                ::RegCloseKey(hkey);

                for (const auto &subkey_name : subkey_names)
                {
                    std::string subkey_path = relative_path.empty() ? subkey_name : (relative_path + "\\" + subkey_name);
                    out_paths.push_back(subkey_path);
                    if (!collect_subkeys_for_sddl(hive, subkey_path, out_paths))
                        return false;
                }
                return true;
            }

        public:

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
                LSTATUS result = ::RegOpenKeyExW(hive, wstr_param{relative_path}, 0, KEY_READ | KEY_WRITE | DELETE, &hkey);

                if (result == ERROR_FILE_NOT_FOUND)
                {
                    // Key doesn't exist - create it
                    DWORD disposition = 0;
                    result = ::RegCreateKeyExW(hive, wstr_param{relative_path}, 0, nullptr, 0, KEY_READ | KEY_WRITE | DELETE, nullptr, &hkey, &disposition);

                    if (result != ERROR_SUCCESS)
                    {
                        PNQ_LOG_WIN_ERROR(result, "RegCreateKeyEx('{}') failed", m_path);
                        return false;
                    }
                }
                else if (result != ERROR_SUCCESS)
                {
                    PNQ_LOG_WIN_ERROR(result, "RegOpenKeyEx('{}') for writing failed", m_path);
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
                        PNQ_LOG_WIN_ERROR(result, "RegCloseKey failed");
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
            bool get(std::string_view name, value &val) const
            {
                if (!const_cast<key *>(this)->open_for_reading())
                    return false;

                DWORD type = 0;
                bytes data;
                DWORD data_size = 1024;
                data.resize(data_size);

                while (data_size < 32768)
                {
                    DWORD actual_size = data_size;
                    LSTATUS result = ::RegQueryValueExW(m_key, wstr_param{name}, nullptr, &type, data.data(), &actual_size);

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
            bool set(std::string_view name, const value &val)
            {
                if (!open_for_writing())
                    return false;

                if (val.remove_flag())
                {
                    // Delete the value
                    LSTATUS result = ::RegDeleteValueW(m_key, wstr_param{name});
                    if (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND)
                        return true;

                    PNQ_LOG_WIN_ERROR(result, "RegDeleteValue('{}') failed", name);
                    return false;
                }
                else
                {
                    // Set the value
                    const bytes &data = val.get_binary();
                    LSTATUS result = ::RegSetValueExW(m_key, wstr_param{name}, 0, val.type(), data.data(), truncate_cast<DWORD>(data.size()));

                    if (result == ERROR_SUCCESS)
                        return true;

                    PNQ_LOG_WIN_ERROR(result, "RegSetValueEx('{}') failed", name);
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

            /// Get a QWORD value.
            uint64_t get_qword(std::string_view name, uint64_t default_value = 0) const
            {
                value v;
                if (get(name, v))
                {
                    return v.get_qword(default_value);
                }
                return default_value;
            }

            /// Get a multi-string value.
            std::vector<std::string> get_multi_string(std::string_view name) const
            {
                value v;
                if (get(name, v))
                {
                    return v.get_multi_string();
                }
                return {};
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

            /// Set a QWORD value.
            bool set_qword(std::string_view name, uint64_t val)
            {
                value v{name};
                v.set_qword(val);
                return set(name, v);
            }

            /// Set an expand string value (REG_EXPAND_SZ).
            bool set_expand_string(std::string_view name, std::string_view val)
            {
                value v{name};
                v.set_expanded_string(val);
                return set(name, v);
            }

            /// Set a multi-string value (REG_MULTI_SZ).
            bool set_multi_string(std::string_view name, const std::vector<std::string> &val)
            {
                value v{name};
                v.set_multi_string(val);
                return set(name, v);
            }

            /// Delete a value from this key.
            bool delete_value(std::string_view name)
            {
                if (!open_for_writing())
                    return false;

                LSTATUS result = ::RegDeleteValueW(m_key, wstr_param{name});
                if (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND)
                    return true;

                PNQ_LOG_WIN_ERROR(result, "RegDeleteValue('{}') failed", name);
                return false;
            }

            // =================================================================
            // Subkey Operations
            // =================================================================

            /// Delete a subkey and all its contents using bottom-up approach.
            /// @param name Subkey name relative to this key
            /// @param force If true, attempt to take ownership and grant permissions on access denied
            /// @return true if successful
            bool delete_subkey(std::string_view name, bool force = false)
            {
                if (!open_for_writing())
                    return false;

                std::string full_subkey_path = m_path + "\\" + std::string{name};
                return delete_key_tree(full_subkey_path, force);
            }

            /// Delete a registry key recursively by full path using bottom-up approach.
            /// @param path Full registry path
            /// @param force If true, attempt to take ownership and grant permissions on access denied
            /// @return true if successful
            static bool delete_recursive(std::string_view path, bool force = false)
            {
                return delete_key_tree(std::string{path}, force);
            }

        private:
            /// Recursively collect all subkey paths depth-first.
            /// Returns paths from deepest to shallowest (ready for bottom-up deletion).
            static bool collect_subkeys_depth_first(HKEY hive, const std::string &relative_path, std::vector<std::string> &out_paths)
            {
                HKEY hkey = nullptr;
                LSTATUS result = ::RegOpenKeyExW(hive, wstr_param{relative_path}, 0, KEY_READ, &hkey);
                if (result == ERROR_FILE_NOT_FOUND)
                    return true; // Key doesn't exist, nothing to collect
                if (result != ERROR_SUCCESS)
                {
                    PNQ_LOG_WIN_ERROR(result, "RegOpenKeyEx('{}') failed during enumeration", relative_path);
                    return false;
                }

                // Enumerate subkeys
                std::vector<std::string> subkey_names;
                DWORD index = 0;
                wchar_t name_buffer[256];
                while (true)
                {
                    DWORD name_size = 256;
                    result = ::RegEnumKeyExW(hkey, index++, name_buffer, &name_size, nullptr, nullptr, nullptr, nullptr);
                    if (result == ERROR_NO_MORE_ITEMS)
                        break;
                    if (result != ERROR_SUCCESS)
                    {
                        PNQ_LOG_WIN_ERROR(result, "RegEnumKeyEx failed");
                        ::RegCloseKey(hkey);
                        return false;
                    }
                    subkey_names.push_back(string::encode_as_utf8({name_buffer, name_size}));
                }
                ::RegCloseKey(hkey);

                // Recurse into each subkey first (depth-first)
                for (const auto &subkey_name : subkey_names)
                {
                    std::string subkey_path = relative_path.empty() ? subkey_name : (relative_path + "\\" + subkey_name);
                    if (!collect_subkeys_depth_first(hive, subkey_path, out_paths))
                        return false;
                }

                // Add this path after children (so it's deleted after them)
                if (!relative_path.empty())
                    out_paths.push_back(relative_path);

                return true;
            }

            /// Enable a privilege on the current process token.
            static bool enable_privilege(LPCWSTR privilege_name)
            {
                HANDLE token = nullptr;
                if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
                    return false;

                TOKEN_PRIVILEGES tp = {};
                if (!::LookupPrivilegeValueW(nullptr, privilege_name, &tp.Privileges[0].Luid))
                {
                    ::CloseHandle(token);
                    return false;
                }

                tp.PrivilegeCount = 1;
                tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
                BOOL ok = ::AdjustTokenPrivileges(token, FALSE, &tp, 0, nullptr, nullptr);
                DWORD err = ::GetLastError();
                ::CloseHandle(token);

                return ok && err == ERROR_SUCCESS;
            }

            /// Get current user's SID. Caller must free with LocalFree.
            static PSID get_current_user_sid()
            {
                HANDLE token = nullptr;
                if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token))
                    return nullptr;

                DWORD size = 0;
                ::GetTokenInformation(token, TokenUser, nullptr, 0, &size);
                if (size == 0)
                {
                    ::CloseHandle(token);
                    return nullptr;
                }

                std::vector<BYTE> buffer(size);
                if (!::GetTokenInformation(token, TokenUser, buffer.data(), size, &size))
                {
                    ::CloseHandle(token);
                    return nullptr;
                }
                ::CloseHandle(token);

                PTOKEN_USER user_info = reinterpret_cast<PTOKEN_USER>(buffer.data());
                DWORD sid_len = ::GetLengthSid(user_info->User.Sid);
                PSID sid_copy = static_cast<PSID>(::LocalAlloc(LMEM_FIXED, sid_len));
                if (sid_copy && !::CopySid(sid_len, sid_copy, user_info->User.Sid))
                {
                    ::LocalFree(sid_copy);
                    return nullptr;
                }
                return sid_copy;
            }

            /// Take ownership of a key and grant full control to current user.
            /// Requires SeTakeOwnershipPrivilege (typically admin).
            /// @return true if successful
            static bool take_ownership_and_grant_access(HKEY hive, const std::string &relative_path)
            {
                // Enable privileges needed for taking ownership
                if (!enable_privilege(SE_TAKE_OWNERSHIP_NAME))
                    PNQ_LOG_WARN("Failed to enable SeTakeOwnershipPrivilege - may not be running elevated");
                enable_privilege(SE_RESTORE_NAME);
                enable_privilege(SE_BACKUP_NAME);

                // Get current user's SID
                PSID user_sid = get_current_user_sid();
                if (!user_sid)
                {
                    PNQ_LOG_LAST_ERROR("Failed to get current user SID");
                    return false;
                }

                // Step 1: Open with WRITE_OWNER only (works with SeTakeOwnershipPrivilege)
                // Try REG_OPTION_BACKUP_RESTORE first (works with SE_RESTORE_NAME), fall back to normal open
                HKEY hkey = nullptr;
                LSTATUS result = ::RegOpenKeyExW(hive, wstr_param{relative_path}, REG_OPTION_BACKUP_RESTORE, WRITE_OWNER, &hkey);
                if (result != ERROR_SUCCESS)
                {
                    // Fall back to normal open
                    result = ::RegOpenKeyExW(hive, wstr_param{relative_path}, 0, WRITE_OWNER, &hkey);
                }
                if (result != ERROR_SUCCESS)
                {
                    PNQ_LOG_WIN_ERROR(result, "RegOpenKeyEx('{}') with WRITE_OWNER failed", relative_path);
                    ::LocalFree(user_sid);
                    return false;
                }

                // Step 2: Take ownership
                result = ::SetSecurityInfo(hkey, SE_REGISTRY_KEY, OWNER_SECURITY_INFORMATION, user_sid, nullptr, nullptr, nullptr);
                ::RegCloseKey(hkey);
                if (result != ERROR_SUCCESS)
                {
                    PNQ_LOG_WIN_ERROR(result, "SetSecurityInfo (ownership) failed for '{}'", relative_path);
                    ::LocalFree(user_sid);
                    return false;
                }
                PNQ_LOG_INFO("Took ownership of '{}'", relative_path);

                // Step 3: Re-open with WRITE_DAC (now that we own it)
                result = ::RegOpenKeyExW(hive, wstr_param{relative_path}, 0, WRITE_DAC, &hkey);
                if (result != ERROR_SUCCESS)
                {
                    PNQ_LOG_WIN_ERROR(result, "RegOpenKeyEx('{}') with WRITE_DAC failed after taking ownership", relative_path);
                    ::LocalFree(user_sid);
                    return false;
                }

                // Step 4: Build DACL granting full control to current user
                EXPLICIT_ACCESSW ea = {};
                ea.grfAccessPermissions = KEY_ALL_ACCESS;
                ea.grfAccessMode = SET_ACCESS;
                ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
                ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
                ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
                ea.Trustee.ptstrName = reinterpret_cast<LPWSTR>(user_sid);

                PACL new_dacl = nullptr;
                result = ::SetEntriesInAclW(1, &ea, nullptr, &new_dacl);
                if (result != ERROR_SUCCESS)
                {
                    PNQ_LOG_WIN_ERROR(result, "SetEntriesInAcl failed");
                    ::RegCloseKey(hkey);
                    ::LocalFree(user_sid);
                    return false;
                }

                // Step 5: Apply the new DACL
                result = ::SetSecurityInfo(hkey, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION, nullptr, nullptr, new_dacl, nullptr);
                ::LocalFree(new_dacl);
                ::LocalFree(user_sid);
                ::RegCloseKey(hkey);

                if (result != ERROR_SUCCESS)
                {
                    PNQ_LOG_WIN_ERROR(result, "SetSecurityInfo (DACL) failed for '{}'", relative_path);
                    return false;
                }
                PNQ_LOG_INFO("Granted full control on '{}'", relative_path);

                return true;
            }

            /// Delete a single key (no subkeys). Optionally forces access.
            static bool delete_single_key(HKEY hive, const std::string &relative_path, bool force)
            {
                LSTATUS result = ::RegDeleteKeyW(hive, wstr_param{relative_path});
                if (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND)
                    return true;

                if (force && result == ERROR_ACCESS_DENIED)
                {
                    PNQ_LOG_INFO("Access denied for '{}', attempting to take ownership", relative_path);
                    // Try taking ownership and granting access
                    if (take_ownership_and_grant_access(hive, relative_path))
                    {
                        result = ::RegDeleteKeyW(hive, wstr_param{relative_path});
                        if (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND)
                        {
                            PNQ_LOG_INFO("Force-deleted '{}'", relative_path);
                            return true;
                        }
                    }
                    PNQ_LOG_WIN_ERROR(result, "RegDeleteKey('{}') failed even after taking ownership", relative_path);
                }
                else
                {
                    PNQ_LOG_WIN_ERROR(result, "RegDeleteKey('{}') failed", relative_path);
                }
                return false;
            }

            /// Delete a key tree using bottom-up approach.
            static bool delete_key_tree(const std::string &full_path, bool force)
            {
                std::string relative_path;
                HKEY hive = parse_hive(full_path, relative_path);
                if (!hive || relative_path.empty())
                {
                    PNQ_LOG_WARN("Cannot delete hive root or invalid path: {}", full_path);
                    return false;
                }

                // Collect all subkeys depth-first
                std::vector<std::string> paths_to_delete;
                if (!collect_subkeys_depth_first(hive, relative_path, paths_to_delete))
                {
                    // Even if enumeration fails, try to delete what we can
                    PNQ_LOG_WARN("Enumeration failed for '{}', attempting direct deletion", full_path);
                }

                // Delete from deepest to shallowest
                bool all_succeeded = true;
                for (const auto &path : paths_to_delete)
                {
                    if (!delete_single_key(hive, path, force))
                        all_succeeded = false;
                }

                return all_succeeded;
            }

        public:
            // =================================================================
            // Enumeration
            // =================================================================

            /// Enumerate all values in this key.
            /// @return Enumerator for range-based for loops
            value_enumerator enum_values() const
            {
                const_cast<key *>(this)->open_for_reading();
                return value_enumerator{m_key};
            }

            /// Enumerate all subkeys of this key.
            /// @return Enumerator for range-based for loops
            key_enumerator enum_keys() const
            {
                const_cast<key *>(this)->open_for_reading();
                return key_enumerator{m_key, m_path};
            }

            // =================================================================
            // Accessors
            // =================================================================

            /// Get the full registry path.
            const std::string &path() const
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

    } // namespace regis3
} // namespace pnq
