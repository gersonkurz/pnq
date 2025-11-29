#pragma once

#include <string>
#include <Windows.h>

namespace pnq
{
    namespace string
    {
        constexpr std::string_view newline = "\r\n";

        inline bool is_empty(const char *p)
        {
            return !p || !*p;
        }

        inline bool is_empty(std::string_view txt)
        {
            return txt.empty();
        }

        inline bool is_empty(const wchar_t *p)
        {
            return !p || !*p;
        }

        inline bool is_empty(std::wstring_view txt)
        {
            return txt.empty();
        }

        inline char hex_digit(uint8_t nibble)
        {
            assert(nibble < 16);
            return "0123456789ABCDEF"[nibble];
        }

        inline uint8_t upper_nibble(unsigned char c)
        {
            return (c & 0xF0) >> 4;
        }

        inline uint8_t lower_nibble(unsigned char c)
        {
            return (c & 0x0F);
        }

        inline size_t length(const char *text)
        {
            return text ? strlen(text) : 0;
        }

        inline size_t length(const wchar_t *text)
        {
            return text ? wcslen(text) : 0;
        }

        inline size_t length(const std::string &text)
        {
            return text.length();
        }

        inline size_t length(const std::wstring &text)
        {
            return text.length();
        }

        inline size_t length(std::string_view text)
        {
            return text.length();
        }

        inline size_t length(std::wstring_view text)
        {
            return text.length();
        }

        inline bool equals(char a, char b)
        {
            return a == b;
        }

        inline bool equals(wchar_t a, wchar_t b)
        {
            return a == b;
        }

        inline bool equals(std::wstring_view a, std::wstring_view b)
        {
            if (a.empty())
                return b.empty();

            if (b.empty())
                return false;

            return wcscmp(a.data(), b.data()) == 0;
        }

        inline bool equals_nocase(std::wstring_view a, std::wstring_view b)
        {
            if (a.empty())
                return b.empty();

            if (b.empty())
                return false;

            return _wcsicmp(a.data(), b.data()) == 0;
        }

        inline bool equals(std::string_view a, std::string_view b)
        {
            if (a.empty())
                return b.empty();

            if (b.empty())
                return false;

            return strcmp(a.data(), b.data()) == 0;
        }

        inline bool equals_nocase(std::string_view a, std::string_view b)
        {
            if (a.empty())
                return b.empty();

            if (b.empty())
                return false;

            return _stricmp(a.data(), b.data()) == 0;
        }

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

        inline const std::string uppercase(std::string_view text)
        {
            std::string result{text};
            _strupr_s(const_cast<char *>(result.c_str()), result.size() + 1);
            return result;
        }

        inline std::string lowercase(std::string_view text)
        {
            std::string result{text};
            _strlwr_s(const_cast<char *>(result.c_str()), result.size() + 1);
            return result;
        }

        inline std::vector<std::string> split(std::string_view svtext, std::string_view svseparators, bool handle_quotation_marks = false)
        {
            std::vector<std::string> result;

            if (auto text = svtext.data())
            {
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

                    if (std::strchr(svseparators.data(), c))
                    {
                        assert(text - start >= 1);
                        result.push_back(std::string{start, (size_t)(text - start - 1)});
                        start = text;
                    }
                }
            }
            return result;
        }

        // UTF-16 to UTF-8 conversion
        // Uses Windows API WideCharToMultiByte
        // @param string_to_encode the UTF-16 string to encode
        // @return the UTF-8 encoded string
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

        inline std::wstring encode_as_utf16(std::string_view utf8_encoded_text, UINT codepage = CP_UTF8)
        {
            const auto utf8_len = utf8_encoded_text.size();
            const auto utf8_encoded_string = utf8_encoded_text.data();

            // handle nullptr gracefully
            if (!utf8_encoded_string || !utf8_len)
                return {};

            // we try the stack first (because that is of an order of magnitudes faster)
            // only if this fails we revert back to allocating the data on the stack
            constexpr size_t size_of_stack_buffer = 1024;
            if (utf8_len < size_of_stack_buffer)
            {
                wchar_t buffer_on_the_stack[size_of_stack_buffer];
                int rc = MultiByteToWideChar(codepage, 0, utf8_encoded_string, (int)utf8_len, buffer_on_the_stack, size_of_stack_buffer - 1);
                if (rc > 0)
                {
                    assert(rc < sizeof(buffer_on_the_stack));
                    buffer_on_the_stack[rc] = 0;
                    return std::wstring(buffer_on_the_stack);
                }
                rc = GetLastError();

                // we don't expect these
                assert(rc != ERROR_INVALID_FLAGS);
                assert(rc != ERROR_INVALID_PARAMETER);
                assert(rc != ERROR_NO_UNICODE_TRANSLATION);

                // we do expect this
                assert(rc == ERROR_INSUFFICIENT_BUFFER);
            }
            size_t size_to_allocate = utf8_len + 16;
            const size_t max_size_to_allocate{64 * size_to_allocate};
            while (max_size_to_allocate)
            {
                std::vector<wchar_t> buffer_on_the_heap;
                buffer_on_the_heap.resize(size_to_allocate + 1);

                int rc = MultiByteToWideChar(CP_UTF8, 0, utf8_encoded_string, (int)utf8_len, &buffer_on_the_heap[0], (int)size_to_allocate);

                if (rc > 0)
                {
                    assert(size_t(rc) < size_to_allocate);
                    buffer_on_the_heap[rc] = 0;
                    return std::wstring{&buffer_on_the_heap[0], size_t(rc)};
                }
                size_to_allocate *= 2;
            }
            assert(false);
            return {};
        }

