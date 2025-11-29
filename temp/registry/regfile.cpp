#include <ngbtools/ngbtools.h>
#include <ngbtools/win32/registry/regfile_importer.h>
#include <ngbtools/win32/registry/regfile.h>
#include <ngbtools/string.h>
#include <ngbtools/win32/wstr_param.h>
#include <ngbtools/logging.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            regfile_import_interface* create_importer_from_file(const std::string& filename, REGFILE_IMPORT_OPTIONS import_options)
            {
                // note: on Windows, the .REG format uses ANSI encoding, not UTF8
                return create_importer_from_string(string::from_textfile(filename, CP_ACP), import_options);
            }
            
            regfile_import_interface* create_importer_from_string(const std::string& content, REGFILE_IMPORT_OPTIONS import_options)
            {
                if (string::starts_with(content, regfile_format4_importer::HEADER))
                {
                    return DBG_NEW regfile_format4_importer(content, import_options);
                }
                if (string::starts_with(content, regfile_format5_importer::HEADER))
                {
                    return DBG_NEW regfile_format5_importer(content, import_options);
                }
                logging::error("Unsupported .REG file format detected");
                return nullptr;
            }

        }
    } // namespace win32
} // namespace ngbt
    
