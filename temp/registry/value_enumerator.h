#pragma once

#include <ngbtools/win32/registry/value_iterator.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            class value_iterator;

            /** \brief   A registry value enumerator.   
            *
            *            Why is there both a key_enumerator and a value_enumerator? Because given a registry_key,
            *            you can enumerate both keys and values, so you need two functions, each returning a separate temporary class
            *            that only implements begin and end. 
            */
             
            class value_enumerator final
            {
            public:
                /**
                * \brief   The default constructor is just a placeholder for the registry key handle
                *
                * \param   key The key.
                */
                explicit value_enumerator(HKEY key)
                    : m_key(key)
                {
                }

                /**
                * \brief   Find the beginning of the enumeration
                *
                * \return  a \c value_iterator at the beginning of the list
                */
                value_iterator begin() const
                {
                    return value_iterator(m_key);
                }

                /**
                * \brief   Find the end of the enumeration
                *
                * \return  a \c value_iterator that is invalid represents the end of the enumeration
                */
                value_iterator end() const
                {
                    return value_iterator(0);
                }

            private:
                /** \brief   The registry key. */
                HKEY m_key;
            };
        }
    }
}
