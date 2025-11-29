#include <ngbtools/ngbtools.h>
#include <ngbtools/string.h>
#include <ngbtools/string_writer.h>
#include <ngbtools/win32/registry/value.h>
#include <ngbtools/win32/registry/key.h>
#include <ngbtools/win32/registry/regfile_exporter.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            const uint32_t REG_UNKNOWN = uint32_t(-1);
            const uint32_t REG_ESCAPED_DWORD = uint32_t(-2);
            const uint32_t REG_ESCAPED_QWORD = uint32_t(-3);

            value::value(key& k, const std::string& name)
                :
                m_name(name),
                m_type(REG_UNKNOWN),
                m_remove_flag(false)
            {
                k.get(name, *this);
            }

            std::string value::as_string() const
            {
                string::writer result;
                regfile_exporter::export_to_writer(this, result);
                return result.as_string();
            }
        }
    } // namespace win32
} // namespace ngbt
