#pragma once

/// @file pnq/registry/parser.h
/// @brief State machine parser infrastructure and .REG file parser

#include <pnq/registry/types.h>
#include <pnq/registry/value.h>
#include <pnq/registry/key_entry.h>
#include <pnq/string.h>
#include <pnq/string_writer.h>
#include <pnq/text_file.h>
#include <pnq/pnq.h>

#include <functional>
#include <vector>
#include <cassert>

namespace pnq
{
    namespace registry
    {
        // =====================================================================
        // Abstract Parser Base
        // =====================================================================

        /// Check if a buffer starts with UTF-16LE BOM.
        /// @param p Pointer to at least 2 bytes
        /// @return true if BOM detected
        inline bool is_utf16le_bom(const unsigned char* p)
        {
            return (p != nullptr) && (p[0] == 0xFF) && (p[1] == 0xFE);
        }

        class abstract_parser;

        /// Parser state function pointer type.
        using parser_state = bool (abstract_parser::*)(char c);

        /// Base class for state-machine parsers.
        ///
        /// Provides line/column tracking, buffer management, and error reporting.
        /// Derived classes implement state handlers as member functions.
        class abstract_parser
        {
        public:
            /// Construct with initial state.
            /// @param initial_state Member function pointer to initial state handler
            explicit abstract_parser(parser_state initial_state)
                : m_initial_state{initial_state},
                  m_line{0},
                  m_column{0},
                  m_index{0},
                  m_current_state{nullptr},
                  m_current_text{nullptr},
                  m_syntax_error_raised{false},
                  m_completed{false}
            {
            }

            virtual ~abstract_parser() = default;

            PNQ_DECLARE_NON_COPYABLE(abstract_parser)

            /// Parse text content.
            /// @param text String content to parse
            /// @return true if parsing succeeded
            bool parse_text(std::string_view text)
            {
                m_last_known_filename.clear();
                return parse_text_impl(text);
            }

            /// Parse a file.
            /// @param filename Path to file to parse
            /// @return true if parsing succeeded
            bool parse_file(std::string_view filename)
            {
                m_last_known_filename = filename;
                std::string content = text_file::read_auto(filename);
                return parse_text_impl(content);
            }

        protected:
            /// Set the current parser state.
            /// @tparam T Derived class type
            /// @param method State handler member function
            /// @return true (for chaining)
            template <typename T>
            bool set_current_state(bool (T::*method)(char c))
            {
                m_current_state = static_cast<parser_state>(method);
                return true;
            }

            /// Report a syntax error with context.
            /// @param fmt Format string
            /// @param args Format arguments
            /// @return false (for return from state handler)
            template <typename... Args>
            bool syntax_error(std::string_view fmt, const Args&... args)
            {
                string::Writer output;
                output.append(std::format("Parser failed at line {}, col {}:\r\n", m_line, m_column));

                if (!m_last_known_filename.empty())
                {
                    output.append(std::format("- in '{}'\r\n", m_last_known_filename));
                }

                // Format the error message
                output.append(std::vformat(fmt, std::make_format_args(args...)));
                output.append("\r\n");

                // Show context line
                uint32_t start_index = m_index;
                while ((start_index > 0) && (m_current_text[start_index] != '\n'))
                {
                    --start_index;
                }
                if (m_current_text[start_index] == '\n')
                    ++start_index;

                uint32_t stop_index = m_index;
                while (m_current_text[stop_index] != 0 &&
                       m_current_text[stop_index] != '\r' &&
                       m_current_text[stop_index] != '\n')
                {
                    ++stop_index;
                }

                output.append(">> ");
                output.append(std::string_view{m_current_text + start_index, stop_index - start_index});
                output.append("\r\n");

                // Show position marker
                output.append(">> ");
                for (uint32_t i = start_index; i < m_index; ++i)
                    output.append(' ');
                output.append("^\r\n");

                spdlog::error("{}", output.as_string());
                m_syntax_error_raised = true;
                return false;
            }

