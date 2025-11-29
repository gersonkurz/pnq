#include <ngbtools/ngbtools.h>
#include <ngbtools/win32/registry/regfile_exporter.h>
#include <ngbtools/win32/registry/key_entry.h>
#include <ngbtools/win32/registry/key.h>
#include <ngbtools/win32/registry/value.h>
#include <ngbtools/string.h>
#include <ngbtools/win32/wstr_param.h>
#include <ngbtools/logging.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {

            template <typename VALUE_TYPE> class sorted_map_item_container
            {
            public:
                std::string key;
                VALUE_TYPE value;
            };

            template <typename VALUE_TYPE> class sorted_string_mapI : public std::vector< sorted_map_item_container<VALUE_TYPE> >
            {
            private:
                struct item_sorter
                {
                    inline bool operator() (const sorted_map_item_container<VALUE_TYPE>& struct1, const sorted_map_item_container<VALUE_TYPE>& struct2)
                    {
                        return struct1.key < struct2.key;
                    }
                };
            public:
                sorted_string_mapI(const std::unordered_map<std::string, VALUE_TYPE>& unsorted_string_map)
                {
                    for (const auto item : unsorted_string_map)
                    {
                        this->push_back({ string::lowercase(item.first), item.second });
                    }

                    std::sort(std::begin(*this), std::end(*this), item_sorter());
                }
            };

            regfile_exporter::~regfile_exporter()
            {
            }

            bool regfile_exporter::perform_export(const key_entry* key, REGFILE_EXPORT_OPTIONS export_options)
            {
                string::writer output;

                bool result = export_to_writer(key, output, export_options);
                if (!result)
                    return result;
                
                if (!string::is_empty(m_filename))
                {
                    // now, write to file in proper encoding
                }
                return true;
            }

            bool regfile_exporter::export_to_writer(const key_entry* key, string::writer& output, REGFILE_EXPORT_OPTIONS export_options)
            {
                output.append(m_header);
                output.append(string::newline);
                output.append(string::newline);

                return export_to_writer_recursive(key, output, has_flag(export_options, REGFILE_EXPORT_OPTIONS::NO_EMPTY_KEYS));
            }

            bool regfile_exporter::write_hex_encoded_value(const value* value, string::writer& output)
            {
                if (value->m_type == REG_BINARY)
                {
                    output.append_formatted("{0}=hex:", value->m_name);
                }
                else
                {
                    output.append_formatted("{0}=hex({1:x}):", value->m_name, value->m_type);
                }
                int bytes_written = 0;
                std::string separator(",");
                int i = 0;
                for (byte b : value->m_data)
                {
                    if (i > 0)
                    {
                        output.append(separator);
                        separator = ",";
                    }
					output.append_formatted("{0:02x}", b);
                    if (bytes_written < 19)
                        ++bytes_written;
                    else
                    {
                        bytes_written = 0;
                        separator = ",\\\r\n  ";
                    }
                    ++i;
                }
                output.append("\r\n");
                return true;
            }


            std::string regfile_exporter::reg_escape_string(const std::string& input)
            {
                string::writer output;
                for (char c : input)
                {
                    if (c == '"')
                    {
                        output.append('\\');
                        output.append('"');
                    }
                    else if (c == '\\')
                    {
                        output.append('\\');
                        output.append('\\');
                    }
                    else
                    {
                        output.append(c);
                    }
                }
                return output.as_string();
            }

            bool regfile_exporter::export_to_writer(const value* value, string::writer& output)
            {
                std::string name(value->is_default_value() ? "@" :
                    std::format("\"{0}\"", reg_escape_string(value->m_name)).c_str());

                if (value->m_remove_flag)
                {
                    output.append_formatted("{0}=-\r\n", name);
                }
                else if (value->m_type == REG_SZ)
                {
                    output.append_formatted("{0}=\"{1}\"\r\n", name, reg_escape_string(value->get_string()));
                }
                else if (value->m_type == REG_DWORD)
                {
                    output.append_formatted("{0}=dword:{1:08x}\r\n", name, value->get_dword());
                }
                else if (value->m_type == REG_QWORD)
                {
                    output.append_formatted("{0}=dword:{1:016x}\r\n", name, value->get_qword());
                }
                else
                {
                    return write_hex_encoded_value(value, output);
                }
                return true;
            }

            bool regfile_exporter::export_to_writer_recursive(const key_entry* key, string::writer& output, bool no_empty_keys)
            {
                bool skip_this_entry = false;
                if (no_empty_keys)
                {
                    skip_this_entry = key->m_values.empty();
                }
                if (!skip_this_entry and !string::is_empty(key->m_name))
                {
                    if (key->m_remove_flag)
                    {
                        output.append_formatted("[-{0}]\r\n", key->get_path());
                    }
                    else
                    {
                        output.append_formatted("[{0}]\r\n", key->get_path());
                        if (key->m_default_value)
                        {
                            export_to_writer(key->m_default_value, output);
                        }

                        sorted_string_mapI<value*> sorted_values(key->m_values);
                        for (auto item : sorted_values)
                        {
                            if (!export_to_writer(item.value, output))
                            {
                                return false;
                            }
                        }
                    }
                    output.append(string::newline);
                }

                sorted_string_mapI<key_entry*> sorted_keys(key->m_keys);
                for (auto item : sorted_keys)
                {
                    if (!export_to_writer_recursive(item.value, output, no_empty_keys))
                    {
                        return false;
                    }
                }
                return true;
            }

            registry_exporter::registry_exporter()
            {
            }

            registry_exporter::~registry_exporter()
            {
            }

            bool registry_exporter::export_recursive(const key_entry* this_key, bool no_empty_keys)
            {
                bool success = true;
                bool skip_this_entry = false;
                if (no_empty_keys)
                {
                    skip_this_entry = this_key->m_values.empty();
                }
                if (!skip_this_entry and !string::is_empty(this_key->m_name))
                {
                    if (this_key->m_remove_flag)
                    {
                        logging::warning("ok, about to delete this recursively: '{0}'", this_key->get_path());
                        if (!registry::key::delete_recursive(this_key->get_path()))
                        {
                            success = false;
                        }
                    }
                    else
                    {
                        // need to create this key
                        registry::key k(this_key->get_path());
                        if (k.open_for_writing())
                        {
                            if (this_key->m_default_value)
                            {
                                if (!k.set("", *this_key->m_default_value))
                                {
                                    success = false;
                                }
                            }

                            for (auto item : this_key->m_values)
                            {
                                if (!k.set(item.first, *item.second))
                                {
                                    success = false;
                                }
                            }
                        }
                    }
                }
                for (auto item : this_key->m_keys)
                {
                    if (!export_recursive(item.second, no_empty_keys))
                    {
                        success = false;
                    }

                }
                return success;
            }

            bool registry_exporter::perform_export(const key_entry* key, REGFILE_EXPORT_OPTIONS export_options)
            {
                return export_recursive(key, has_flag(export_options, REGFILE_EXPORT_OPTIONS::NO_EMPTY_KEYS));
            }
        }
    } // namespace win32
} // namespace ngbt
    
