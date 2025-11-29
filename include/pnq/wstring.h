#pragma once

#include <string>
#include <vector>

namespace pnq
{
    namespace wstring
    {
        /// compare two wide strings, ignoring case
        /// @param string_a first string
        /// @param string_b second string
        /// @return true if they are equal or false if they are not
        inline bool equals_nocase(std::wstring_view string_a, std::wstring_view string_b)
        {
            if (string_a.empty())
                return string_b.empty();

            if (string_b.empty())
                return false;

            return _wcsicmp(string_a.data(), string_b.data()) == 0;
        }

        /// compare two wide strings, case-sensitive
        /// @param string_a first string
        /// @param string_b second string
        /// @return true if they are equal or false if they are not
        inline bool equals(std::wstring_view string_a, std::wstring_view string_b)
        {
            if (string_a.empty())
                return string_b.empty();

            if (string_b.empty())
                return false;

            return wcscmp(string_a.data(), string_b.data()) == 0;
        }

    } // namespace wstring
} // namespace pnq