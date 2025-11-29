#pragma once

#include <string>
#include <unordered_map>
#include <pnq/string_writer.h>
#include <pnq/environment_variables.h>

namespace pnq
{
    namespace string
    {
        /// Expands variable placeholders in strings.
        /// Supports %VARIABLE% (Windows-style) and ${VARIABLE} (Unix-style) syntax.
        /// Variables are looked up first in a provided map, then in environment variables.
        /// Use %% to escape a literal percent sign, $$ to escape a literal dollar sign.
        class Expander final
        {
        public:
            Expander()
                : m_variables{nullptr},
                  m_use_environment_variables{true},
                  m_expand_percent{true},
                  m_expand_dollar{false}
            {
            }

            ~Expander() = default;

            /// Construct with a variable lookup map.
            /// @param variables map of variable names to values
            /// @param use_environment_variables whether to fall back to environment variables
            Expander(const std::unordered_map<std::string, std::string> &variables, bool use_environment_variables = true)
                : m_variables{&variables},
                  m_use_environment_variables{use_environment_variables},
                  m_expand_percent{true},
                  m_expand_dollar{false}
            {
            }

            Expander(const Expander &) = delete;
            Expander &operator=(const Expander &) = delete;
            Expander(Expander &&) = delete;
            Expander &operator=(Expander &&) = delete;

            /// Enable or disable ${VAR} syntax expansion.
            /// @param enable true to enable, false to disable
            /// @return reference to this for chaining
            Expander &expand_dollar(bool enable)
            {
                m_expand_dollar = enable;
                return *this;
            }

            /// Enable or disable %VAR% syntax expansion.
            /// @param enable true to enable, false to disable
            /// @return reference to this for chaining
            Expander &expand_percent(bool enable)
            {
                m_expand_percent = enable;
                return *this;
            }

            /// Expand all variable patterns in the input string.
            /// @param string_to_expand input string with placeholders
            /// @return expanded string with variables substituted
            std::string expand(std::string_view string_to_expand) const
            {
                if (string_to_expand.empty())
                    return {};

                const char *text{string_to_expand.data()};
                const char *end{text + string_to_expand.size()};
                Writer output;

                while (text < end)
                {
                    const char c{*text};

                    if (m_expand_percent && c == '%')
                    {
                        text = expand_percent_var(text, end, output);
                    }
                    else if (m_expand_dollar && c == '$')
                    {
                        text = expand_dollar_var(text, end, output);
                    }
                    else
                    {
                        output.append(c);
                        ++text;
                    }
                }
                return output.as_string();
            }

        private:
            /// Expand %VAR% syntax starting at text position.
            /// @return pointer to next character after the expanded pattern
            const char *expand_percent_var(const char *text, const char *end, Writer &output) const
            {
                ++text; // skip initial %

                if (text >= end)
                {
                    output.append('%');
                    return text;
                }

                // %% escape sequence
                if (*text == '%')
                {
                    output.append('%');
                    return text + 1;
                }

                // find closing %
                const char *var_start{text};
                while (text < end && *text != '%')
                    ++text;

                if (text >= end)
                {
                    // no closing %, output literally
                    output.append('%');
                    output.append(std::string_view{var_start, static_cast<size_t>(text - var_start)});
                    return text;
                }

                // got closing %
                std::string_view var_name{var_start, static_cast<size_t>(text - var_start)};
                ++text; // skip closing %

                std::string replacement;
                if (locate_variable(var_name, replacement))
                {
                    output.append(replacement);
                }
                else
                {
                    output.append('%');
                    output.append(var_name);
                    output.append('%');
                }
                return text;
            }

            /// Expand ${VAR} syntax starting at text position.
            /// @return pointer to next character after the expanded pattern
            const char *expand_dollar_var(const char *text, const char *end, Writer &output) const
            {
                ++text; // skip initial $

                if (text >= end)
                {
                    output.append('$');
                    return text;
                }

                // $$ escape sequence
                if (*text == '$')
                {
                    output.append('$');
                    return text + 1;
                }

                // must be ${
                if (*text != '{')
                {
                    output.append('$');
                    return text; // don't consume the char, let main loop handle it
                }

                ++text; // skip {

                // find closing }
                const char *var_start{text};
                while (text < end && *text != '}')
                    ++text;

                if (text >= end)
                {
                    // no closing }, output literally
                    output.append("${");
                    output.append(std::string_view{var_start, static_cast<size_t>(text - var_start)});
                    return text;
                }

                // got closing }
                std::string_view var_name{var_start, static_cast<size_t>(text - var_start)};
                ++text; // skip closing }

                std::string replacement;
                if (locate_variable(var_name, replacement))
                {
                    output.append(replacement);
                }
                else
                {
                    output.append("${");
                    output.append(var_name);
                    output.append('}');
                }
                return text;
            }

            bool locate_variable(std::string_view variable, std::string &result) const
            {
                if (const auto p{locate_variable(variable)})
                {
                    result = p;
                    return true;
                }
                if (!m_use_environment_variables)
                    return false;

                if (environment_variables::get(variable, result))
                {
                    return true;
                }

                return false;
            }

            const char *locate_variable(std::string_view variable) const
            {
                if (m_variables)
                {
                    const std::string key{variable};
                    const auto item{m_variables->find(key)};
                    if (item != m_variables->end())
                    {
                        return item->second.c_str();
                    }
                }
                return nullptr;
            }

            const std::unordered_map<std::string, std::string> *m_variables;
            const bool m_use_environment_variables;
            bool m_expand_percent;
            bool m_expand_dollar;
        };
    } // namespace string

    namespace environment_variables
    {
        /// Get the expanded text of an environment variable, allowing for additional local context-variable substitution
        ///
        /// @param text environment variable to expand
        /// @param variables lookup map with additional local context-variables
        /// @param use_environment_variables flag indicating if environment variables should be included (defaults to true)
        /// @return expanded environment variable
        inline auto expand(std::string_view text, const std::unordered_map<std::string, std::string> &variables, bool use_environment_variables = true)
        {
            return string::Expander{variables, use_environment_variables}.expand(text);
        }

        /// Get the expanded text of an environment variable.
        ///
        /// @param text environment variable to expand
        /// @return expanded environment variable
        inline auto expand(std::string_view text)
        {
            return string::Expander().expand(text);
        }
    } // namespace environment_variables
} // namespace pnq