            /// Called after parsing completes successfully.
            /// Override to perform post-processing.
            /// @return true if cleanup succeeded
            virtual bool cleanup()
            {
                return true;
            }

            /// Mark parsing as completed (stop processing).
            /// @return true
            bool set_completed()
            {
                m_completed = true;
                return true;
            }

            /// Get buffer contents as string.
            std::string buffer_as_string() const
            {
                return std::string{m_buffer.begin(), m_buffer.end()};
            }

            /// Append char to buffer.
            void buffer_append(char c)
            {
                m_buffer.push_back(c);
            }

            /// Clear buffer.
            void buffer_clear()
            {
                m_buffer.clear();
            }

        private:
            bool parse_text_impl(std::string_view text)
            {
                m_current_text = text.data();
                m_line = 1;
                m_column = 1;
                m_index = 0;
                m_syntax_error_raised = false;
                m_current_state = m_initial_state;
                m_buffer.clear();

                const char* ptr = text.data();
                const char* end = ptr + text.size();

                while (ptr < end)
                {
                    char c = *ptr++;

                    if (!(this->*m_current_state)(c))
                        return false;

                    if (m_syntax_error_raised)
                        return false;

                    if (m_completed)
                        return cleanup();

                    if (c == '\n')
                    {
                        ++m_line;
                        m_column = 0;
                    }
                    ++m_column;
                    ++m_index;
                }

                return cleanup();
            }

        protected:
            const parser_state m_initial_state;
            uint32_t m_line;
            uint32_t m_column;
            uint32_t m_index;
            std::vector<char> m_buffer;

        private:
            parser_state m_current_state;
            const char* m_current_text;
            bool m_syntax_error_raised;
            bool m_completed;
            std::string m_last_known_filename;
        };

        // =====================================================================
        // REG File Parser
        // =====================================================================

        /// Parser for Windows .REG files (REGEDIT4 and Windows Registry Editor 5.00 formats).
        ///
        /// Uses a state machine to parse registry key and value definitions.
        /// Handles multi-line hex values, escaped strings, comments, and variable references.
        class regfile_parser final : public abstract_parser
        {
        public:
            /// Construct parser with expected header and options.
            /// @param expected_header "REGEDIT4" or "Windows Registry Editor Version 5.00"
            /// @param options Import options flags
            regfile_parser(std::string_view expected_header, import_options options)
                : abstract_parser{static_cast<parser_state>(&regfile_parser::state_expect_header)},
                  m_options{options},
                  m_header_id{expected_header},
                  m_number_of_closing_brackets_expected{0},
                  m_result{PNQ_NEW key_entry()},
                  m_current_key{nullptr},
                  m_current_value{nullptr},
                  m_current_data_kind{REG_TYPE_UNKNOWN}
            {
            }

            ~regfile_parser()
            {
                if (m_result)
                {
                    m_result->release();
                    m_result = nullptr;
                }
            }

            /// Get the parsed result.
            /// Caller receives ownership (reference count is incremented).
            /// @return Root key_entry of parsed content
            key_entry* get_result() const
            {
                if (m_result)
                {
                    m_result->retain();
                }
                return m_result;
            }

        protected:
            bool cleanup() override
            {
                // Recursively unwrap single-child keys with no values until we reach
                // a key with values or multiple children (like the C# version)
                while (m_result &&
                       m_result->keys().size() == 1 &&
                       m_result->values().empty() &&
                       m_result->default_value() == nullptr)
                {
                    auto it = m_result->keys().begin();
                    key_entry* child = it->second;
                    child->retain();
                    m_result->release();
                    m_result = child;
                }
                return true;
            }

        private:
            // Option helpers
            bool allow_variable_names_for_non_string_variables() const
            {
                return has_flag(m_options, import_options::allow_variable_names_for_non_string_variables);
            }

            bool allow_semicolon_comments() const
            {
                return has_flag(m_options, import_options::allow_semicolon_comments);
            }

            bool allow_hashtag_comments() const
            {
                return has_flag(m_options, import_options::allow_hashtag_comments);
            }

