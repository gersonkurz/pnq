#include <ngbtools/ngbtools.h>
#include <ngbtools/win32/registry/key_entry.h>
#include <ngbtools/win32/registry/regfile_exporter.h>
#include <ngbtools/win32/registry/value.h>
#include <ngbtools/string.h>
#include <ngbtools/win32/wstr_param.h>
#include <ngbtools/logging.h>
#include <ngbtools/utilities.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            key_entry* key_entry::clone(key_entry* parent) const
            {
                key_entry* result = DBG_NEW key_entry(parent, m_name);

                for (auto keys_iter : m_keys)
                {
                    result->m_keys[keys_iter.first] = keys_iter.second->clone(result);
                }

                for (auto values_iter : m_values)
                {
                    result->m_values[values_iter.first] = DBG_NEW value(*values_iter.second);
                }

                if (m_default_value)
                {
                    result->m_default_value = DBG_NEW value(*m_default_value);
                }
                return result;
            }
            
            key_entry::~key_entry()
            {
                if (m_parent)
                {
                    NGBT_ADDREF(m_parent);
                }
                utilities::clean(m_values);
                // cannot use utilities::clean(m_keys) because those are dynamic objects
                for (auto var : m_keys)
                {
                    NGBT_RELEASE(var.second);
                }
            }
            key_entry* key_entry::ask_to_add_key(const key_entry* add_this)
            {
                key_entry* key = find_or_create_key(add_this->get_path());
                for (const auto& subkey : add_this->m_keys)
                {
                    key_entry* clone = subkey.second->clone(key);
                    key->m_keys[subkey.first] = clone;

                }
                for (const auto& existing_value : add_this->m_values)
                {
                    key->m_values[existing_value.first] = DBG_NEW registry::value(*existing_value.second);
                }
                if (add_this->m_default_value)
                {
                    key->m_default_value = DBG_NEW registry::value(*key->m_default_value);
                }
                else
                {
                    assert(key->m_default_value == nullptr);
                }
                return key;
            }

            void key_entry::ask_to_remove_value(const key_entry* key, const value* v)
            {
                key_entry* k = ask_to_add_key(key);
                if (v->is_default_value())
                {
                    k->m_default_value = DBG_NEW registry::value(*v);
                    k->m_default_value->m_remove_flag = true;
                }
                else
                {
                    std::string value_name(string::lowercase(v->m_name));
                    registry::value* nv = DBG_NEW registry::value(*v);
                    nv->m_remove_flag = true;
                    k->m_values[value_name] = nv;
                }
            }


            void key_entry::ask_to_add_value(const key_entry* key, const value* v)
            {
                key_entry* k = ask_to_add_key(key);
                if (v->is_default_value())
                {
                    k->m_default_value = DBG_NEW registry::value(*v);
                }
                else
                {
                    std::string value_name(string::lowercase(v->m_name));
                    k->m_values[value_name] = DBG_NEW registry::value(*v);
                }
            }

            std::string key_entry::as_string() const
            {
                string::writer result;
                regfile_exporter::export_to_writer_recursive(this, result, false);
                return result.as_string();
            }
           

            std::string key_entry::get_path() const
            {
                std::vector<std::string> path_components;
                const key_entry* k = this;
                while (k)
                {
                    if (!k->m_parent)
                    {
                        if (!string::is_empty(k->m_name))
                        {
                            path_components.push_back(k->m_name);
                        }
                        break;
                    }
                    assert(!string::is_empty(k->m_name));
                    path_components.push_back(k->m_name);
                    k = k->m_parent;
                }

                string::writer result;
                bool first = true;
                for (int32_t index = int32_t(path_components.size()) - 1; index >= 0; --index)
                {
                    if (first)
                        first = false;
                    else
                        result.append('\\');
                    result.append(path_components[index]);
                }
                return result.as_string();
            }

            value* key_entry::find_or_create_value(const std::string& name)
            {
                if (string::is_empty(name))
                {
                    if (!m_default_value)
                    {
                        m_default_value = DBG_NEW registry::value();
                    }
                    return m_default_value;
                }
                std::string name_as_key(string::lowercase(name));
                auto result = m_values.find(name_as_key);
                if (result != m_values.end())
                    return result->second;

                value* v = DBG_NEW registry::value(name);
                m_values[name_as_key] = v;
                return v;
            }

            key_entry* key_entry::find_or_create_key(const std::string& path)
            {
                assert(!string::is_empty(path));

                std::string copy_of_path{ path };

                bool remove_this = false;
                if (string::starts_with(path, "-"))
                {
                    remove_this = true;
                    copy_of_path = string::substring(copy_of_path.c_str(), 1);
                }

                key_entry* result = this;

                auto tokens(string::split(copy_of_path, "\\"));
                for (const auto token : tokens)
                {
                    std::string key(string::lowercase(token));
                    auto item = result->m_keys.find(key);
                    key_entry* subkey = nullptr;
                    if (item != result->m_keys.end())
                    {
                        subkey = item->second;
                    }
                    else
                    {
                        subkey = DBG_NEW key_entry(result, token);
                        result->m_keys[key] = subkey;
                    }
                    assert(subkey->m_parent == result);
                    result = subkey;
                }
                if (remove_this)
                {
                    result->m_remove_flag = true;
                }
                return result;
            }

            void key_entry::write_reg_file_format(string::writer& /*output*/, REG_FILE_EXPORT_OPTIONS /*options*/)
            {
//                std::vector<std::string> names;
//                bool skip_this_entry = false;
            }
        }
    } // namespace win32
} // namespace ngbt
    
