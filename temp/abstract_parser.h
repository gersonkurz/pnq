#pragma once

#include <ngbtools/string.h>
#include <ngbtools/string_writer.h>
#include <ngbtools/logging.h>

namespace ngbtools
{
    class abstract_parser;

    typedef bool(abstract_parser::*parser_state)(char c);

    inline bool is_utf16le_bom(const unsigned char* p)
    {
        return (p != nullptr) and
            (p[0] == 0xFF) and
            (p[1] == 0xFE);
    }


    class char_buffer
    {
    public:
        std::string as_string() const
        {
            if (m_buffer.empty())
                return std::string();

            return std::string(&m_buffer[0], m_buffer.size());
        }

        void append(char c)
        {
            m_buffer.push_back(c);
        }

        void clear()
        {
            m_buffer.clear();
        }

    private:
        std::vector<char> m_buffer;
    };


    class abstract_parser
    {
    public:
        abstract_parser(parser_state initial_state)
            : 
			m_initial_state(initial_state),
			m_line(0),
			m_column(0),
			m_index(0),
			m_current_state(nullptr),
			m_current_text(nullptr),
			m_syntax_error_raised(false),
            m_completed(false)
        {
        }

        virtual ~abstract_parser()
        {
        }

        bool parse_text(const std::string& text)
        {
			m_last_known_filename.clear();
            return parse_text(text.c_str());
        }

        bool parse_file(const std::string& filename)
        {
            m_last_known_filename = filename;
            return parse_text(string::from_textfile(filename));
        }

    protected:
        template <typename T> bool set_current_state(bool(T::*method)(char c))
        {
            m_current_state = static_cast<parser_state>(method);
            return true;
        }

		template <typename... Args> bool syntax_error(const char* text, const Args& ... args)
        {
            string::writer output;
            output.append_formatted("Parser failed at line {0}, col {1}:\r\n", m_line, m_column);
            if (!string::is_empty(m_last_known_filename))
                output.append_formatted("- in '{0}'\r\n", m_last_known_filename);
            output.append_formatted(text, args...);
            output.append("\r\n");

            uint32_t start_index = m_index;
            while ((start_index > 0) and (m_current_text[start_index] != '\n'))
            {
                --start_index;
            }
            uint32_t stop_index = m_index;
            for (;;)
            {
                char c = m_current_text[stop_index];
                if( (c == 0) or (c == '\r') or (c == '\n'))
                    break;

                ++stop_index;
            }
            output.append(">> ");
            output.append(string::slice(m_current_text, start_index, stop_index - start_index));
            output.append("\r\n");

            std::vector<char> buffer(stop_index - start_index+4, ' ');
            buffer[0] = '>';
            buffer[1] = '>';
            buffer[2 + (m_index - start_index)] = '^';
            buffer[buffer.size() - 1] = '\0';
            output.append(&buffer[0]);
            logging::error(output.as_string());
            m_syntax_error_raised = true;
            return false;
        }

        virtual bool cleanup()
        {
            return true;
        }
    
        bool set_completed()
        {
            m_completed = true;
            return true;
        }

    private:
        bool parse_text(const char* text)
        {
            m_current_text = text;
            m_line = 1;
            m_column = 1;
            m_index = 0;
            m_syntax_error_raised = false;
            m_current_state = m_initial_state;

            for (;;)
            {
                char c = *(text++);

                // special handling for 'c': we must recognize broken files, but at the same time
                // we should not force every handle to check for end-of-string, so:
                if (!c)
                    return cleanup();

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
        }

    protected:
        const parser_state m_initial_state;
        uint32_t m_line;
        uint32_t m_column;
        uint32_t m_index;
        char_buffer m_buffer;

    private:
        parser_state m_current_state;
        const char* m_current_text;
        bool m_syntax_error_raised;
        bool m_completed;
        std::string m_last_known_filename;
        
    private:
        abstract_parser(const abstract_parser& objectSrc) = delete;
        abstract_parser& operator=(const abstract_parser& objectSrc) = delete;
        abstract_parser(abstract_parser&& moveSrc) = delete;
        abstract_parser& operator=(abstract_parser&& moveSrc) = delete;
    };
}

