#pragma once

#include <pnq/platform.h>
#include <string>
#include <vector>
#include <charconv>
#include <cctype>
#include <cstring>

#ifdef PNQ_PLATFORM_WINDOWS
#include <Windows.h>
#else
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace pnq
{
    namespace string
    {
        constexpr std::string_view newline = "\r\n";

        /// Check if a C string is null or empty.
        inline bool is_empty(const char *p)
        {
            return !p || !*p;
        }

        /// Check if a wide C string is null or empty.
        inline bool is_empty(const wchar_t *p)
        {
            return !p || !*p;
        }

        /// Check if a string_view is empty.
        inline bool is_empty(std::string_view txt)
        {
            return txt.empty();
        }

        /// Check if a wstring_view is empty.
        inline bool is_empty(std::wstring_view txt)
        {
            return txt.empty();
        }

        /// Convert nibble (0-15) to hex character ('0'-'F').
        inline char hex_digit(uint8_t nibble)
        {
            assert(nibble < 16);
            return "0123456789ABCDEF"[nibble];
        }

        /// Get upper nibble (high 4 bits) of a byte.
        inline uint8_t upper_nibble(unsigned char c)
        {
            return (c & 0xF0) >> 4;
        }

        /// Get lower nibble (low 4 bits) of a byte.
        inline uint8_t lower_nibble(unsigned char c)
        {
            return (c & 0x0F);
        }

        /// Get length of C string, returning 0 for nullptr.
        inline size_t length(const char *text)
        {
            return text ? strlen(text) : 0;
        }

        /// Get length of wide C string, returning 0 for nullptr.
        inline size_t length(const wchar_t *text)
        {
            return text ? wcslen(text) : 0;
        }

        /// Get length of string_view.
        inline size_t length(std::string_view text)
        {
            return text.size();
        }

        /// Get length of wstring_view.
        inline size_t length(std::wstring_view text)
        {
            return text.size();
        }

        /// String equality (prefer using operator== directly on string_view).
        inline bool equals(std::string_view a, std::string_view b)
        {
            return a == b;
        }

        /// Wide string equality (prefer using operator== directly on wstring_view).
        inline bool equals(std::wstring_view a, std::wstring_view b)
        {
            return a == b;
        }

        /// Case-insensitive comparison of wide strings.
        /// @param a first string
        /// @param b second string
        /// @return true if equal (case-insensitive)
        inline bool equals_nocase(std::wstring_view a, std::wstring_view b)
        {
            if (a.size() != b.size())
                return false;
            return _wcsnicmp(a.data(), b.data(), a.size()) == 0;
        }

        /// Case-insensitive comparison of strings.
        /// @param a first string
        /// @param b second string
        /// @return true if equal (case-insensitive)
        inline bool equals_nocase(std::string_view a, std::string_view b)
        {
            if (a.size() != b.size())
                return false;
            return _strnicmp(a.data(), b.data(), a.size()) == 0;
        }

        /// Join strings with a separator.
        inline std::string join(const std::vector<std::string> &items, std::string_view joiner)
        {
            std::string combined;
            bool first = true;
            for (const auto &item : items)
            {
                if (first)
                    first = false;
                else
                    combined += joiner;
                combined += item;
            }
            return combined;
        }

        /// Convert string to uppercase (ASCII only).
        inline std::string uppercase(std::string_view text)
        {
            std::string result{text};
#ifdef PNQ_PLATFORM_WINDOWS
            _strupr_s(result.data(), result.size() + 1);
#else
            for (auto &c : result)
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
#endif
            return result;
        }

        /// Convert UTF-8 string to uppercase (Unicode-aware, locale-sensitive).
        /// @param text UTF-8 encoded input string
        /// @return uppercase UTF-8 string
        inline std::string uppercase_unicode(std::string_view text)
        {
            if (text.empty())
                return {};

#ifdef PNQ_PLATFORM_WINDOWS
            // UTF-8 -> UTF-16
            int wide_len = MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), nullptr, 0);
            if (wide_len <= 0)
                return std::string{text};

            std::wstring wide(wide_len, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), wide.data(), wide_len);

            // Uppercase
            int result_len = LCMapStringEx(LOCALE_NAME_USER_DEFAULT, LCMAP_UPPERCASE, wide.data(), wide_len, nullptr, 0, nullptr, nullptr, 0);
            if (result_len <= 0)
                return std::string{text};

            std::wstring upper(result_len, L'\0');
            LCMapStringEx(LOCALE_NAME_USER_DEFAULT, LCMAP_UPPERCASE, wide.data(), wide_len, upper.data(), result_len, nullptr, nullptr, 0);

            // UTF-16 -> UTF-8
            int utf8_len = WideCharToMultiByte(CP_UTF8, 0, upper.data(), result_len, nullptr, 0, nullptr, nullptr);
            if (utf8_len <= 0)
                return std::string{text};

            std::string result(utf8_len, '\0');
            WideCharToMultiByte(CP_UTF8, 0, upper.data(), result_len, result.data(), utf8_len, nullptr, nullptr);
            return result;
