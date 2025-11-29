#pragma once

#include <ngbtools/win32/registry/regfile_export_options.h>
#include <ngbtools/win32/registry/regfile_importer.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            class key_entry;

            interface regfile_export_interface
            {
                virtual bool perform_export(const key_entry* key, REGFILE_EXPORT_OPTIONS export_options) = 0;

                virtual ~regfile_export_interface()
                {
                }
            };

            class regfile_exporter : public regfile_export_interface
            {
            protected:
                regfile_exporter(const std::string& header, UINT target_encoding, const std::string& filename)
                    :
                    m_header(header),
                    m_encoding(target_encoding),
                    m_filename(filename)
                {
                }
                virtual ~regfile_exporter();

            private:
                virtual bool perform_export(const key_entry* key, REGFILE_EXPORT_OPTIONS export_options) override;

            private:
                friend class value;
                friend class key_entry;

                bool export_to_writer(const key_entry* key, string::writer& output, REGFILE_EXPORT_OPTIONS export_options);
                static bool export_to_writer_recursive(const key_entry* key, string::writer& output, bool no_empty_keys);
                static bool export_to_writer(const value* value, string::writer& output);
                static bool write_hex_encoded_value(const value* value, string::writer& output);
                static std::string reg_escape_string(const std::string& input);

            private:
                UINT m_encoding;
                const std::string m_header;
                const std::string m_filename;
            };

            class regfile_format4_exporter : public regfile_exporter
            {
            public:
                regfile_format4_exporter(const std::string& filename)
                    :
                    regfile_exporter(regfile_format4_importer::HEADER, CP_ACP, filename)
                {
                }
            };

            class regfile_format5_exporter : public regfile_exporter
            {
            public:
                regfile_format5_exporter(const std::string& filename)
                    :
                    regfile_exporter(regfile_format5_importer::HEADER, CP_UTF8, filename)
                {
                }
            };
  
            class registry_exporter : public regfile_export_interface
            {
            public:
                registry_exporter();
                virtual ~registry_exporter();

            public:
                virtual bool perform_export(const key_entry* key, REGFILE_EXPORT_OPTIONS export_options) override;
            private:
                bool export_recursive(const key_entry* key, bool no_empty_keys);
            };
        }
    }
}

