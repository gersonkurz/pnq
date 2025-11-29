#include <ngbtools/ngbtools.h>
#include <ngbtools/win32/registry/regfile_importer.h>
#include <ngbtools/win32/registry/key_entry.h>
#include <ngbtools/string.h>
#include <ngbtools/win32/wstr_param.h>
#include <ngbtools/logging.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {

            regfile_importer::~regfile_importer()
            {
                if (m_result)
                {
                    NGBT_RELEASE(m_result);
                }
            }

            key_entry* regfile_importer::import() 
            {
                if (!m_result)
                {
                    if (!m_parser.parse_text(m_content))
                        return nullptr;

                    m_result = m_parser.get_result();
                }
                assert(m_result != nullptr);
                NGBT_ADDREF(m_result);
                return m_result;
            }

            const std::string regfile_format4_importer::HEADER = "REGEDIT4";
            const std::string regfile_format5_importer::HEADER = "Windows Registry Editor Version 5.00";
        }
    } // namespace win32
} // namespace ngbt
    
