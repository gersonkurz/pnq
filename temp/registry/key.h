#pragma once

#include <ngbtools/win32/registry/value_enumerator.h>
#include <ngbtools/win32/registry/key_enumerator.h>
#include <ngbtools/string.h>

/**      \file ngbt/win32/registry_key.h
*
*       \brief Defines a wrapper class for win32 registry key API.
*       	   This class does not REPRESENT the keys in memory, it PROVIDES ACCESS to them in the registry database.
*       	   See \c registry::key_entry for representation of a key in memory (for example, when reading .REG files)
*       	   
*
*/

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            class value;

            /** \brief   A registry key. */
            class key final
            {
            public:
                /** \brief   Default constructor. */
                explicit key(const std::string& name)
                    :
                    m_name(name),
                    m_key(0),
                    m_is_root_key(false),
                    m_has_write_permissions(false)
                {
                }

                ~key()
                {
                    close();
                }

            private:
                key(const key&) = delete;
                key& operator=(const key&) = delete;
                key(key&&) = delete;
                key& operator=(key&&) = delete;

            public:
                bool open_for_reading();
                bool open_for_writing();
                void close();

                bool delete_subkey(const std::string& name);
                static bool delete_recursive(const std::string& name);

                bool get(const std::string& name, value& value) const;
                bool set(const std::string& name, const value& value);

                std::string get_string(const std::string& name, const std::string& default_value = std::string()) const
                {
                    registry::value v;
                    if (get(name, v))
                    {
                        return v.get_string(default_value);
                    }
                    return default_value;
                }

                uint32_t get_dword(const std::string& name, uint32_t default_value = 0) const
                {
                    registry::value v;
                    if (get(name, v))
                    {
                        return v.get_dword(default_value);
                    }
                    return default_value;
                }

                bool set_string(const std::string& name, const std::string& value)
                {
                    registry::value v;
                    v.set_string(value);
                    return set(name, v);
                }

                bool set_dword(const std::string& name, uint32_t value)
                {
                    registry::value v;
                    v.set_dword(value);
                    return set(name, v);
                }

                bool is_open() const
                {
                    return m_key != 0;
                }

                std::string as_string() const
                {
                    return std::format("[{0}]", m_name);
                }

                /**
                 * \brief   Enum values.
                 *
                 * \return  A registry_value_enumerator.
                 */

                value_enumerator enum_values() const
                {
                    return value_enumerator(m_key);
                }

                /**
                 * \brief   Enum keys.
                 *
                 * \return  A key_enumerator.
                 */

                key_enumerator enum_keys() const
                {
                    return key_enumerator(m_key, m_name);
                }

                std::string get_path() const
                {
                    return m_name;
                }

                std::string get_name() const
                {
                    return string::split_at_last_occurence(m_name, '\\').second;
                }

            private:
                std::string m_name;
                HKEY m_key;
                bool m_is_root_key;
                bool m_has_write_permissions;
            };
        }
    }
}
