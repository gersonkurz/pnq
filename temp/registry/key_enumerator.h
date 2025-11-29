#pragma once

#include <ngbtools/win32/registry/key_iterator.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            /** \brief   A registry key enumerator.
            *
            *            Why is there both a key_enumerator and a value_enumerator? Because given a registry_key,
            *            you can enumerate both keys and values, so you need two functions, each returning a separate temporary class
            *            that only implements begin and end.
            */
            class key_enumerator final
            {
            public:

                /**
                * \brief   The default constructor is just a placeholder for the registry key handle
                *
                * \param   key          The key.
                * \param   parent_name  The name of the key (this is necessary so that the complete path name can be generated)
                */
                explicit key_enumerator(HKEY hKey, const std::string& parent_name)
                    : 
                    m_key(hKey),
                    m_parent_name(parent_name)
                {
                }


                /**
                * \brief   Find the beginning of the enumeration
                *
                * \return  a \c key_iterator at the beginning of the list
                */
                key_iterator begin() const
                {
                    return key_iterator(m_key, m_parent_name);
                }

                /**
                * \brief   Find the end of the enumeration
                *
                * \return  a \c key_iterator that is invalid represents the end of the enumeration
                */

                key_iterator end() const
                {
                    return key_iterator(0, m_parent_name);
                }

            private:
                /** \brief   The registry key. */
                HKEY m_key;
                
                /** \brief   Name of the parent. */
                std::string m_parent_name;
            };

        }
    }
}