            bool ignore_whitespaces() const
            {
                return has_flag(m_options, import_options::ignore_whitespaces);
            }

            bool is_whitespace(char c) const
            {
                if (ignore_whitespaces())
                {
                    return (c == ' ') || (c == '\t');
                }
                return false;
            }

            // Hex helpers
            static bool is_hex_digit(char c)
            {
                return ((c >= '0') && (c <= '9')) ||
                       ((c >= 'A') && (c <= 'F')) ||
                       ((c >= 'a') && (c <= 'f'));
            }

            static std::uint8_t hex_char_to_byte(char c)
            {
                if ((c >= '0') && (c <= '9'))
                    return static_cast<std::uint8_t>(c - '0');
                if ((c >= 'A') && (c <= 'F'))
                    return static_cast<std::uint8_t>(10 + c - 'A');
                if ((c >= 'a') && (c <= 'f'))
                    return static_cast<std::uint8_t>(10 + c - 'a');
                return 0;
            }

            void decode_current_hex_value()
            {
                std::string val = buffer_as_string();
                buffer_clear();

                uint32_t result = 0;
                if (string::from_hex_string(val, result))
                {
                    m_current_value->set_dword(result);
                    set_current_state(&regfile_parser::state_expect_newline);
                }
                else
                {
                    syntax_error("'{}' is not a valid hex integer", val);
                }
            }

            void decode_current_variable_defined_hex_value()
            {
                m_current_value->set_escaped_dword_value(buffer_as_string());
                buffer_clear();
                set_current_state(&regfile_parser::state_expect_newline);
            }

            bytes create_byte_array_from_buffer()
            {
                std::string input = buffer_as_string();
                size_t len = input.size();

                // Pad to even length
                if (len % 2 != 0)
                {
                    input += '0';
                    ++len;
                }

                bytes result;
                result.reserve(len / 2);

                for (size_t i = 0; i < len; i += 2)
                {
                    std::uint8_t high = hex_char_to_byte(input[i]);
                    std::uint8_t low = hex_char_to_byte(input[i + 1]);
                    result.push_back(static_cast<std::uint8_t>((high << 4) | low));
                }

                return result;
            }

            // =========================================================
            // State Handlers
            // =========================================================

            bool state_expect_header(char c)
            {
                if (c == '\r')
                {
                    std::string header = buffer_as_string();
                    if (header != m_header_id)
                    {
                        return syntax_error(".REG file expected header '{}', got '{}' instead",
                                            m_header_id, header);
                    }
                    return set_current_state(&regfile_parser::state_expect_newline);
                }
                else
                {
                    buffer_append(c);
                }
                return true;
            }

            bool state_expect_carriage_return(char c)
            {
                if (c == '\r')
                {
                    return set_current_state(&regfile_parser::state_expect_start_of_line);
                }
                else if ((c == ' ') || (c == '\t'))
                {
                    // ignore trailing whitespace
                }
                else if ((c == '#') || (c == ';'))
                {
                    return set_current_state(&regfile_parser::state_expect_comment_until_end_of_line);
                }
                else
                {
                    return syntax_error("Expected carriage return but got '{}' instead", c);
                }
                return true;
            }

            bool state_expect_newline(char c)
            {
                if (c == '\n')
                {
                    return set_current_state(&regfile_parser::state_expect_start_of_line);
                }
                return true;
            }

            bool state_expect_start_of_line(char c)
            {
                if (c == '\r' || c == '\n')
                {
                    // blank line
                }
                else if (c == '[')
                {
                    buffer_clear();
                    m_number_of_closing_brackets_expected = 0;
                    return set_current_state(&regfile_parser::state_expect_key_path);
                }
                else if (c == '@')
                {
                    m_current_value = m_current_key->find_or_create_value("");
                    return set_current_state(&regfile_parser::state_expect_equal_sign);
                }
                else if (c == '"')
                {
                    buffer_clear();
                    return set_current_state(&regfile_parser::state_expect_value_name_definition);
                }
                else if ((c == '#') && allow_hashtag_comments())
                {
                    buffer_clear();
                    return set_current_state(&regfile_parser::state_expect_comment_until_end_of_line);
                }
                else if ((c == ';') && allow_semicolon_comments())
                {
                    buffer_clear();
                    return set_current_state(&regfile_parser::state_expect_comment_until_end_of_line);
                }
                else
                {
                    return syntax_error("Unexpected character '{}' at start of line", c);
                }
                return true;
            }

