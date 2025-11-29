#pragma once

#include <ngbtools/dynamic_object.h>
#include <ngbtools/string.h>
#include <ngbtools/win32/registry/regfile_parser.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            class value;

            enum class REG_FILE_EXPORT_OPTIONS : uint32_t
            {
                NONE = 0,
                NO_EMPTY_KEYS = (1<<0)
            };

            /** \brief   A registry key. */
            class key_entry final : public dynamic_object_impl
            {
            public:
                /** \brief   The default constructor creates an empty - unnamed - registry key. */
                explicit key_entry()
                    :
                    m_parent(nullptr),
                    m_default_value(nullptr),
                    m_remove_flag(false)
                {
                }

                explicit key_entry(key_entry* parent, const std::string& name)
                    :
                    m_parent(parent),
                    m_name(name),
                    m_default_value(nullptr),
                    m_remove_flag(false)
                {
                    if (m_parent)
                    {
                        NGBT_ADDREF(m_parent);
                    }
                }

                virtual ~key_entry();

                /**
                 * \brief    When creating a diff/merge file, asks the key to remove a subkey based on an existing key
                 *
                 * \param   remove_this The remove this.
                 *
                 * \return  Key in the diff/merge file with the RemoveFlag set
                 */

                key_entry* ask_to_remove_key(const key_entry* remove_this)
                {
                    key_entry* key = ask_to_add_key(remove_this);
                    key->m_remove_flag = true;
                    return key;
                }

                /**
                 * \brief   When creating a diff/merge file, asks the key to create a subkey based on an existing key
                 *
                 * \param   add_this    The add this.
                 *
                 * \return  Key in the diff/merge file
                 */

                key_entry* ask_to_add_key(const key_entry* add_this);

                /**
                 * \brief   When creating a diff/merge file, asks the key to remove a value based on an existing value
                 *
                 * \param   key The key.
                 * \param   v   The value to process.
                 */

                void ask_to_remove_value(const key_entry* key, const value* v);

                /**
                 * \brief   When creating a diff/merge file, asks the key to add a value based on an existing value
                 *
                 * \param   key The key.
                 * \param   v   The value to process.
                 */

                void ask_to_add_value(const key_entry* key, const value* v);

                /**
                 * \brief    Return the complete (recursive) path of this key
                 *
                 * \return  The path.
                 */

                std::string get_path() const;

                std::string as_string() const;

                void write_reg_file_format(string::writer& output, REG_FILE_EXPORT_OPTIONS options);

            private:
                key_entry(const key_entry& copy_src) = delete;
                key_entry& operator=(const key_entry& copy_src) = delete;
                key_entry(key_entry&& move_src) = delete;
                key_entry& operator=(key_entry&& move_src) = delete;

                key_entry* clone(key_entry* parent) const;

            private:
                friend class regfile_parser;
                friend class regfile_exporter;
                friend class registry_exporter;

                /**
                 * \brief   Find or create a subkey relative to this one
                 *
                 * \param   name    The name.
                 *
                 * \return  Newly created subkey
                 */

                key_entry* find_or_create_key(const std::string& name);

                /**
                 * \brief   Find or create a named registry value
                 *
                 * \param   name    The name.
                 *
                 * \return  Newly created registry value or null if it cannot be created
                 */

                value* find_or_create_value(const std::string& name);

            private:

                /** \brief   Name of the key (not the complete path name: use the Path member for that) */
                std::string m_name;

                /** \brief   Parent key or null if this is a root key */
                key_entry* m_parent;
            
                /** \brief   Subkeys relative to this key */
                std::unordered_map<std::string, key_entry*> m_keys;
            
                /** \brief   Values in this key */
                std::unordered_map<std::string, value*> m_values;

                /** \brief   Default value or null if undefined */  
                value* m_default_value;
            
                /** \brief   Flag indicating wether the .REG file actually asks to REMOVE this value, rather than add it. */
                bool m_remove_flag;


            };
        }
    }
}
