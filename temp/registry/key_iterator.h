#pragma once

#include <ngbtools/string_writer.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            /** \brief   A registry key iterator. The layout of this class is defined
            *           by how standard C++ iterators work - the details are the use of the Win32 API
            *           to enumerate the keys.
            */
            class key_iterator final
            {
            public:

                /**
                * \brief   The default constructor creates an iterator positioned at index 0 in
                *
                * \param   key          The key. If this is 0, the iterator marks the end of the list
                * \param   parent_name  The name of the parent key. This is used to generate the complete pathname.
                */
                key_iterator(HKEY key, const std::string& parent_name)
                    : 
                    m_key(key),
                    m_index(0),
                    m_parent_name(parent_name)
                {
                }

                /**
                * \brief   Inequality operator.
                *
                * \param   other   The other item to compare this against.
                *
                * \return  true if the parameters are not considered equivalent.
                */

                bool operator!= (const key_iterator& other) const
                {
                    if (m_key)
                    {
                        // we read the data "lazily", that is: only if the next step in the iteration is actually reached
                        const_cast<key_iterator*>(this)->ensure_valid_data();
                    }

                    return (m_key != other.m_key) or (m_index != other.m_index);
                }

                /**
                * \brief   Indirection operator.
                *
                * \return  A string with the complete path of this registry value
                */
                const std::string& operator* () const
                {
                    return m_path;
                }

                /**
                * \brief   Increment operator. This just increments the enumeration index, nothing more...
                *
                * \return  The result of the operation. 
                */

                const key_iterator& operator++ ()
                {
                    ++m_index;
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
                    unsigned long _class = truncate_cast<unsigned long>(m_wclass.size());

                    if (_name == 0)
                    {
                        _name = 256;
                        m_wname.resize(_name);
                    }
                    if (_class == 0)
                    {
                        _class = 1024;
                        m_wclass.resize(_class);
                    }
                    FILETIME ignored;

                    while (_name < 32768)
                    {
                        LRESULT lResult = ::RegEnumKeyExW(m_key, m_index, &m_wname[0], &_name, nullptr, &m_wclass[0], &_class, &ignored);
                        if (lResult == S_OK)
                        {
                            const auto name(wstring::encode_as_utf8({ &m_wname[0], _name }));
                            m_path = std::format("{0}\\{1}", m_parent_name, name);
                            return true;
                        }
                        if (lResult == ERROR_MORE_DATA)
                        {
                            _name *= 2;
                            m_wname.resize(_name);
                            _class *= 2;
                            m_wclass.resize(_class);
                        }
                        else return false;
                    }
                    return false;
                }

            private:
                /** \brief   The index. */
                unsigned long m_index;

                /** \brief   The key. */
                HKEY m_key;

                /** \brief   The value. */
                std::string m_path;

                /** \brief   Name of the parent. */
                const std::string m_parent_name;

                /** \brief   Temporary buffer for the registry key name */
                std::vector<wchar_t> m_wname;

                /** \brief   Temporary buffer for the registry class data */
                std::vector<wchar_t> m_wclass;
            };


        }
    }
}