        inline std::string escape_json_string(std::string_view input)
        {
            std::vector<char> output;
            output.push_back('"');
            for (const char c : input)
            {
                if (c == '"')
                {
                    output.insert(output.end(), "\\\"", "\\\"" + 2);
                }
                else if (c == '\\')
                {
                    output.insert(output.end(), "\\\\", "\\\\" + 2);
                }
                else if (c == '/')
                {
                    output.insert(output.end(), "\\/", "\\/" + 2);
                }
                else if (c == '\b')
                {
                    output.insert(output.end(), "\\b", "\\b" + 2);
                }
                else if (c == '\f')
                {
                    output.insert(output.end(), "\\f", "\\f" + 2);
                }
                else if (c == '\r')
                {
                    output.insert(output.end(), "\\r", "\\r" + 2);
                }
                else if (c == '\n')
                {
                    output.insert(output.end(), "\\n", "\\n" + 2);
                }
                else if (c == '\t')
                {
                    output.insert(output.end(), "\\t", "\\t" + 2);
                }
                else
                {
                    output.push_back(c);
                }
            }
            output.push_back('"');
            return std::string{output.begin(), output.end()};
        }

        inline bool ends_with(std::string_view a, std::string_view b)
        {
            // if a is nullptr, it ends with nothing at all
            // if b is nullptr, it makes no sense to ask for ends-with
            if (a.empty() || b.empty())
                return false;

            const size_t sla{a.size()};
            const size_t slb{b.size()};

            // cannot be a Match: a has less characters than b
            if (sla < slb)
                return false;

            const size_t offset{sla - slb};
            return strcmp(a.data() + offset, b.data()) == 0;
        }

        inline bool starts_with(std::string_view a, char c)
        {
            if (a.empty() || !c)
                return false;

            return *(a.data()) == c;
        }

        inline bool starts_with(std::string_view a, std::string_view b)
        {
            // if a is nullptr, it ends with nothing at all
            // if b is nullptr, it makes no sense to ask for ends-with
            if (a.empty() || b.empty())
                return false;

            const size_t slb{a.size()};
            const size_t sla{a.size()};

            // cannot be a match: a has fewer characters than b
            if (sla < slb)
                return false;

            return strncmp(a.data(), b.data(), slb) == 0;
        }

