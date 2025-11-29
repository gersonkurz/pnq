#pragma once

namespace pnq
{
    namespace string
    {
        class Writer final
        {
        public:
            Writer()
                : m_write_position{0},
                  m_dynamic_size{0},
                  m_dynamic_buffer{nullptr},
                  m_builtin_buffer{}
            {
            }

            Writer(const Writer &source)
                : m_write_position{0},
                  m_dynamic_size{0},
                  m_dynamic_buffer{nullptr},
                  m_builtin_buffer{}
            {
                assign(source);
            }

            Writer &operator=(const Writer &source)
            {
                assign(source);
                return *this;
            }

            Writer(Writer &&source) noexcept;

            Writer &operator=(Writer &&source) noexcept;

            ~Writer()
            {
                clear();
            }

            bool empty() const
            {
                return (m_write_position == 0);
            }

            std::string as_string() const
            {
                // this is needed to ensure that the string is zero-terminated
                const_cast<Writer *>(this)->append('\0');
                --m_write_position;
                return m_dynamic_buffer ? m_dynamic_buffer : m_builtin_buffer;
            }

            void hexdump(const unsigned char *address, size_t size);

            bool newline()
            {
                return append(string::newline);
            }

            bool append(char c);

            bool append(const char *ps)
            {
                if (!ps || !*ps)
                    return true;

                size_t len = strlen(ps);
                char *wp = ensure_free_space(len);
                if (!wp)
                    return false;

                memcpy(wp, ps, len);
                m_write_position += len;
                return true;
            }

            bool append(std::string_view s)
            {
                return append(s.data());
            }

            bool append_repeated(char c, size_t number_of_times)
            {
                if (!c || !number_of_times)
                    return true;

                char *wp = ensure_free_space(number_of_times);
                if (!wp)
                    return false;

                memset(wp, c, number_of_times);
                m_write_position += number_of_times;
                return true;
            }

            bool append_repeated(std::string_view text, size_t number_of_times)
            {
                if (text.empty() || !number_of_times)
                    return true;

                const size_t length_of_single_string = string::length(text);
                const size_t bytes_required = length_of_single_string * number_of_times;

                char *wp = ensure_free_space(bytes_required);
                if (!wp)
                    return false;

                const char *sp = text.data();
                for (size_t index = 0; index < number_of_times; ++index)
                {
                    memcpy(wp, sp, length_of_single_string);
                    wp += length_of_single_string;
                }

                m_write_position += bytes_required;
                return true;
            }

            bool append_sized_string(const char *ps, size_t len)
            {
                if (!ps || !*ps || !len)
                    return true;

                const size_t actual_len = strlen(ps);
                if (len > actual_len)
                    len = actual_len;

                char *wp = ensure_free_space(len);
                if (!wp)
                    return false;

                memcpy(wp, ps, len);
                m_write_position += len;
                return true;
            }

            template <typename... Args> bool append_formatted(const std::string_view text, Args &&...args)
            {
                return append(std::vformat(text, std::make_format_args(args...)));
            }

            void clear()
            {
                if (m_dynamic_buffer)
                {
                    free(m_dynamic_buffer);
                    m_dynamic_buffer = nullptr;
                    m_dynamic_size = 0;
                }
                m_write_position = 0;
            }

        private:
            void correct(int bytes_written) const;

            char *ensure_free_space(size_t space_needed);

            /**
             * \brief   Assigns the whole contents of a given Writer object (used by both copy constructor and assignment operator)
             *
             * \param   objectSrc   The object source.
             *
             * \return  true if it succeeds, false if it fails.
             */

            bool assign(const Writer &objectSrc);

        private:
            /** \brief   The current write position */
            mutable size_t m_write_position;

            /** \brief   Size of the dynamic memory (if any) */
            size_t m_dynamic_size;

            /** \brief   Buffer for dynamic data. Allocated only if necessary */
            char *m_dynamic_buffer;

            /** \brief   The builtin cache (1024 bytes). */
            char m_builtin_buffer[1024];
        };

        inline std::string multiply(std::string_view text, uint32_t n)
        {
            Writer result;
            result.append_repeated(text, n);
            return result.as_string();
        }

        inline std::string multiply(const char c, uint32_t n)
        {
            Writer result;
            result.append_repeated(c, n);
            return result.as_string();
        }
    } // namespace string
} // namespace pnq