#else
            CFStringRef cfstr = CFStringCreateWithBytes(nullptr, (const UInt8 *)text.data(), text.size(), kCFStringEncodingUTF8, false);
            if (!cfstr)
                return std::string{text};

            CFMutableStringRef mutable_str = CFStringCreateMutableCopy(nullptr, 0, cfstr);
            CFRelease(cfstr);
            if (!mutable_str)
                return std::string{text};

            CFStringUppercase(mutable_str, nullptr);

            CFIndex len = CFStringGetMaximumSizeForEncoding(CFStringGetLength(mutable_str), kCFStringEncodingUTF8) + 1;
            std::string result(len, '\0');
            if (!CFStringGetCString(mutable_str, result.data(), len, kCFStringEncodingUTF8))
            {
                CFRelease(mutable_str);
                return std::string{text};
            }
            result.resize(std::strlen(result.c_str()));

            CFRelease(mutable_str);
            return result;
#endif
        }

        /// Convert string to lowercase (ASCII only).
        inline std::string lowercase(std::string_view text)
        {
            std::string result{text};
#ifdef PNQ_PLATFORM_WINDOWS
            _strlwr_s(result.data(), result.size() + 1);
#else
            for (auto &c : result)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
#endif
            return result;
        }

        /// Convert UTF-8 string to lowercase (Unicode-aware, locale-sensitive).
        /// @param text UTF-8 encoded input string
        /// @return lowercase UTF-8 string
        inline std::string lowercase_unicode(std::string_view text)
        {
            if (text.empty())
                return {};

#ifdef PNQ_PLATFORM_WINDOWS
            // UTF-8 -> UTF-16
            int wide_len = MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), nullptr, 0);
            if (wide_len <= 0)
                return std::string{text};

            std::wstring wide(wide_len, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), wide.data(), wide_len);

            // Lowercase
            int result_len = LCMapStringEx(LOCALE_NAME_USER_DEFAULT, LCMAP_LOWERCASE, wide.data(), wide_len, nullptr, 0, nullptr, nullptr, 0);
            if (result_len <= 0)
                return std::string{text};

            std::wstring lower(result_len, L'\0');
            LCMapStringEx(LOCALE_NAME_USER_DEFAULT, LCMAP_LOWERCASE, wide.data(), wide_len, lower.data(), result_len, nullptr, nullptr, 0);

            // UTF-16 -> UTF-8
            int utf8_len = WideCharToMultiByte(CP_UTF8, 0, lower.data(), result_len, nullptr, 0, nullptr, nullptr);
            if (utf8_len <= 0)
                return std::string{text};

            std::string result(utf8_len, '\0');
            WideCharToMultiByte(CP_UTF8, 0, lower.data(), result_len, result.data(), utf8_len, nullptr, nullptr);
            return result;
#else
            CFStringRef cfstr = CFStringCreateWithBytes(nullptr, (const UInt8 *)text.data(), text.size(), kCFStringEncodingUTF8, false);
            if (!cfstr)
                return std::string{text};

            CFMutableStringRef mutable_str = CFStringCreateMutableCopy(nullptr, 0, cfstr);
            CFRelease(cfstr);
            if (!mutable_str)
                return std::string{text};

            CFStringLowercase(mutable_str, nullptr);

            CFIndex len = CFStringGetMaximumSizeForEncoding(CFStringGetLength(mutable_str), kCFStringEncodingUTF8) + 1;
            std::string result(len, '\0');
            if (!CFStringGetCString(mutable_str, result.data(), len, kCFStringEncodingUTF8))
            {
                CFRelease(mutable_str);
                return std::string{text};
            }
            result.resize(std::strlen(result.c_str()));

            CFRelease(mutable_str);
            return result;