        inline bool starts_with_nocase(std::string_view a, std::string_view b)
        {
            // if a is nullptr, it ends with nothing at all
            // if b is nullptr, it makes no sense to ask for ends-with
            if (a.empty() || b.empty())
                return false;

            const size_t slb{a.size()};
            const size_t sla{a.size()};

            // cannot be a match: a has fewer characters than b
            if (sla < slb)
                return false;

            return _strnicmp(a.data(), b.data(), slb) == 0;
        }
        inline bool from_hex_string(const std::string &text, uint32_t &result)
        {
            char *eptr = nullptr;
            result = strtoul(text.c_str(), &eptr, 16);
            return *eptr == 0;
        }
        inline std::string as_string(char c)
        {
            char buffer[2] = {c, 0};
            return buffer;
        }
        /**
         * find the relative start of a string.
         *
         * \param	text the input string
         * \param   start_pos the relative position. use positive numbers to indicate relative to startup
         *          of string, and negative numbers to indicate relative to end of string.
         *
         * \return	string representing the number in decimal format.
         */
        inline const char *find_relative_start(const char *text, int64_t start_pos)
        {
            if (!text)
                return nullptr;

            long long n = (long long)string::length(text);

            if (start_pos < 0)
            {
                /*

                Example:

                str:			"ABC"
                len:			3
                pos[-1] = "C" =>  str + len - 1
                pos[-2] = "BC" =>  str + len - 2
                pos[-3] = "ABC" =>  str + len - 3
                pos[-4] = nullptr
                */
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
        inline std::string substring(const char *text, int64_t start_pos, uint64_t len)
        {
            std::string result;
            const char *start = find_relative_start(text, start_pos);
            if (start)
            {
                const char *stop = find_relative_start(start, len);
                if (stop)
                {
                    result = std::string(start, stop - start);
                }
                else
                {
                    result = start;
                }
            }
            return result;
        }
        inline std::string substring(const char *text, int64_t start_pos)
        {
            const char *result = find_relative_start(text, start_pos);
            if (result)
            {
                return result;
            }
            return std::string();
        }
        inline std::pair<std::string, std::string> split_at_first_occurence(const std::string &str, char c)
        {
            if (str.empty())
                return {"", ""};

            const char *text = str.c_str();
            const size_t size_max = str.length();

            for (size_t i = 0; text[i]; ++i)
            {
                if (text[i] == c)
                {
                    if (i == size_max)
                    {
                        return {str, ""};
                    }
                    if (i == 0)
                    {
                        return {"", std::string(text + 1)};
                    }
                    return {std::string(text, i), std::string(text + i + 1)};
                }
            }
            return {std::string(text), std::string()};
        }

        inline std::pair<std::string, std::string> split_at_first_occurence(const std::string &str, const std::string &substr)
        {
            const char *text = str.c_str();

            size_t n = string::length(substr);
            assert(n > 0);

            for (size_t i = 0; text[i]; ++i)
            {
                // this is not good...
                if (strncmp(text + i, substr.c_str(), n) == 0)
                {
                    return {std::string(text, i), std::string(text + i + 1)};
                }
            }
            return {std::string(text), std::string()};
        }

        inline std::pair<std::string, std::string> split_at_last_occurence(const char *text, const char *tokens)
        {
            size_t last_found = 0;
            bool last_found_valid = false;

            for (size_t i = 0; text[i]; ++i)
            {
                if (strchr(tokens, text[i]))
                {
                    last_found = i;
                    last_found_valid = true;
                }
            }
            if (last_found_valid)
            {
                return {std::string(text, last_found), std::string(text + last_found + 1)};
            }
            else
            {
                return {std::string(), std::string(text)};
            }
        }
        inline std::pair<std::string, std::string> split_at_last_occurence(const char *text, char c)
        {
            size_t last_found = 0;
            bool last_found_valid = false;

            for (size_t i = 0; text[i]; ++i)
            {
                if (text[i] == c)
                {
                    last_found = i;
                    last_found_valid = true;
                }
            }
            if (last_found_valid)
            {
                return {std::string(text, last_found), std::string(text + last_found + 1)};
            }
            else
            {
                return {std::string(), std::string(text)};
            }
        }
        inline std::pair<std::string, std::string> split_at_last_occurence(const std::string &text, char c)
        {
            return split_at_last_occurence(text.c_str(), c);
        }
        /**
         * String slicing, modelled after the Python with the [:] slicing operator on strings
         *
         * \param	input	   	The input string.
         * \param	start_index	The starting index can be positive or negative. Negative
         * 						indices start from the back of the string, so for example
         * 						-1 is the last character in the string.
         * \param	stop_index 	The stop index, again positive or negative.
         *
         * \return	A std::string with the proper substring.
         *
         * Example Usage:
         * \code
         * 		slice("foo",0,2) ==> "fo"
         * 		slice("foo",-2,-1) == "o"
         * \endcode
         */

        inline std::string slice(const char *input, int32_t start_index, int32_t stop_index)
        {
            if (is_empty(input))
                return "";

            const auto len = truncate_cast<int32_t>(string::length(input));

            if (start_index < 0)
            {
                start_index = len + start_index;
                if (start_index < 0)
                {
                    start_index = 0;
                }
            }

            if (stop_index < 0)
            {
                stop_index = len + stop_index;
                if (stop_index < 0)
                {
                    stop_index = 0;
                }
            }

            // return empty string
            if (stop_index <= start_index)
                return "";

            return string::substring(input, start_index, stop_index - start_index);
        }

        /**
         * String slicing, modelled after the Python with the [:] slicing operator on strings
         *
         * \param	input	   	The input string pointer
         * \param	start_index	The starting index can be positive or negative. Negative
         * 						indices start from the back of the string, so for example
         * 						-1 is the last character in the string.
         * \param	stop_index 	The stop index, again positive or negative.
         *
         * \return	A std::string with the proper substring.
         *
         * Example Usage:
         * \code
         * 		slice("foo",0,2) ==> "fo"
         * 		slice("foo",-2,-1) == "o"
         * \endcode
         */
        inline std::string slice(const std::string &input, int32_t start_index, int32_t stop_index)
        {
            return slice(input.c_str(), start_index, stop_index);
        }

        std::string encode_as_utf8(std::string_view input_data, UINT input_codepage)
        {
            std::wstring wide = encode_as_utf16(input_data, input_codepage);
            int required_size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.length()), nullptr, 0, nullptr, nullptr);
            if (required_size <= 0)
            {
                return std::string{};
            }
            std::string utf8_string(static_cast<size_t>(required_size), 0);
            WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.length()), utf8_string.data(), required_size, nullptr, nullptr);
            return utf8_string;
        }

    } // namespace string
} // namespace pnq
