#pragma once

/// @file pnq/regis3/exporter.h
/// @brief Registry exporters - to .REG files and live registry

#include <pnq/regis3/types.h>
#include <pnq/regis3/value.h>
#include <pnq/regis3/key_entry.h>
#include <pnq/string_writer.h>
#include <pnq/text_file.h>
#include <pnq/logging.h>

#ifdef PNQ_PLATFORM_WINDOWS
#include <pnq/regis3/key.h>
#endif

#include <algorithm>
#include <vector>

namespace pnq
{
    namespace regis3
    {
        // =====================================================================
        // Export Interface
        // =====================================================================

        /// Pure virtual interface for registry exporters.
        class export_interface
        {
        public:
            virtual ~export_interface() = default;

            /// Export key_entry tree to the target.
            /// @param key Root key entry to export
            /// @param options Export options
            /// @return true if successful
            virtual bool perform_export(const key_entry* key, export_options options = export_options::none) = 0;
        };

        // =====================================================================
        // Helper: Sorted Map Iterator
        // =====================================================================

        /// Helper to iterate an unordered_map in sorted key order.
        template <typename T>
        class sorted_map
        {
        public:
            struct item
            {
                std::string key;
                T value;
            };

            explicit sorted_map(const std::unordered_map<std::string, T>& source)
            {
                m_items.reserve(source.size());
                for (const auto& [k, v] : source)
                {
                    m_items.push_back({string::lowercase(k), v});
                }
                std::sort(m_items.begin(), m_items.end(),
                          [](const item& a, const item& b) { return a.key < b.key; });
            }

            auto begin() const { return m_items.begin(); }
            auto end() const { return m_items.end(); }

        private:
            std::vector<item> m_items;
        };

        // =====================================================================
        // RegFile Exporter Base
        // =====================================================================

        /// Escape a string for .REG file format.
        /// Escapes backslashes and double quotes.
        inline std::string reg_escape_string(std::string_view input)
        {
            string::Writer output;
            for (char c : input)
            {
                if (c == '"')
                {
                    output.append("\\\"");
                }
                else if (c == '\\')
                {
                    output.append("\\\\");
                }
                else
                {
                    output.append(c);
                }
            }
            return output.as_string();
        }

        /// Base class for .REG file exporters.
        class regfile_exporter : public export_interface
        {
        public:
            PNQ_DECLARE_NON_COPYABLE(regfile_exporter)

            virtual ~regfile_exporter() = default;

            /// Export to file or get string result.
            /// @param key Root key entry to export
            /// @param options Export options
            /// @return true if successful
            bool perform_export(const key_entry* key, export_options options = export_options::none) override
            {
                string::Writer output;

                // Write header
                output.append(m_header);
                output.append("\r\n\r\n");

                // Write content recursively
                if (!export_recursive(key, output, has_flag(options, export_options::no_empty_keys)))
                    return false;

                m_result = output.as_string();

                // Write to file if filename was provided
                if (!m_filename.empty())
                {
                    return write_file();
                }

                return true;
            }

            /// Get the export result as a string.
            /// Only valid after perform_export() succeeds.
            const std::string& result() const { return m_result; }

        protected:
            regfile_exporter(std::string_view header, bool use_utf8, std::string_view filename = {})
                : m_header{header},
                  m_use_utf8{use_utf8},
                  m_filename{filename}
            {
            }

            /// Write the result to file with appropriate encoding.
            virtual bool write_file() const
            {
                return text_file::write_utf8(m_filename, m_result);
            }

        private:
            /// Export a key_entry tree recursively.
            static bool export_recursive(const key_entry* key, string::Writer& output, bool no_empty_keys)
            {
                bool skip_this_entry = false;

                if (no_empty_keys)
                {
                    skip_this_entry = !key->has_values() && key->default_value() == nullptr;
                }

                if (!skip_this_entry && !key->name().empty())
                {
                    if (key->remove_flag())
                    {
                        output.append_formatted("[-{}]\r\n", key->get_path());
                    }
                    else
                    {
                        output.append_formatted("[{}]\r\n", key->get_path());

                        // Export default value first
                        if (key->default_value())
                        {
                            export_value(key->default_value(), output);
                        }

                        // Export named values in sorted order
                        sorted_map<value*> sorted_values{key->values()};
                        for (const auto& item : sorted_values)
                        {
                            export_value(item.value, output);
                        }
                    }
                    output.append("\r\n");
                }

                // Export subkeys in sorted order
                sorted_map<key_entry*> sorted_keys{key->keys()};
                for (const auto& item : sorted_keys)
                {
                    if (!export_recursive(item.value, output, no_empty_keys))
                        return false;
                }

                return true;
            }

