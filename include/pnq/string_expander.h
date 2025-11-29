#pragma once

#include <string>
#include <unordered_map>
#include <pnq/string_writer.h>
#include <pnq/environment_variables.h>

namespace pnq
{
    namespace string
    {
        /// Expands %VARIABLE% style placeholders in strings.
        /// Variables are looked up first in a provided map, then in environment variables.
        /// Use %% to escape a literal percent sign.
        class Expander final
        {
            enum class RECORDING_PATTERN
            {
                PLAINTEXT,
                ENVIRONMENT_VARIABLE,
            };

        public:
            Expander()
                : m_variables{nullptr},
                  m_use_environment_variables{true}
            {
            }

            ~Expander() = default;

            /// Construct with a variable lookup map.
            /// @param variables map of variable names to values
            /// @param use_environment_variables whether to fall back to environment variables
            Expander(const std::unordered_map<std::string, std::string> &variables, bool use_environment_variables = true)
                : m_variables{&variables},
                  m_use_environment_variables{use_environment_variables}
            {
            }

            Expander(const Expander &) = delete;
            Expander &operator=(const Expander &) = delete;
            Expander(Expander &&) = delete;
            Expander &operator=(Expander &&) = delete;

        public:
            /// Expand all %VARIABLE% patterns in the input string.
            /// @param string_to_expand input string with placeholders
            /// @return expanded string with variables substituted
            std::string expand(std::string_view string_to_expand) const
            {
                if (string_to_expand.empty())
                    return {};

                const char *text{string_to_expand.data()};

                auto recording_pattern{RECORDING_PATTERN::PLAINTEXT};
                bool is_first_char_after_start_of_pattern{false};
                Writer output;
                Writer pattern;

                for (; text;)
                {
                    const char c{*(text++)};

                    if (!c)
                    {
                        if (recording_pattern == RECORDING_PATTERN::ENVIRONMENT_VARIABLE)
                        {
                            output.append('%');
                            output.append(pattern.as_string());
                        }
                        break;
                    }
                    if (c == '%')
                    {
                        if (recording_pattern == RECORDING_PATTERN::PLAINTEXT) // start recording
                        {
                            recording_pattern = RECORDING_PATTERN::ENVIRONMENT_VARIABLE;
                            is_first_char_after_start_of_pattern = true;
                        }
                        else
                        {
                            assert(recording_pattern == RECORDING_PATTERN::ENVIRONMENT_VARIABLE);
                            if (is_first_char_after_start_of_pattern)
                            {
                                output.append('%');
                            }
                            else
                            {
                                const auto variable{pattern.as_string()};
                                std::string replacement;
                                if (!locate_variable(variable, replacement))
                                {
                                    output.append("%");
                                    output.append(variable);
                                    output.append("%");
                                }
                                else
                                {
                                    output.append(replacement);
                                }
                                pattern.clear();
                            }
                            recording_pattern = RECORDING_PATTERN::PLAINTEXT;
                        }
                    }
                    else if (recording_pattern != RECORDING_PATTERN::PLAINTEXT)
                    {
                        pattern.append(c);
                        is_first_char_after_start_of_pattern = false;
                    }
                    else
                    {
                        output.append(c);
                    }
                }
                return output.as_string();
            }

        private:
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
