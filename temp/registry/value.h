#pragma once

#include <ngbtools/string.h>
#include <ngbtools/memory_view.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            /**
             * \brief   Query if 'dwType' is string type.
             *
             * \param   dwType  The type.
             *
             * \return  true if string type, false if not.
             */

            inline bool is_string_type(unsigned long registry_value_type)
            {
                return (registry_value_type == REG_SZ) or (registry_value_type == REG_EXPAND_SZ);
            }

            class key;

            extern const uint32_t REG_UNKNOWN;
            extern const uint32_t REG_ESCAPED_DWORD;
            extern const uint32_t REG_ESCAPED_QWORD;

            /** \brief   A registry value. */
            class value
            {
            public:
                /** \brief   Default constructor. */
                explicit value()
                    : 
                    m_type(REG_UNKNOWN),
                    m_remove_flag(false)
                {
                }

                explicit value(const std::string& name)
                    :
                    m_name(name),
                    m_type(REG_UNKNOWN),
                    m_remove_flag(false)
                {
                }

                // todo: read from registry: only possible in code once key is defined
                explicit value(key& k, const std::string& name);

                /**
                * \brief   Constructor.
                *
                * \param   name    The name.
                * \param   type    The type.
                * \param   data    The data.
                */

                explicit value(
                    const std::string& name,
                    unsigned long type,
                    const bytes& data,
                    unsigned long data_size)
                    :
                    m_name(name),
                    m_type(type),
                    m_data(data),
                    m_remove_flag(false)
                {
                    m_data.resize(data_size);
                }

                /**
                * \brief   Converts this object to a string.
                *
                * \return  A std::string.
                */

                std::string as_string() const;

                /**
                * \brief   Gets the double word.
                *
                * \return  The double word.
                */

                void set_none()
                {
                    m_data.clear();
                    m_type = REG_NONE;
                }

                bool is_default_value() const
                {
                    return string::is_empty(m_name);
                }

                void set_dword(uint32_t value)
                {
                    m_data.resize(sizeof(uint32_t));
                    m_type = REG_DWORD;
                    *reinterpret_cast<uint32_t*>(&m_data[0]) = value;
                }

                void set_qword(uint64_t value)
                {
                    m_data.resize(sizeof(uint64_t));
                    m_type = REG_QWORD;
                    *reinterpret_cast<uint64_t*>(&m_data[0]) = value;
                }

                void set_string(const std::string& value)
                {
                    assign_from_utf8_string(value, REG_SZ);
                }

                std::string get_string(const std::string& default_value = std::string()) const
                {
                    if (is_string_type(m_type))
                    {
                        // NOTE: the string is read in unicode format, but we need utf-8
                        return wstring::encode_as_utf8({ truncate_cast<wchar_t*>(&m_data[0]), m_data.size() / sizeof(wchar_t) });
                    }
                    assert(false); // not implemented yet
                    return default_value;
                }

                void set_expanded_string(const std::string& value)
                {
                    assign_from_utf8_string(value, REG_EXPAND_SZ);
                }

                void set_multi_string(const std::vector<std::string>& strings)
                {
                    m_data.clear();
                    m_type = REG_MULTI_SZ;
                    for (const auto& str : strings)
                    {
                        const std::wstring wvalue(string::encode_as_utf16(str));

                        const byte* p = reinterpret_cast<const byte*>(&wvalue[0]);
                        size_t len_in_bytes = sizeof(wchar_t) * (string::length(wvalue) + 1);
                        const byte* p_end = p + len_in_bytes;

                        m_data.insert(std::end(m_data), p, p_end);
                    }
                }

                void set_escaped_dword_value(const std::string& value)
                {
                    assign_from_utf8_string(value, REG_ESCAPED_DWORD);
                }

                void set_escaped_qword_value(const std::string& value)
                {
                    assign_from_utf8_string(value, REG_ESCAPED_QWORD);
                }

                uint32_t get_dword(uint32_t default_value = 0) const
                {
                    if (m_type != REG_DWORD)
                        return default_value;
                    if (m_data.size() < sizeof(uint32_t))
                        return default_value;

                    return *reinterpret_cast<const uint32_t*>(&m_data[0]);
                }

                uint64_t get_qword(uint64_t default_value = 0) const
                {
                    if (m_type != REG_QWORD)
                        return default_value;
                    if (m_data.size() < sizeof(uint64_t))
                        return default_value;

                    return *reinterpret_cast<const uint64_t*>(&m_data[0]);
                }

                const bytes& get_binary_type() const
                {
                    return m_data;
                }

                void set_binary_type(uint32_t new_type, const bytes& b)
                {
                    m_data = b;
                    m_type = new_type;
                }

                uint32_t get_type() const
                {
                    return m_type;
                }

            private:

                void assign_from_utf8_string(const std::string& value, uint32_t type)
                {
                    const std::wstring wvalue(string::encode_as_utf16(value));

                    size_t len_in_bytes = sizeof(wchar_t) * (string::length(wvalue) + 1);

                    m_data.resize(len_in_bytes);
                    m_type = type;
                    memcpy(&m_data[0], reinterpret_cast<const byte*>(wvalue.c_str()), len_in_bytes);
                }

            private:
                friend class key;
                friend class key_entry;
                friend class regfile_exporter;

                /** \brief   The name. */
                std::string m_name;

                /** \brief   The type. */
                uint32_t m_type;

                /** \brief   The data. */
                bytes m_data;

                bool m_remove_flag;
            };
        }
    }
}