            /// Export a single value.
            static void export_value(const value* val, string::Writer& output)
            {
                std::string name = val->is_default_value() ? "@" :
                    std::format("\"{}\"", reg_escape_string(val->name()));

                if (val->remove_flag())
                {
                    output.append_formatted("{}=-\r\n", name);
                }
                else if (val->type() == REG_SZ)
                {
                    output.append_formatted("{}=\"{}\"\r\n", name, reg_escape_string(val->get_string()));
                }
                else if (val->type() == REG_DWORD)
                {
                    output.append_formatted("{}=dword:{:08x}\r\n", name, val->get_dword());
                }
                else if (val->type() == REG_QWORD)
                {
                    // QWORD is exported as hex(b):
                    write_hex_value(val, output, name);
                }
                else
                {
                    write_hex_value(val, output, name);
                }
            }

            /// Write a hex-encoded value (for binary, multi-string, expand-string, etc.)
            static void write_hex_value(const value* val, string::Writer& output, const std::string& name)
            {
                // Build header
                if (val->type() == REG_BINARY)
                {
                    output.append_formatted("{}=hex:", name);
                }
                else
                {
                    output.append_formatted("{}=hex({:x}):", name, val->type());
                }

                // Write bytes with line wrapping at ~77 chars
                const bytes& data = val->get_binary();
                int line_len = static_cast<int>(name.length()) + 6; // rough estimate of header length
                bool first = true;

                for (std::uint8_t b : data)
                {
                    if (!first)
                    {
                        if (line_len >= 75)
                        {
                            output.append(",\\\r\n  ");
                            line_len = 2;
                        }
                        else
                        {
                            output.append(",");
                            line_len += 1;
                        }
                    }
                    first = false;
                    output.append_formatted("{:02x}", b);
                    line_len += 2;
                }

                output.append("\r\n");
            }

        protected:
            std::string m_header;
            bool m_use_utf8;
            std::string m_filename;
            std::string m_result;
        };

        // =====================================================================
        // Format-Specific Exporters
        // =====================================================================

#ifdef PNQ_PLATFORM_WINDOWS
        /// Exporter for REGEDIT4 format .REG files (ANSI/CP_ACP encoding).
        /// Windows-only due to codepage conversion requirements.
        class regfile_format4_exporter final : public regfile_exporter
        {
        public:
            explicit regfile_format4_exporter(std::string_view filename = {})
                : regfile_exporter{HEADER_FORMAT4, false, filename}
            {
            }

        protected:
            bool write_file() const override
            {
                // REGEDIT4 uses system ANSI codepage (CP_ACP), no BOM
                return text_file::write_ansi(m_filename, m_result);
            }
        };
#endif // PNQ_PLATFORM_WINDOWS

        /// Exporter for Windows Registry Editor Version 5.00 format (UTF-16LE encoding).
        class regfile_format5_exporter final : public regfile_exporter
        {
        public:
            explicit regfile_format5_exporter(std::string_view filename = {})
                : regfile_exporter{HEADER_FORMAT5, true, filename}
            {
            }

        protected:
            bool write_file() const override
            {
                // REGEDIT5 uses UTF-16LE encoding with BOM
                string16 wide = unicode::to_utf16(m_result);
                return text_file::write_utf16(m_filename, wide);
            }
        };

        // =====================================================================
        // Live Registry Exporter (Windows-only)
        // =====================================================================

#ifdef PNQ_PLATFORM_WINDOWS
        /// Exporter that writes a key_entry tree to the live Windows registry.
        class registry_exporter final : public export_interface
        {
        public:
            registry_exporter() = default;

            /// Export key_entry tree to the live registry.
            /// @param root Root key entry to export
            /// @param options Export options
            /// @return true if all operations succeeded
            bool perform_export(const key_entry* root, export_options options = export_options::none) override
            {
                return export_recursive(root, has_flag(options, export_options::no_empty_keys));
            }

        private:
            bool export_recursive(const key_entry* entry, bool no_empty_keys)
            {
                bool success = true;
                bool skip_this_entry = false;

                if (no_empty_keys)
                {
                    skip_this_entry = !entry->has_values() && entry->default_value() == nullptr;
                }

                if (!skip_this_entry && !entry->name().empty())
                {
                    if (entry->remove_flag())
                    {
                        // Delete this key and all subkeys
                        if (!key::delete_recursive(entry->get_path()))
                        {
                            spdlog::warn("Failed to delete registry key: {}", entry->get_path());
                            success = false;
                        }
                    }
                    else
                    {
                        // Create/open the key and write values
                        key k{entry->get_path()};
                        if (k.open_for_writing())
                        {
                            // Write default value
                            if (entry->default_value())
                            {
                                if (!k.set("", *entry->default_value()))
                                {
                                    success = false;
                                }
                            }

                            // Write named values
                            for (const auto& [name, val] : entry->values())
                            {
                                if (!k.set(val->name(), *val))
                                {
                                    success = false;
                                }
                            }
                        }
                        else
                        {
                            spdlog::warn("Failed to open registry key for writing: {}", entry->get_path());
                            success = false;
                        }
                    }
                }

                // Recurse into subkeys
                for (const auto& [name, subkey] : entry->keys())
                {
                    if (!export_recursive(subkey, no_empty_keys))
                    {
                        success = false;
                    }
                }

                return success;
            }
        };
#endif // PNQ_PLATFORM_WINDOWS

    } // namespace regis3
} // namespace pnq
