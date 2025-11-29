#include <ngbtools/ngbtools.h>
#include <ngbtools/string.h>
#include <ngbtools/abstract_parser.h>
#include <ngbtools/logging.h>
#include <ngbtools/win32/registry/regfile_parser.h>
#include <ngbtools/win32/registry/key_entry.h>
#include <ngbtools/win32/registry/value.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            regfile_parser::regfile_parser(const std::string& expected_header, REGFILE_IMPORT_OPTIONS options)
                :
                abstract_parser(static_cast<parser_state>(&regfile_parser::mf_expect_header)),
                m_options(options),
                m_header_id(expected_header),
                m_number_of_closing_brackets_expected(0),
                m_current_value(nullptr),
                m_current_key(nullptr),
                m_result(DBG_NEW key_entry()),
                m_current_data_kind(REG_UNKNOWN)
            {
            }

            regfile_parser::~regfile_parser()
            {
                NGBT_RELEASE(m_result);
            }
            key_entry* regfile_parser::get_result() const
            {
                NGBT_ADDREF(m_result);
                return m_result;
            }
            bool regfile_parser::cleanup()
            {
                if ((m_result->m_keys.size() == 1) and (m_result->m_values.empty()))
                {
                    for (auto item : m_result->m_keys)
                    {
                        key_entry* p = item.second;
                        NGBT_ADDREF(p);
                        NGBT_RELEASE(m_result);
                        m_result = p;
                    }
                }

                return true;
            }

            bool regfile_parser::mf_expect_header(char c)
            {
                if (c == '\r')
                {
                    std::string header = m_buffer.as_string();
                    if (!string::equals(header, m_header_id))
                    {
                        return syntax_error(".REG file expected header '{0}', got '{1}' instead",
                            m_header_id,
                            header);
                    }
                    set_current_state(&regfile_parser::mf_expect_newline);
                }
                else
                {
                    m_buffer.append(c);
                }
                return true;
            }

            bool regfile_parser::mf_carriage_return(char c)
            {
                if (c == '\r')
                {
                    set_current_state(&regfile_parser::mf_expect_start_of_line);
                }
                else if ((c == ' ') or (c == '\t'))
                {

                }
                else if ((c == '#') or (c == ';'))
                {
                    set_current_state(&regfile_parser::mf_expect_comment_until_end_of_line);
                }
                else
                {
                    return syntax_error("Expected carriage return but got '{0}' instead", c);
                }
                return true;
            }

            bool regfile_parser::mf_expect_newline(char c)
            {
                if (c == '\n')
                {
                    set_current_state(&regfile_parser::mf_expect_start_of_line);
                }
                return true;
            }

            bool regfile_parser::mf_expect_start_of_line(char c)
            {
                if (c == '\r')
                {
                }
                else if (c == '\n')
                {
                }
                else if (c == '[')
                {
                    m_buffer.clear();
                    m_number_of_closing_brackets_expected = 0;
                    set_current_state(&regfile_parser::mf_expect_key_path);
                }
                else if (c == '@')
                {
                    m_current_value = m_current_key->find_or_create_value("");
                    set_current_state(&regfile_parser::mf_expect_equal_sign);
                }
                else if (c == '"')
                {
                    m_buffer.clear();
                    set_current_state(&regfile_parser::mf_expect_value_name_definition);
                }
                else if ((c == '#') and allow_hashtag_comments())
                {
                    m_buffer.clear();
                    set_current_state(&regfile_parser::mf_expect_comment_until_end_of_line);
                }
                else if ((c == ';') and allow_semicolon_comments())
                {
                    m_buffer.clear();
                    set_current_state(&regfile_parser::mf_expect_comment_until_end_of_line);
                }
                else
                {
                    return syntax_error("Parser does not support values yet; '{0}'", c);
                }
                return true;
            }

            bool regfile_parser::mf_expect_comment_until_end_of_line(char c)
            {
                if (c == 0)
                {
                    set_current_state(&regfile_parser::mf_expect_start_of_line);
                }
                return true;
            }

            bool regfile_parser::mf_expect_value_name_definition(char c)
            {
                if (c == '"')
                {
                    m_current_value = m_current_key->find_or_create_value(m_buffer.as_string());
                    set_current_state(&regfile_parser::mf_expect_equal_sign);
                }
                else if (c == '\\')
                {
                    set_current_state(&regfile_parser::mf_expect_quoted_char_in_string_value_name_definition);
                }
                else
                {
                    m_buffer.append(c);
                }
                return true;
            }

            bool regfile_parser::mf_expect_quoted_char_in_string_value_name_definition(char c)
            {
                m_buffer.append(c);
                set_current_state(&regfile_parser::mf_expect_value_name_definition);
                return true;
            }

            bool regfile_parser::mf_expect_equal_sign(char c)
            {
                if (c == '=')
                {
                    m_buffer.clear();
                    set_current_state(&regfile_parser::mf_expect_start_of_value_definition);
                }
                else
                {
                    return syntax_error("Parser expected '=', got '{0}' instead.", c);
                }
                return true;
            }

            bool regfile_parser::mf_expect_start_of_value_definition(char c)
            {
                if (c == '"')
                {
                    set_current_state(&regfile_parser::mf_expect_string_value_definition);
                    m_buffer.clear();
                }
                else if (c == ':')
                {
                    std::string type_name = string::lowercase(m_buffer.as_string());
                    m_buffer.clear();
                    if (string::equals(type_name, "dword"))
                    {
                        set_current_state(&regfile_parser::mf_expect_hex_integer_value);
                    }
                    else if (string::starts_with(type_name,"hex(") && string::ends_with(type_name, ")"))
                    {
                        std::vector<std::string> tokens(string::split(type_name, "()"));
                        if (tokens.size() >= 2)
                        {
                            uint32_t data_kind = 0;
                            if (!string::from_hex_string(tokens[1], data_kind))
                            { 
                                return syntax_error("'{0}' is not a valid hex() kind ", tokens[1]);
                            }

                            m_current_data_kind = data_kind;
                            set_current_state(&regfile_parser::mf_expect_start_of_multibyte_value_definition);
                        }
                    }
                    else if (string::equals(type_name, "hex"))
                    {
                        m_current_data_kind = REG_BINARY;
                        set_current_state(&regfile_parser::mf_expect_start_of_multibyte_value_definition);
                    }
                    else
                    {
                        return syntax_error("Value type '{0}' not supported", type_name);
                    }
                }
                else
                {
                    m_buffer.append(c);
                }
                return true;
            }

            void regfile_parser::decode_current_hex_value()
            {
                std::string value(m_buffer.as_string());
                m_buffer.clear();

                uint32_t result = 0;
                if(string::from_hex_string(value, result))
                {
                    m_current_value->set_dword(result);
                    set_current_state(&regfile_parser::mf_expect_newline);
                }
                else
                {
                    syntax_error("'{0}' is not a valid hex integer", value);
                }
            }

            static inline bool is_hex_digit(char c)
            {
                return ((c >= '0') and (c <= '9')) or
                    ((c >= 'A') and (c <= 'F')) or
                    ((c >= 'a') and (c <= 'f'));
            }

            bool regfile_parser::mf_expect_hex_integer_value(char c)
            {
                if (c == '\r')
                {
                    decode_current_hex_value();
                }
                else if (is_hex_digit(c))
                {
                    m_buffer.append(c);
                }
                else if (is_whitespace(c))
                {
                }
                else if ((c == '#') and allow_hashtag_comments())
                {
                    decode_current_hex_value();
                    set_current_state(&regfile_parser::mf_expect_comment_until_end_of_line);
                }
                else if ((c == ';') and allow_hashtag_comments())
                {
                    decode_current_hex_value();
                    set_current_state(&regfile_parser::mf_expect_comment_until_end_of_line);
                }
                else if ((c == '$') and allow_variable_names_for_non_string_variables())
                {
                    m_buffer.append(c);
                    set_current_state(&regfile_parser::mf_expect_variable_defined_hex_integer_value);
                }
                else
                {
                    return syntax_error("'{0}' is not a valid hex digit", c);
                }
                return true;
            }

            void regfile_parser::decode_current_variable_defined_hex_value()
            {
                m_current_value->set_escaped_dword_value(m_buffer.as_string());
                set_current_state(&regfile_parser::mf_expect_newline);
                m_buffer.clear();
            }

            bool regfile_parser::mf_expect_variable_defined_hex_integer_value(char c)
            {
                if (c == '\r')
                {
                    decode_current_variable_defined_hex_value();
                }
                else if ((c == '#') and allow_hashtag_comments())
                {
                    decode_current_variable_defined_hex_value();
                    set_current_state(&regfile_parser::mf_expect_comment_until_end_of_line);
                }
                else if ((c == ';') and allow_hashtag_comments())
                {
                    decode_current_variable_defined_hex_value();
                    set_current_state(&regfile_parser::mf_expect_comment_until_end_of_line);
                }
                else
                {
                    m_buffer.append(c);
                }
                return true;
            }

            bool regfile_parser::mf_expect_string_value_definition(char c)
            {
                if (c == '"')
                {
                    m_current_value->set_string(m_buffer.as_string());
                    set_current_state(&regfile_parser::mf_carriage_return);
                }
                else if (c == '\\')
                {
                    set_current_state(&regfile_parser::mf_expect_quoted_char_in_string_value_definition);
                }
                else
                {
                    m_buffer.append(c);
                }
                return true;
            }

            bool regfile_parser::mf_expect_quoted_char_in_string_value_definition(char c)
            {
                m_buffer.append(c);
                set_current_state(&regfile_parser::mf_expect_string_value_definition);
                return true;
            }

            bool regfile_parser::mf_expect_key_path(char c)
            {
                if (c == '[')
                {
                    ++m_number_of_closing_brackets_expected;
                    m_buffer.append(c);
                }
                else if (c == ']')
                {
                    if (0 == m_number_of_closing_brackets_expected)
                    {
                        m_current_key = m_result->find_or_create_key(m_buffer.as_string());
                        set_current_state(&regfile_parser::mf_carriage_return);
                    }
                    else if (m_number_of_closing_brackets_expected > 0)
                    {
                        --m_number_of_closing_brackets_expected;
                        m_buffer.append(c);
                    }
                    else
                    {
                        return syntax_error("Too many closing square brackets");
                    }
                }
                else
                {
                    m_buffer.append(c);
                }
                return true;
            }
        
            bool regfile_parser::mf_expect_start_of_multibyte_value_definition(char c)
            {
                if (c == '\r')
                {
                    return mf_expect_multibyte_value_definition(c);
                }
                else
                {
                    uint32_t data_kind = 0;
                    if (!string::from_hex_string(string::as_string(c), data_kind))
                    {
                        return syntax_error("expected byte-oriented hex(2) definition");
                    }

                    m_buffer.append(c);
                    set_current_state(&regfile_parser::mf_expect_multibyte_value_definition);
                }
                return true;
            }



            bool regfile_parser::mf_expect_multibyte_value_definition(char c)
            {
                if (c == ',')
                {
                }
                else if (c == '\\')
                {
                    set_current_state(&regfile_parser::mf_expect_newline_followed_by_multibyte_value_definition);
                }
                else if ((c == ' ') or (c == '\t'))
                {
                }
                else if (c == '\r')
                {
                    m_current_value->set_binary_type(m_current_data_kind, create_byte_array_from_buffer());
                    m_buffer.clear();
                    set_current_state(&regfile_parser::mf_expect_newline);
                }
                else if (c == '\n')
                {
                    return syntax_error("got \\n without \\r - registry file is not properly encoded");
                }
                else
                {
                    m_buffer.append(c);
                }
                return true;
            }

            static inline byte hex_char_to_byte(char c)
            {
                if ((c >= '0') and (c <= '9'))
                {
                    return byte(c - '0');
                }
                if ((c >= 'A') and (c <= 'F'))
                {
                    return 10 + byte(c - 'A');
                }
                if ((c >= 'a') and (c <= 'f'))
                {
                    return 10 + byte(c - 'a');
                }
                assert(false);
                return 0;
            }

            bytes regfile_parser::create_byte_array_from_buffer()
            {
                // this is a string, and it will be decode as bytes 
                std::string input(m_buffer.as_string());

                size_t len_in_chars = string::length(input);
                if(len_in_chars % 2)
                {
                    string::writer temp;
                    temp.append(input);
                    temp.append('0');
                    input = temp.as_string();
                    ++len_in_chars;
                    assert(string::length(input) == len_in_chars);
                }
                assert((len_in_chars % 2) == 0);
                bytes result;
                result.resize(len_in_chars / 2);
                int j = 0;

                const char* m = input.c_str();
                byte value = 0;
                for (size_t i = 0; i < len_in_chars; ++i)
                {
                    if ((i % 2) == 0)
                    {
                        value = m[i];
                        value <<= 4;
                    }
                    else
                    {
                        value |= m[i];
                        result[j++] = value;
                    }
                }
                return result;
            }

            bool regfile_parser::mf_expect_newline_followed_by_multibyte_value_definition(char c)
            {
                if (c == '\r')
                {
                }
                else if (c == '\n')
                {
                    set_current_state(&regfile_parser::mf_expect_multibyte_value_definition);
                }
                else
                {
                    return syntax_error("expected newline to follow trailing backslash");
                }
                return true;
            }

        }
    }
}
