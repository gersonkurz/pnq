#pragma once

#include <ngbtools/win32/registry/regfile_import_options.h>
#include <ngbtools/win32/registry/regfile_parser.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            class key_entry;

            /** \brief   This is the import interface supported by all ngbt::registry importer classes: get data from somewhere. */
            interface regfile_import_interface
            {
                virtual key_entry* import() = 0;
                virtual ~regfile_import_interface()
                {
                }
            };

            class regfile_importer : public regfile_import_interface
            {
            protected:
                regfile_importer(const std::string& content, const std::string& expected_header, REGFILE_IMPORT_OPTIONS import_options)
                    :
                    m_content(content),
                    m_parser(expected_header, import_options),
                    m_result(nullptr)
                {
                }
                virtual ~regfile_importer();

            private:
                virtual key_entry* import() override;
            
            private:
                key_entry* m_result;
                regfile_parser m_parser;
                const std::string m_content;
            };

            class regfile_format4_importer : public regfile_importer
            {
            public:
                static const std::string HEADER;

                regfile_format4_importer(const std::string& content, REGFILE_IMPORT_OPTIONS import_options)
                    :
                    regfile_importer(content, HEADER, import_options)
                {
                }
            };

            class regfile_format5_importer : public regfile_importer
            {
            public:
                static const std::string HEADER;

                regfile_format5_importer(const std::string& content, REGFILE_IMPORT_OPTIONS import_options)
                    :
                    regfile_importer(content, HEADER, import_options)
                {
                }
            };
        }
    }
}
