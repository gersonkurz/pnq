#include <ngbtools/ngbtools.h>
#include <ngbtools/win32/registry/key.h>
#include <ngbtools/win32/wstr_param.h>
#include <ngbtools/logging.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            static std::unordered_map<std::string, HKEY> known_hives
            {
                { "HKEY_CLASSES_ROOT", HKEY_CLASSES_ROOT },
                { "HKEY_CURRENT_USER", HKEY_CURRENT_USER },
                { "HKEY_USERS", HKEY_USERS },
                { "HKEY_LOCAL_MACHINE", HKEY_LOCAL_MACHINE },
                { "HKEY_PERFORMANCE_DATA", HKEY_PERFORMANCE_DATA },
                { "HKEY_PERFORMANCE_TEXT", HKEY_PERFORMANCE_TEXT },
                { "HKEY_PERFORMANCE_NLSTEXT", HKEY_PERFORMANCE_NLSTEXT },
                { "HKEY_CURRENT_USER_LOCAL_SETTINGS", HKEY_CURRENT_USER_LOCAL_SETTINGS },
                { "HKEY_CURRENT_CONFIG", HKEY_CURRENT_CONFIG },
                { "HKEY_DYN_DATA", HKEY_DYN_DATA },
                { "HKCR", HKEY_CLASSES_ROOT },
                { "HKCU", HKEY_CURRENT_USER },
                { "HKLM", HKEY_LOCAL_MACHINE },
                { "HKU", HKEY_USERS },
            };

            void key::close()
            {
                if (!m_is_root_key)
                {
                    if (m_key)
                    {
                        LONG result = ::RegCloseKey(m_key);
                        if(result != 0)
                        {
                            logging::report_windows_error(result, NGBT_FUNCTION_CONTEXT, "RegCloseKey() failed");
                        }
                    }
                    m_key = 0;
                    m_has_write_permissions = false;
                }
            }

            bool key::get(const std::string& name, value& value) const
            {
                if (!const_cast<key*>(this)->open_for_reading())
                    return false;
                unsigned long _type;
                bytes data;
                unsigned long _data = 1024;

                data.resize(_data);

                while (_data < 32768)
                {
                    LRESULT lResult = ::RegQueryValueExW(m_key, wstr_param(name), nullptr, &_type, &data[0], &_data);
                    if (lResult == S_OK)
                    {
                        value = registry::value{ name, _type, data, _data };
                        return true;
                    }
                    if (lResult == ERROR_MORE_DATA)
                    {
                        _data *= 2;
                        data.resize(_data);
                    }
                    else return false;
                }
                return false;
            }

            bool key::set(const std::string& name, const value& value)
            {
                if (!open_for_writing())
                    return false;

                if (value.m_remove_flag)
                {
                    HRESULT result = RegDeleteValueW(m_key, wstr_param(name));
                    if (result == 0)
                        return true;

                    logging::report_windows_error(result, NGBT_FUNCTION_CONTEXT, std::format("RegDeleteValueW({}) failed", name));
                    return false;
                }
                else
                {
                    const bytes& raw_data = value.get_binary_type();
                    HRESULT result = RegSetValueExW(m_key, wstr_param(name), 0, value.get_type(), &raw_data[0], truncate_cast<DWORD>(raw_data.size()));
                    if (result == 0)
                        return true;

                    logging::report_windows_error(result, NGBT_FUNCTION_CONTEXT, std::format("RegSetValueEx({}) failed", name));
                    return false;
                }
            }

            bool key::open_for_reading()
            {
                if (m_key != 0)
                    return true;

                auto sv(string::split_at_first_occurence(m_name, '\\'));

                auto mapped_value = known_hives.find(string::uppercase(sv.first));
                if (mapped_value == known_hives.end())
                {
                    logging::error("'{0}' does not start with a known hive name [in key::open_for_reading()]", m_name);
                    return false;
                }

                // if this is a root key
                if (string::is_empty(sv.second))
                {
                    m_key = mapped_value->second;
                    m_is_root_key = true;
                    m_has_write_permissions = true;
                    return true;
                }

                HKEY hkRoot = mapped_value->second;
                HKEY hKey = 0;

                // todo: should handle permissions gracefully. try minimum permissions if possible, only upgrade if necessary
                HRESULT result = ::RegOpenKeyExW(hkRoot, wstr_param(sv.second), 0, KEY_READ, &hKey);
                if (result != S_OK)
                {
                    logging::warning("RegOpenKeyEx({}) for reading failed: {}", sv.second, result);
                    return false;
                }
                assert(hKey != 0);
                m_key = hKey;
                assert(!m_is_root_key);
                m_has_write_permissions = false;
                return true;
            }

            bool key::delete_recursive(const std::string& name)
            {
                auto items = string::split_at_last_occurence(name, '\\');
                return key(items.first).delete_subkey(items.second);
            }

            bool key::delete_subkey(const std::string& name)
            {
                if (!open_for_writing())
                    return false;
                
                HRESULT result = ::RegDeleteTree(m_key, wstr_param(name));
                if (result != S_OK)
                {
                    logging::report_windows_error(result, NGBT_FUNCTION_CONTEXT, std::format("RegDeleteTree({0}) failed", name));
                    return false;
                }
                return true;
            }

            bool key::open_for_writing()
            {
                if (m_has_write_permissions)
                    return true;

                auto sv(string::split_at_first_occurence(m_name, '\\'));

                auto mapped_value = known_hives.find(string::uppercase(sv.first));
                if (mapped_value == known_hives.end())
                {
                    logging::error("'{}' does not start with a known hive name [in key::open_for_writing()]", m_name);
                    return false;
                }

                // if this is a root key
                if (string::is_empty(sv.second))
                {
                    m_key = mapped_value->second;
                    m_is_root_key = true;
                    m_has_write_permissions = true;
                    return true;
                }

                HKEY hkRoot = mapped_value->second;
                HKEY hKey = 0;

                // todo: should handle permissions gracefully. try minimum permissions if possible, only upgrade if necessary
                HRESULT result = ::RegOpenKeyExW(hkRoot, wstr_param(sv.second), 0, KEY_WRITE|DELETE, &hKey);
                if (result != S_OK)
                {
                    if (result == ERROR_FILE_NOT_FOUND)
                    {
                        DWORD dwDisposition;
                        result = ::RegCreateKeyExW(hkRoot, wstr_param(sv.second), 0, nullptr, 0, KEY_WRITE | DELETE, nullptr, &hKey, &dwDisposition);
                        if (result != S_OK)
                        {
                            logging::report_windows_error(result, NGBT_FUNCTION_CONTEXT, std::format("RegCreateKeyEx({}) for writing failed", m_name));
                            return false;
                        }
                    }
                    else
                    {
                        logging::report_windows_error(result, NGBT_FUNCTION_CONTEXT, std::format("RegOpenKeyEx({}) for writing failed", m_name));
                        return false;
                    }
                }
                assert(hKey != 0);
                assert(!m_is_root_key);

                if (m_key)
                {
                    result = ::RegCloseKey(m_key);
                    if(result != S_OK)
                    {
                        logging::report_windows_error(result, NGBT_FUNCTION_CONTEXT, "RegCloseKey() failed");
                    }
                }
                m_key = hKey;
                assert(!m_is_root_key);
                m_has_write_permissions = true;
                return true;
            }
        }
    } // namespace win32
} // namespace ngbt
    
