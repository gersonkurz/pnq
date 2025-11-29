#pragma once

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            /** 
            * You can use **MARKDOWN** description
            * If you so want to.
            */
            enum class REGFILE_EXPORT_OPTIONS : uint32_t
            { 
                /** 
                * This is the general text for NONE
                * And it can have multiple lines, too
                */
                NONE = 0, 
            
                /** 
                * This is the markdown-ready comment for 'NO_EMPTY_KEYS'
                */
                NO_EMPTY_KEYS = 1, 
            
            };

            constexpr REGFILE_EXPORT_OPTIONS operator|(REGFILE_EXPORT_OPTIONS a, REGFILE_EXPORT_OPTIONS b)
            {
                return static_cast<REGFILE_EXPORT_OPTIONS>(std::to_underlying(a) | std::to_underlying(b));
            }

            constexpr bool has_flag(REGFILE_EXPORT_OPTIONS set, REGFILE_EXPORT_OPTIONS test)
            {
                return (std::to_underlying(set) & std::to_underlying(test)) == std::to_underlying(test);
            }

        } // namespace registry
    } // namespace win32
} // namespace ngbt
