#pragma once

namespace pnq
{
    namespace string
    {
        /// Small buffer optimized string builder.
        /// Uses a 1024-byte stack buffer for small strings, dynamically allocates for larger ones.
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

            Writer(Writer &&source) noexcept
                : m_write_position{source.m_write_position},
                  m_dynamic_size{source.m_dynamic_size},
                  m_dynamic_buffer{source.m_dynamic_buffer},
                  m_builtin_buffer{}
            {
                source.m_write_position = 0;
                source.m_dynamic_size = 0;
                source.m_dynamic_buffer = nullptr;

                // if (and only if) the origin still uses the cache, we need to do so as well
                if (!m_dynamic_buffer)
                {
                    assert(m_write_position < std::size(m_builtin_buffer));
                    memcpy(m_builtin_buffer, source.m_builtin_buffer, m_write_position);
                }
            }

            Writer &operator=(Writer &&source) noexcept
            {
                assert(this != &source);
                if (m_dynamic_buffer)
                {
                    free(m_dynamic_buffer);
                }

                m_write_position = source.m_write_position;
                source.m_write_position = 0;

                m_dynamic_size = source.m_dynamic_size;
                source.m_dynamic_size = 0;

                m_dynamic_buffer = source.m_dynamic_buffer;
                source.m_dynamic_buffer = nullptr;

                // if (and only if) the origin still uses the cache, we need to do so as well
                if (!m_dynamic_buffer)
                {
                    assert(m_write_position < std::size(m_builtin_buffer));
                    memcpy(m_builtin_buffer, source.m_builtin_buffer, m_write_position);
                }
                return *this;
            }

            ~Writer()
            {
                clear();
            }

            /// Check if the writer is empty.
            bool empty() const
            {
                return (m_write_position == 0);
            }

            /// Convert contents to std::string.
            std::string as_string() const
            {
                // this is needed to ensure that the string is zero-terminated
                const_cast<Writer *>(this)->append('\0');
                --m_write_position;
                return m_dynamic_buffer ? m_dynamic_buffer : m_builtin_buffer;
            }

            /// Write a hex dump of memory to the writer.
            /// @param address pointer to memory to dump
            /// @param size number of bytes to dump
            void hexdump(const unsigned char *address, size_t size)
            {
                constexpr unsigned NIBBLES_PER_DWORD = 8;
                constexpr unsigned DWORDS_PER_LINE = 5;
                constexpr unsigned BYTES_PER_LINE = DWORDS_PER_LINE * 4;
#ifdef _M_X64
                constexpr unsigned START_OF_DWORDS = 17; // first 8 chars are the address, followed by a single blank
                constexpr unsigned START_OF_CTEXT = 17 + (DWORDS_PER_LINE * (NIBBLES_PER_DWORD + 1)) - 1;
#else
                constexpr unsigned START_OF_DWORDS = 9; // first 8 chars are the address, followed by a single blank
                constexpr unsigned START_OF_CTEXT = 9 + (DWORDS_PER_LINE * (NIBBLES_PER_DWORD + 1)) - 1;
#endif
                if (!address)
                {
                    append("nullptr\r\n");
                    return;
                }
                append_formatted("{0} bytes at {1}:\r\n", size, static_cast<const void *>(address));

                if (!size)
                    return;

                size_t lines = size / BYTES_PER_LINE;
                if (size % BYTES_PER_LINE)
                    ++lines;

                char output[128];

                size_t bytesRemaining = size;
                for (size_t line = 0; line < lines; ++line)
                {
#ifdef _M_X64
                    static_cast<void>(sprintf_s(output, "%016p", address));
#else
                    sprintf_s(output, "%08p", address);
#endif
                    memset(output + START_OF_DWORDS - 1, ' ', sizeof(output) - 1 - START_OF_DWORDS);
                    output[START_OF_DWORDS - 1] = ':';
                    char *wp_bytes = &output[START_OF_DWORDS];
                    char *wp_text = &output[START_OF_CTEXT + 4];

                    size_t index, offset = 0;
                    for (index = 0; index < BYTES_PER_LINE; ++index)
                    {
                        if (!bytesRemaining)
                            break;

                        unsigned char c = address[index];
                        *(wp_bytes++ + offset) = hex_digit(upper_nibble(c));
                        *(wp_bytes++ + offset) = hex_digit(lower_nibble(c));

                        wp_text[index] = isprint(c) ? c : '.';
                        --bytesRemaining;
                        if (index % 4 == 3)
                            offset++;
                    }
                    wp_text[index++ + offset] = '\r';
                    wp_text[index++ + offset] = '\n';
                    assert(index + offset < 100);
                    wp_text[index + offset] = 0;
                    append(output);
                    address += BYTES_PER_LINE;
                }
            }

            /// Append a Windows-style newline (\r\n).
            bool newline()
            {
                return append(string::newline);
            }