#endif
        }

        /// Split string by separator characters, optionally handling quoted strings.
        inline std::vector<std::string> split(std::string_view svtext, std::string_view svseparators, bool handle_quotation_marks = false)
        {
            std::vector<std::string> result;

            if (svtext.empty())
                return result;

            // Note: assumes null-terminated input for simplicity
            auto text = svtext.data();
            bool is_recording_quoted_string = false;
            auto start = text;
            for (;;)
            {
                char c = *(text++);
                if (!c)
                {
                    if (*start)
                    {
                        result.push_back(start);
                    }
                    break;
                }
                if (is_recording_quoted_string)
                {
                    if (c == '"')
                    {
                        assert(text - start >= 1);
                        result.push_back(std::string{start, (size_t)(text - start - 1)});
                        start = text;
                        is_recording_quoted_string = false;
                    }
                    continue;
                }
                else if (handle_quotation_marks and (c == '"'))
                {
                    assert(text - start >= 1);
                    if (text - start > 1)
                    {
                        result.push_back(std::string{start, (size_t)(text - start - 1)});
                    }
                    start = text;
                    is_recording_quoted_string = true;
                    continue;
                }

                // Check if c is one of the separators
                if (svseparators.find(c) != std::string_view::npos)
                {
                    assert(text - start >= 1);
                    result.push_back(std::string{start, (size_t)(text - start - 1)});
                    start = text;
                }
            }
            return result;
        }

        /// UTF-16 to UTF-8 conversion using Windows API.
        /// @param string_to_encode the UTF-16 string to encode
        /// @return the UTF-8 encoded string
        inline std::string encode_as_utf8(std::wstring_view string_to_encode)
        {
            if (string_to_encode.empty())
                return {};

            const int required_size =
                WideCharToMultiByte(CP_UTF8, 0, string_to_encode.data(), static_cast<int>(string_to_encode.size()), nullptr, 0, nullptr, nullptr);

            if (required_size <= 0)
                return {};

            constexpr size_t stack_buffer_size{1024};
            if (required_size < stack_buffer_size)
            {
                char stack_buffer[stack_buffer_size];
                const int rc = WideCharToMultiByte(
                    CP_UTF8, 0, string_to_encode.data(), static_cast<int>(string_to_encode.size()), stack_buffer, required_size, nullptr, nullptr);

                if (rc > 0)
                {
                    return std::string(stack_buffer, rc);
                }
            }

            std::string result(required_size, '\0');
            const int rc = WideCharToMultiByte(
                CP_UTF8, 0, string_to_encode.data(), static_cast<int>(string_to_encode.size()), result.data(), required_size, nullptr, nullptr);

            if (rc <= 0)
                return {};

            return result;
        }

        /// UTF-16 to target codepage conversion using Windows API.
        /// @param string_to_encode the UTF-16 string to encode
        /// @param codepage target codepage (e.g., CP_ACP for system ANSI)
        /// @return the encoded string in the target codepage
        inline std::string encode_to_codepage(std::wstring_view string_to_encode, UINT codepage)
        {
            if (string_to_encode.empty())
                return {};

            const int required_size =
                WideCharToMultiByte(codepage, 0, string_to_encode.data(), static_cast<int>(string_to_encode.size()), nullptr, 0, nullptr, nullptr);

            if (required_size <= 0)
                return {};

            constexpr size_t stack_buffer_size{1024};
            if (required_size < stack_buffer_size)
            {
                char stack_buffer[stack_buffer_size];
                const int rc = WideCharToMultiByte(
                    codepage, 0, string_to_encode.data(), static_cast<int>(string_to_encode.size()), stack_buffer, required_size, nullptr, nullptr);

                if (rc > 0)
                {
                    return std::string(stack_buffer, rc);
                }
            }

            std::string result(required_size, '\0');
            const int rc = WideCharToMultiByte(
                codepage, 0, string_to_encode.data(), static_cast<int>(string_to_encode.size()), result.data(), required_size, nullptr, nullptr);

            if (rc <= 0)
                return {};

            return result;
        }

        /// UTF-8 (or other codepage) to UTF-16 conversion using Windows API.
        inline std::wstring encode_as_utf16(std::string_view utf8_encoded_text, UINT codepage = CP_UTF8)
        {
            if (utf8_encoded_text.empty())
                return {};

            const auto len = static_cast<int>(utf8_encoded_text.size());
            const auto data = utf8_encoded_text.data();

            // Try stack buffer first for performance
            constexpr int stack_buffer_size = 1024;
            if (len < stack_buffer_size)
            {
                wchar_t buffer[stack_buffer_size];
                int rc = MultiByteToWideChar(codepage, 0, data, len, buffer, stack_buffer_size);
                if (rc > 0)
                {
                    return std::wstring(buffer, rc);
                }
                // Fall through to heap allocation if buffer too small
            }

            // Query required size
            int required_size = MultiByteToWideChar(codepage, 0, data, len, nullptr, 0);
            if (required_size <= 0)
                return {};

            std::wstring result(required_size, L'\0');
            int rc = MultiByteToWideChar(codepage, 0, data, len, result.data(), required_size);
            if (rc <= 0)
                return {};

            return result;
        }

        /// Escape a string for JSON output (includes surrounding quotes).
        inline std::string escape_json_string(std::string_view input)
        {
            std::string output;
            output.reserve(input.size() + 2);
            output.push_back('"');
            for (const char c : input)
            {
                switch (c)
                {
                case '"':  output += "\\\""; break;
                case '\\': output += "\\\\"; break;
                case '\b': output += "\\b"; break;
                case '\f': output += "\\f"; break;
                case '\r': output += "\\r"; break;
                case '\n': output += "\\n"; break;
                case '\t': output += "\\t"; break;
                default:   output.push_back(c); break;
                }
            }
            output.push_back('"');
            return output;
        }

        // For starts_with/ends_with, use std::string_view::starts_with() / ends_with() (C++20)

        /// Strip leading characters from string.
        /// @param text the input string
        /// @param chars characters to strip (default: whitespace)
        /// @return string_view with leading chars removed
        inline std::string_view lstrip(std::string_view text, std::string_view chars = " \t\r\n")
        {
            auto pos = text.find_first_not_of(chars);
            return pos == std::string_view::npos ? std::string_view{} : text.substr(pos);
        }

        /// Strip trailing characters from string.
        /// @param text the input string
        /// @param chars characters to strip (default: whitespace)
        /// @return string_view with trailing chars removed
        inline std::string_view rstrip(std::string_view text, std::string_view chars = " \t\r\n")
        {
            auto pos = text.find_last_not_of(chars);
            return pos == std::string_view::npos ? std::string_view{} : text.substr(0, pos + 1);
        }

        /// Strip leading and trailing characters from string.
        /// @param text the input string
        /// @param chars characters to strip (default: whitespace)
        /// @return string_view with leading and trailing chars removed
        inline std::string_view strip(std::string_view text, std::string_view chars = " \t\r\n")
        {
            return rstrip(lstrip(text, chars), chars);
        }

        /// Split string by separator characters, stripping each element.
        /// @param svtext the input string to split
        /// @param svseparators characters that act as separators
        /// @param handle_quotation_marks if true, quoted strings are preserved as single elements
        /// @param strip_chars characters to strip from each element (default: whitespace)
        /// @return vector of stripped string elements
        inline std::vector<std::string> split_stripped(std::string_view svtext, std::string_view svseparators, bool handle_quotation_marks = false, std::string_view strip_chars = " \t\r\n")
        {
            auto result = split(svtext, svseparators, handle_quotation_marks);
            for (auto &item : result)
            {
                item = strip(item, strip_chars);
            }
            return result;
        }

        /// Case-insensitive starts_with check.
        inline bool starts_with_nocase(std::string_view a, std::string_view b)
        {
            if (b.empty())
                return true;
            if (a.size() < b.size())
                return false;
            return _strnicmp(a.data(), b.data(), b.size()) == 0;
        }

        /// Parse hex string to uint32_t.
        inline bool from_hex_string(std::string_view text, uint32_t &result)
        {
            auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), result, 16);
            return ec == std::errc{} && ptr == text.data() + text.size();
        }

        // For single char to string, use std::string(1, c)

        /**
         * Find the relative start of a string.
         * @param text the input string
         * @param start_pos positive = from start, negative = from end
         * @return pointer to position, or nullptr if out of bounds
         */
        inline const char *find_relative_start(const char *text, int64_t start_pos)
        {
            if (!text)
                return nullptr;

            auto n = static_cast<int64_t>(length(text));

            if (start_pos < 0)
            {
                if (start_pos < -n)
                    return nullptr;
                return text + n + start_pos;
            }
            else
            {
                if (start_pos >= n)
                    return nullptr;
                return text + start_pos;
            }
        }

        /// Extract substring with relative positioning.
        /// @param text input string
        /// @param start_pos start position (negative = from end)
        /// @param len maximum length to extract
        inline std::string substring(const char *text, int64_t start_pos, uint64_t len)
        {
            const char *start = find_relative_start(text, start_pos);
            if (!start)
                return {};

            const char *stop = find_relative_start(start, len);
            if (stop)
                return std::string(start, stop - start);
            return std::string(start);
        }

        /// Extract substring from position to end.
        inline std::string substring(const char *text, int64_t start_pos)
        {
            const char *result = find_relative_start(text, start_pos);
            return result ? std::string(result) : std::string();
        }

        /// Split string at first occurrence of character.
        /// @return pair of (before, after), or (str, "") if not found
        inline std::pair<std::string, std::string> split_at_first_occurence(std::string_view str, char c)
        {
            auto pos = str.find(c);
            if (pos == std::string_view::npos)
                return {std::string(str), std::string()};
            return {std::string(str.substr(0, pos)), std::string(str.substr(pos + 1))};
        }

        /// Split string at first occurrence of substring.
        /// @return pair of (before, after), or (str, "") if not found
        inline std::pair<std::string, std::string> split_at_first_occurence(std::string_view str, std::string_view substr)
        {
            auto pos = str.find(substr);
            if (pos == std::string_view::npos)
                return {std::string(str), std::string()};
            return {std::string(str.substr(0, pos)), std::string(str.substr(pos + substr.size()))};
        }

        /// Split string at last occurrence of character.
        /// @return pair of (before, after), or ("", str) if not found
        inline std::pair<std::string, std::string> split_at_last_occurence(std::string_view str, char c)
        {
            auto pos = str.rfind(c);
            if (pos == std::string_view::npos)
                return {std::string(), std::string(str)};
            return {std::string(str.substr(0, pos)), std::string(str.substr(pos + 1))};
        }

        /// Split string at last occurrence of any character in tokens.
        /// @return pair of (before, after), or ("", str) if not found
        inline std::pair<std::string, std::string> split_at_last_occurence(std::string_view str, std::string_view tokens)
        {
            auto pos = str.find_last_of(tokens);
            if (pos == std::string_view::npos)
                return {std::string(), std::string(str)};
            return {std::string(str.substr(0, pos)), std::string(str.substr(pos + 1))};
        }

        /**
         * Python-style string slicing with [:] operator semantics.
         * @param input the input string
         * @param start_index start position (negative = from end)
         * @param stop_index stop position (negative = from end)
         * @return sliced substring
         *
         * Example: slice("foo", 0, 2) => "fo", slice("foo", -2, -1) => "o"
         */
        inline std::string slice(const char *input, int32_t start_index, int32_t stop_index)
        {
            if (is_empty(input))
                return "";

            const auto len = static_cast<int32_t>(length(input));

            if (start_index < 0)
            {
                start_index = len + start_index;
                if (start_index < 0)
                    start_index = 0;
            }

            if (stop_index < 0)
            {
                stop_index = len + stop_index;
                if (stop_index < 0)
                    stop_index = 0;
            }

            if (stop_index <= start_index)
                return "";

            return substring(input, start_index, stop_index - start_index);
        }

        /// Python-style string slicing (std::string overload).
        inline std::string slice(const std::string &input, int32_t start_index, int32_t stop_index)
        {
            return slice(input.c_str(), start_index, stop_index);
        }

        /// Convert from one codepage to UTF-8.
        inline std::string encode_as_utf8(std::string_view input_data, UINT input_codepage)
        {
            std::wstring wide = encode_as_utf16(input_data, input_codepage);
            return encode_as_utf8(wide);
        }

    } // namespace string
} // namespace pnq