            bool state_expect_comment_until_end_of_line(char c)
            {
                if (c == '\n')
                {
                    return set_current_state(&regfile_parser::state_expect_start_of_line);
                }
                return true;
            }

            bool state_expect_value_name_definition(char c)
            {
                if (c == '"')
                {
                    m_current_value = m_current_key->find_or_create_value(buffer_as_string());
                    return set_current_state(&regfile_parser::state_expect_equal_sign);
                }
                else if (c == '\\')
                {
                    return set_current_state(&regfile_parser::state_expect_quoted_char_in_value_name);
                }
                else
                {
                    buffer_append(c);
                }
                return true;
            }

            bool state_expect_quoted_char_in_value_name(char c)
            {
                buffer_append(c);
                return set_current_state(&regfile_parser::state_expect_value_name_definition);
            }

            bool state_expect_equal_sign(char c)
            {
                if (c == '=')
                {
                    buffer_clear();
                    return set_current_state(&regfile_parser::state_expect_start_of_value_definition);
                }
                else if (c == '-')
                {
                    // "value"=- means delete
                    m_current_value->set_remove_flag(true);
                    return set_current_state(&regfile_parser::state_expect_carriage_return);
                }
                else
                {
                    return syntax_error("Expected '=' but got '{}' instead", c);
                }
                return true;
            }

            bool state_expect_start_of_value_definition(char c)
            {
                if (c == '"')
                {
                    buffer_clear();
                    return set_current_state(&regfile_parser::state_expect_string_value_definition);
                }
                else if (c == '-')
                {
                    // @=- means delete default value
                    m_current_value->set_remove_flag(true);
                    return set_current_state(&regfile_parser::state_expect_carriage_return);
                }
                else if (c == ':')
                {
                    std::string type_name = string::lowercase(buffer_as_string());
                    buffer_clear();

                    if (type_name == "dword")
                    {
                        return set_current_state(&regfile_parser::state_expect_hex_integer_value);
                    }
                    else if (type_name.starts_with("hex(") && type_name.ends_with(")"))
                    {
                        // hex(N): format - extract N
                        auto tokens = string::split(type_name, "()");
                        if (tokens.size() >= 2)
                        {
                            uint32_t data_kind = 0;
                            if (!string::from_hex_string(tokens[1], data_kind))
                            {
                                return syntax_error("'{}' is not a valid hex() kind", tokens[1]);
                            }
                            m_current_data_kind = data_kind;
                            return set_current_state(&regfile_parser::state_expect_start_of_multibyte_value);
                        }
                    }
                    else if (type_name == "hex")
                    {
                        m_current_data_kind = REG_BINARY;
                        return set_current_state(&regfile_parser::state_expect_start_of_multibyte_value);
                    }
                    else
                    {
                        return syntax_error("Value type '{}' not supported", type_name);
                    }
                }
                else
                {
                    buffer_append(c);
                }
                return true;
            }

            bool state_expect_hex_integer_value(char c)
            {
                if (c == '\r')
                {
                    decode_current_hex_value();
                }
                else if (is_hex_digit(c))
                {
                    buffer_append(c);
                }
                else if (is_whitespace(c))
                {
                    // ignore
                }
                else if ((c == '#') && allow_hashtag_comments())
                {
                    decode_current_hex_value();
                    return set_current_state(&regfile_parser::state_expect_comment_until_end_of_line);
                }
                else if ((c == ';') && allow_semicolon_comments())
                {
                    decode_current_hex_value();
                    return set_current_state(&regfile_parser::state_expect_comment_until_end_of_line);
                }
                else if ((c == '$') && allow_variable_names_for_non_string_variables())
                {
                    buffer_append(c);
                    return set_current_state(&regfile_parser::state_expect_variable_defined_hex_value);
                }
                else
                {
                    return syntax_error("'{}' is not a valid hex digit", c);
                }
                return true;
            }

