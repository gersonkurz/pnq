#pragma once

#include <ngbtools/win32/registry/value.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            /** \brief   A registry value iterator. The layout of this class is defined
             *           by how standard C++ iterators work - the details are the use of the Win32 API
             *           to enumerate the items
             */
            class value_iterator final
            {
            public:

                /**
                * \brief   The default constructor creates an iterator positioned at index 0 in 
                *
                * \param   key The key. If this is 0, the iterator marks the end of the list
                */

                explicit value_iterator(const HKEY& key)
                    : 
                    m_key(key),
                    m_index(0)
                {
                }

                /**
                * \brief   Inequality operator.
                *
                * \param   other   The other item to compare this against.
                *
                * \return  true if the parameters are not considered equivalent.
                */

                bool operator!= (const value_iterator& other) const
                {
                    if (m_key)
                    {
                        // we read the data "lazily", that is: only if the next step in the iteration is actually reached
                        const_cast<value_iterator*>(this)->ensure_valid_data();
                    }
                    return (m_key != other.m_key) or (m_index != other.m_index);
                }

                /**
                * \brief   Indirection operator.
                *
                * \return  A const reference to a registry \c value
                */

                const value& operator* () const
                {
                    return m_value;
                }

                /**
                * \brief   Increment operator. This just increments the enumeration index, nothing more...
                *
                * \return  The result of the operation. 
                */

                const value_iterator& operator++ ()
                {
                    if (m_key)
                    {
                        ++m_index;
                    }
                    return *this;
                }

            private:
                /** \brief   Ensures that the data is valid. This is really a helper function that groups 
                *            functionality needed for reading the value on a const object.
                */
                void ensure_valid_data()
                {
                    assert(m_key != 0);
                    if (!read_value())
                    {
                        m_key = 0;
                        m_index = 0;
                    }
                }

                /**
                * \brief    Actually read the value from the registry.
                *
                * \return   true if it succeeds, false if it fails.
                */

                bool read_value()
                {
                    unsigned long _name = truncate_cast<unsigned long>(m_wname.size());
                    unsigned long _type;
                    unsigned long _data = truncate_cast<unsigned long>(m_data.size());

                    if (_name == 0)
                    {
                        _name = 256;
                        m_wname.resize(_name);
                    }
                    if (_data == 0)
                    {
                        _data = 1024;
                        m_data.resize(_data);
                    }

                    while (_name < 32768)
                    {
                        LRESULT lResult = ::RegEnumValueW(m_key, m_index, &m_wname[0], &_name, nullptr, &_type, &m_data[0], &_data);
                        if (lResult == S_OK)
                        {
                            const auto name{ wstring::encode_as_utf8({ &m_wname[0], _name}) };

                            m_value = value{ name, _type, m_data, _data };
                            return true;
                        }
                        if (lResult == ERROR_MORE_DATA)
                        {
                            _name *= 2;
                            m_wname.resize(_name);
                            _data *= 2;
                            m_data.resize(_data);
                        }
                        else return false;
                    }
                    return false;
                }

            private:
                /** \brief   The zero-based index. */
                unsigned long m_index;

                /** \brief   The registry key. */
                HKEY m_key;

                /** \brief   The current registry value. Warning: this might be invalid, if the object itself is invalid. */
                value m_value;

                /** \brief   Temporary buffer for the registry value name */
                std::vector<wchar_t> m_wname;

                /** \brief   Temporary buffer for the registry value data */
                bytes m_data;
            };
        }
    }
}
