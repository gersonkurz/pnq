#pragma once

#include <string>

namespace pnq
{
    namespace wstring
    {
        // No equals() - use operator== on std::wstring_view directly

        /// Compare two wide strings, ignoring case.
        /// @param a first string
        /// @param b second string
        /// @return true if equal (case-insensitive), false otherwise
        inline bool equals_nocase(std::wstring_view a, std::wstring_view b)
        {
            if (a.size() != b.size())
                return false;
            return _wcsnicmp(a.data(), b.data(), a.size()) == 0;
        }
    }
}