            bool state_expect_variable_defined_hex_value(char c)
            {
                if (c == '\r')
                {
                    decode_current_variable_defined_hex_value();
                }
                else if ((c == '#') && allow_hashtag_comments())
                {
                    decode_current_variable_defined_hex_value();
                    return set_current_state(&regfile_parser::state_expect_comment_until_end_of_line);
                }
                else if ((c == ';') && allow_semicolon_comments())
                {
                    decode_current_variable_defined_hex_value();
                    return set_current_state(&regfile_parser::state_expect_comment_until_end_of_line);
                }
                else
                {
                    buffer_append(c);
                }
                return true;
            }

            bool state_expect_string_value_definition(char c)
            {
                if (c == '"')
                {
                    m_current_value->set_string(buffer_as_string());
                    return set_current_state(&regfile_parser::state_expect_carriage_return);
                }
                else if (c == '\\')
                {
                    return set_current_state(&regfile_parser::state_expect_quoted_char_in_string_value);
                }
                else
                {
                    buffer_append(c);
                }
                return true;
            }

            bool state_expect_quoted_char_in_string_value(char c)
            {
                buffer_append(c);
                return set_current_state(&regfile_parser::state_expect_string_value_definition);
            }

            bool state_expect_key_path(char c)
            {
                if (c == '[')
                {
                    ++m_number_of_closing_brackets_expected;
                    buffer_append(c);
                }
                else if (c == ']')
                {
                    if (m_number_of_closing_brackets_expected == 0)
                    {
                        m_current_key = m_result->find_or_create_key(buffer_as_string());
                        return set_current_state(&regfile_parser::state_expect_carriage_return);
                    }
                    else
                    {
                        --m_number_of_closing_brackets_expected;
                        buffer_append(c);
                    }
                }
                else
                {
                    buffer_append(c);
                }
                return true;
            }

            bool state_expect_start_of_multibyte_value(char c)
            {
                if (c == '\r')
                {
                    return state_expect_multibyte_value_definition(c);
                }
                else if (is_hex_digit(c))
                {
                    buffer_append(c);
                    return set_current_state(&regfile_parser::state_expect_multibyte_value_definition);
                }
                else
                {
                    return syntax_error("Expected hex digit at start of binary value");
                }
                return true;
            }

            bool state_expect_multibyte_value_definition(char c)
            {
                if (c == ',')
                {
                    // separator, ignore
                }
                else if (c == '\\')
                {
                    return set_current_state(&regfile_parser::state_expect_newline_followed_by_multibyte);
                }
                else if ((c == ' ') || (c == '\t'))
                {
                    // whitespace in hex data
                }
                else if (c == '\r')
                {
                    m_current_value->set_binary_type(m_current_data_kind, create_byte_array_from_buffer());
                    buffer_clear();
                    return set_current_state(&regfile_parser::state_expect_newline);
                }
                else if (c == '\n')
                {
                    return syntax_error("Got \\n without \\r - registry file is not properly encoded");
                }
                else if (is_hex_digit(c))
                {
                    buffer_append(c);
                }
                else
                {
                    return syntax_error("Unexpected character '{}' in hex data", c);
                }
                return true;
            }

            bool state_expect_newline_followed_by_multibyte(char c)
            {
                if (c == '\r')
                {
                    // ignore
                }
                else if (c == '\n')
                {
                    return set_current_state(&regfile_parser::state_expect_multibyte_value_definition);
                }
                else
                {
                    return syntax_error("Expected newline to follow trailing backslash");
                }
                return true;
            }

        private:
            import_options m_options;
            std::string m_header_id;
            int32_t m_number_of_closing_brackets_expected;
            key_entry* m_result;
            key_entry* m_current_key;
            value* m_current_value;
            uint32_t m_current_data_kind;
        };

    } // namespace registry
} // namespace pnq
