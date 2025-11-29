#pragma once

#include <sddl.h>

namespace pnq
{
    namespace win32
    {
        class SecurityAttributes final
        {
        public:
            SecurityAttributes() = default;
            ~SecurityAttributes() = default;

        public:
            LPSECURITY_ATTRIBUTES full_access_for_everyone()
            {
                if (!SP_FULL_ACCESS_FOR_EVERYONE)
                {
                    // see https://blogs.technet.microsoft.com/askds/2008/05/07/the-security-descriptor-definition-language-of-love-part-2/
                    // for more details
                    SP_FULL_ACCESS_FOR_EVERYONE = get_from_sddl("D:PAI(A;OIIO;GA;;;WD)(A;CI;FA;;;WD)");
                }
                return SP_FULL_ACCESS_FOR_EVERYONE.get();
            }

            LPSECURITY_ATTRIBUTES default_access()
            {
                return full_access_for_everyone();
            }
            
            static void delete_security_attributes(SECURITY_ATTRIBUTES *sd)
            {
                if (sd)
                {
                    if (sd->lpSecurityDescriptor)
                    {
                        LocalFree(sd->lpSecurityDescriptor);
                    }
                    LocalFree(sd);
                }
            }

            std::shared_ptr<SECURITY_ATTRIBUTES> get_from_sddl(LPCSTR SDDL)
            {
                std::shared_ptr<SECURITY_ATTRIBUTES> result;
                const auto sd = static_cast<SECURITY_ATTRIBUTES *>(LocalAlloc(0, sizeof(SECURITY_ATTRIBUTES)));
                if (sd)
                {

                    sd->nLength = sizeof(SECURITY_ATTRIBUTES);
                    sd->bInheritHandle = FALSE;
                    sd->lpSecurityDescriptor = nullptr;

                    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
                            string::encode_as_utf16(SDDL).c_str(), SDDL_REVISION_1, &sd->lpSecurityDescriptor, nullptr))
                    {

                        logging::report_windows_error(
                            GetLastError(), PNQ_FUNCTION_CONTEXT, std::format("ConvertStringSecurityDescriptorToSecurityDescriptorW({}) failed", SDDL));
                    }
                    else
                    {
                        result = std::shared_ptr<SECURITY_ATTRIBUTES>(sd, delete_security_attributes);
                    }
                }
                return result;
            }

        private:
            std::shared_ptr<SECURITY_ATTRIBUTES> SP_FULL_ACCESS_FOR_EVERYONE;
            std::shared_ptr<SECURITY_ATTRIBUTES> SP_DEFAULT_ACCESS;
        };
    } // namespace win32
} // namespace pnq
