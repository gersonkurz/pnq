#pragma once

/// @file pnq/regis3/importer.h
/// @brief Registry importers - from .REG files and live registry

#include <pnq/regis3/types.h>
#include <pnq/regis3/key_entry.h>
#include <pnq/regis3/key.h>
#include <pnq/regis3/parser.h>
#include <pnq/text_file.h>
#include <pnq/ref_counted.h>

namespace pnq
{
    namespace regis3
    {
        // =====================================================================
        // Import Interface
        // =====================================================================

        /// Pure virtual interface for registry importers.
        class import_interface
        {
        public:
            virtual ~import_interface() = default;

            /// Import registry data and return the resulting key tree.
            /// @return Root key entry (caller must release), or nullptr on failure
            virtual key_entry* import() = 0;
        };

        // =====================================================================
        // RegFile Importer Base
        // =====================================================================

        /// Base class for .REG file importers.
        /// Holds content and parser, provides import() implementation.
        class regfile_importer : public import_interface
        {
        public:
            PNQ_DECLARE_NON_COPYABLE(regfile_importer)

            virtual ~regfile_importer()
            {
                if (m_result)
                {
                    PNQ_RELEASE(m_result);
                }
            }

            /// Import the .REG file content.
            /// @return Root key entry (caller must release), or nullptr on failure
            key_entry* import() override
            {
                if (!m_result)
                {
                    if (!m_parser.parse_text(m_content))
                        return nullptr;

                    m_result = m_parser.get_result();
                }

                if (m_result)
                {
                    PNQ_ADDREF(m_result);
                }
                return m_result;
            }

        protected:
            regfile_importer(std::string_view content, std::string_view expected_header, import_options options)
                : m_content{content},
                  m_parser{expected_header, options},
                  m_result{nullptr}
            {
            }

        private:
            std::string m_content;
            regfile_parser m_parser;
            key_entry* m_result;
        };

        // =====================================================================
        // Format-Specific Importers
        // =====================================================================

        /// Header for REGEDIT4 format files.
        inline constexpr std::string_view HEADER_FORMAT4 = "REGEDIT4";

        /// Header for Windows Registry Editor Version 5.00 format files.
        inline constexpr std::string_view HEADER_FORMAT5 = "Windows Registry Editor Version 5.00";

        /// Importer for REGEDIT4 format .REG files.
        class regfile_format4_importer final : public regfile_importer
        {
        public:
            explicit regfile_format4_importer(std::string_view content, import_options options = import_options::none)
                : regfile_importer{content, HEADER_FORMAT4, options}
            {
            }
        };

        /// Importer for Windows Registry Editor Version 5.00 format .REG files.
        class regfile_format5_importer final : public regfile_importer
        {
        public:
            explicit regfile_format5_importer(std::string_view content, import_options options = import_options::none)
                : regfile_importer{content, HEADER_FORMAT5, options}
            {
            }
        };

        // =====================================================================
        // Factory Functions
        // =====================================================================

        /// Auto-detect format and create appropriate importer from string content.
        /// @param content .REG file content
        /// @param options Import options
        /// @return Importer instance, or nullptr if format not recognized
        inline std::unique_ptr<regfile_importer> create_importer_from_string(
            std::string_view content,
            import_options options = import_options::none)
        {
            // Check for format 5 header first (more specific)
            if (content.starts_with(HEADER_FORMAT5))
            {
                return std::make_unique<regfile_format5_importer>(content, options);
            }
            // Check for format 4 header
            if (content.starts_with(HEADER_FORMAT4))
            {
                return std::make_unique<regfile_format4_importer>(content, options);
            }
            // UTF-8 BOM + format 5
            if (content.size() >= 3 &&
                static_cast<unsigned char>(content[0]) == 0xEF &&
                static_cast<unsigned char>(content[1]) == 0xBB &&
                static_cast<unsigned char>(content[2]) == 0xBF)
            {
                if (content.substr(3).starts_with(HEADER_FORMAT5))
                {
                    return std::make_unique<regfile_format5_importer>(content.substr(3), options);
                }
                if (content.substr(3).starts_with(HEADER_FORMAT4))
                {
                    return std::make_unique<regfile_format4_importer>(content.substr(3), options);
                }
            }
            return nullptr;
        }

        /// Read file and create appropriate importer.
        /// @param filename Path to .REG file
        /// @param options Import options
        /// @return Importer instance, or nullptr if file can't be read or format not recognized
        inline std::unique_ptr<regfile_importer> create_importer_from_file(
            std::string_view filename,
            import_options options = import_options::none)
        {
            std::string content = text_file::read_auto(filename);
            if (content.empty())
            {
                return nullptr;
            }
            return create_importer_from_string(content, options);
        }

        // =====================================================================
        // Live Registry Importer
        // =====================================================================

        /// Importer that reads from the live Windows registry.
        class registry_importer final : public import_interface
        {
        public:
            PNQ_DECLARE_NON_COPYABLE(registry_importer)

            /// Create importer for the given registry path.
            /// @param root_path Full registry path (e.g., "HKEY_CURRENT_USER\\Software\\MyApp")
            explicit registry_importer(std::string_view root_path)
                : m_root_path{root_path},
                  m_result{nullptr}
            {
            }

            ~registry_importer()
            {
                if (m_result)
                {
                    PNQ_RELEASE(m_result);
                }
            }

            /// Import from the live registry.
            /// @return Root key entry (caller must release), or nullptr on failure
            key_entry* import() override
            {
                if (m_result)
                {
                    PNQ_ADDREF(m_result);
                    return m_result;
                }

                // Create root entry
                m_result = PNQ_NEW key_entry{nullptr, m_root_path};

                // Open the registry key
                key reg_key{m_root_path};
                if (!reg_key.open_for_reading())
                {
                    // Key doesn't exist - return empty tree
                    PNQ_ADDREF(m_result);
                    return m_result;
                }

                // Import recursively
                import_recursive(m_result, reg_key);

                PNQ_ADDREF(m_result);
                return m_result;
            }

        private:
            void import_recursive(key_entry* parent, key& reg_key)
            {
                // Import all subkeys
                for (const auto& subkey_path : reg_key.enum_keys())
                {
                    // Extract just the name from the full path
                    std::string subkey_name{string::split_at_last_occurence(subkey_path, '\\').second};

                    key_entry* entry = parent->find_or_create_key(subkey_name);

                    key subkey{subkey_path};
                    if (subkey.open_for_reading())
                    {
                        import_recursive(entry, subkey);
                    }
                }

                // Import all values
                for (const auto& val : reg_key.enum_values())
                {
                    if (val.is_default_value())
                    {
                        *parent->default_value() = val;
                    }
                    else
                    {
                        value* entry_val = parent->find_or_create_value(val.name());
                        *entry_val = val;
                    }
                }
            }

            std::string m_root_path;
            key_entry* m_result;
        };

    } // namespace regis3
} // namespace pnq
