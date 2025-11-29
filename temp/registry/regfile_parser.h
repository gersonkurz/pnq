#pragma once

#include <ngbtools/string.h>
#include <ngbtools/memory_view.h>
#include <ngbtools/abstract_parser.h>
#include <ngbtools/win32/registry/regfile_import_options.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            class key_entry;
            class value;

            

            /** \brief   A regfile parser. */
            class regfile_parser : public abstract_parser
            {
            public:
                regfile_parser(const std::string& expected_header, REGFILE_IMPORT_OPTIONS options);
                virtual ~regfile_parser();

                key_entry* get_result() const;


            private:
                virtual bool cleanup();

                bool allow_variable_names_for_non_string_variables() const
                {
                    return has_flag(m_options, REGFILE_IMPORT_OPTIONS::ALLOW_VARIABLE_NAMES_FOR_NON_STRING_VARIABLES);
                }

                bool allow_semicolon_comments() const
                {
                    return has_flag(m_options, REGFILE_IMPORT_OPTIONS::ALLOW_SEMICOLON_COMMENTS);
                }

                bool allow_hashtag_comments() const
                {
                    return has_flag(m_options, REGFILE_IMPORT_OPTIONS::ALLOW_HASHTAG_COMMENTS);
                }

                bool ignore_whitespaces() const
                {
                    return has_flag(m_options, REGFILE_IMPORT_OPTIONS::IGNORE_WHITESPACES);
                }

                bool is_whitespace(char c) const
                {
                    if (ignore_whitespaces())
                    {
                        return (c == ' ') or (c == '\t');
                    }
                    return false;
                }

                void decode_current_hex_value();
                void decode_current_variable_defined_hex_value();
                bytes create_byte_array_from_buffer();

            private:
                bool mf_expect_header(char c);
                bool mf_carriage_return(char c);
                bool mf_expect_newline(char c);
                bool mf_expect_start_of_line(char c);
                bool mf_expect_comment_until_end_of_line(char c);
                bool mf_expect_value_name_definition(char c);
                bool mf_expect_quoted_char_in_string_value_name_definition(char c);
                bool mf_expect_equal_sign(char c);
                bool mf_expect_start_of_value_definition(char c);
                bool mf_expect_hex_integer_value(char c);
                bool mf_expect_variable_defined_hex_integer_value(char c);
                bool mf_expect_string_value_definition(char c);
                bool mf_expect_quoted_char_in_string_value_definition(char c);
                bool mf_expect_key_path(char c);
                bool mf_expect_start_of_multibyte_value_definition(char c);
                bool mf_expect_multibyte_value_definition(char c);
                bool mf_expect_newline_followed_by_multibyte_value_definition(char c);

            private:
                REGFILE_IMPORT_OPTIONS m_options;
                const std::string m_header_id;
                int32_t m_number_of_closing_brackets_expected;
                key_entry* m_result;
                key_entry* m_current_key;
                value* m_current_value;
                uint32_t m_current_data_kind;
            };
        }
    } 
}