            /// Append a single character.
            bool append(char c)
            {
                if (m_dynamic_buffer == nullptr)
                {
                    if (m_write_position < std::size(m_builtin_buffer))
                    {
                        m_builtin_buffer[m_write_position++] = c;
                        return true;
                    }
                }
                else if (m_write_position < m_dynamic_size)
                {
                    m_dynamic_buffer[m_write_position++] = c;
                    return true;
                }

                if (m_dynamic_buffer)
                {
                    char *np = (char *)realloc(m_dynamic_buffer, m_dynamic_size * 2);
                    if (!np)
                        return false;

                    m_dynamic_buffer = np;
                    m_dynamic_size *= 2;
                    m_dynamic_buffer[m_write_position++] = c;
                    return true;
                }
                const auto np{static_cast<char *>(malloc(std::size(m_builtin_buffer) * 2))};
                if (!np)
                    return false;

                memcpy(np, m_builtin_buffer, m_write_position);
                m_dynamic_buffer = np;
                m_dynamic_size = std::size(m_builtin_buffer) * 2;
                m_dynamic_buffer[m_write_position++] = c;
                return true;
            }

            /// Append a null-terminated C string.
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

            /// Append a string_view (does not require null-termination).
            bool append(std::string_view s)
            {
                if (s.empty())
                    return true;

                char *wp = ensure_free_space(s.size());
                if (!wp)
                    return false;

                memcpy(wp, s.data(), s.size());
                m_write_position += s.size();
                return true;
            }

            /// Append a character repeated N times.
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

            /// Append a string repeated N times.
            bool append_repeated(std::string_view text, size_t number_of_times)
            {
                if (text.empty() || !number_of_times)
                    return true;

                const size_t length_of_single_string = text.size();
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

            /// Append up to len characters from a C string.
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

            /// Append formatted text using std::format syntax.
            template <typename... Args> bool append_formatted(const std::string_view text, Args &&...args)
            {
                return append(std::vformat(text, std::make_format_args(args...)));
            }

            /// Clear all content and free dynamic memory.
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
            void correct(int bytes_written) const
            {
                if (m_dynamic_buffer == nullptr)
                {
                    if ((m_write_position + bytes_written) >= std::size(m_builtin_buffer))
                    {
                        assert(false);
                    }
                }
                else
                {
                    if ((m_write_position + bytes_written) >= m_dynamic_size)
                    {
                        assert(false);
                    }
                }

                if (bytes_written >= 0)
                {
                    m_write_position += bytes_written;
                }
                else
                {
                    assert(false);
                }
            }

            char *ensure_free_space(size_t space_needed)
            {
                const size_t space_total = m_write_position + space_needed;

                // if we're still using the stack buffer
                if (m_dynamic_buffer == nullptr)
                {
                    // and have enough space left
                    if (space_total < std::size(m_builtin_buffer))
                    {
                        return m_builtin_buffer + m_write_position;
                    }
                }

                // if we are using the dynamic buffer and have enough space left
                if (space_total < m_dynamic_size)
                {
                    return m_dynamic_buffer + m_write_position;
                }

                // we DO have a dynamic buffer, but not enough size: allocate large enough
                if (m_dynamic_buffer)
                {
                    size_t size_needed = m_dynamic_size * 2;
                    while (size_needed < space_total)
                    {
                        size_needed *= 2;
                    }

                    char *np = (char *)realloc(m_dynamic_buffer, size_needed);
                    if (!np)
                        return nullptr;

                    m_dynamic_buffer = np;
                    m_dynamic_size = size_needed;
                    return m_dynamic_buffer + m_write_position;
                }
                size_t size_needed = std::size(m_builtin_buffer) * 2;
                while (size_needed < space_total)
                {
                    size_needed *= 2;
                }
                const auto np{static_cast<char *>(malloc(size_needed))};
                if (!np)
                    return nullptr;

                memcpy(np, m_builtin_buffer, m_write_position);
                m_dynamic_buffer = np;
                m_dynamic_size = size_needed;
                return m_dynamic_buffer + m_write_position;
            }

            bool assign(const Writer &objectSrc)
            {
                if (this == &objectSrc)
                    return true;

                clear();

                // we need to copy EITHER the cache OR the dynamic buffer
                if (objectSrc.m_dynamic_buffer)
                {
                    m_dynamic_buffer = static_cast<char *>(malloc(objectSrc.m_dynamic_size));

                    if (!m_dynamic_buffer)
                        return false;

                    // but we need to copy only the used bytes
                    memcpy(m_dynamic_buffer, objectSrc.m_dynamic_buffer, objectSrc.m_write_position);

                    m_write_position = objectSrc.m_write_position;
                    m_dynamic_size = objectSrc.m_dynamic_size;
                    return true;
                }
                else
                {
                    clear();
                    m_write_position = objectSrc.m_write_position;
                    if (m_write_position)
                    {
                        memcpy(m_builtin_buffer, objectSrc.m_builtin_buffer, m_write_position);
                    }
                    return true;
                }
            }

        private:
            mutable size_t m_write_position;
            size_t m_dynamic_size;
            char *m_dynamic_buffer;
            char m_builtin_buffer[1024];
        };

        /// Repeat a string N times.
        /// @param text string to repeat
        /// @param n number of repetitions
        /// @return concatenated result
        inline std::string multiply(std::string_view text, uint32_t n)
        {
            Writer result;
            result.append_repeated(text, n);
            return result.as_string();
        }

        /// Repeat a character N times.
        /// @param c character to repeat
        /// @param n number of repetitions
        /// @return string of repeated characters
        inline std::string multiply(const char c, uint32_t n)
        {
            Writer result;
            result.append_repeated(c, n);
            return result.as_string();
        }
    } // namespace string
} // namespace pnq
