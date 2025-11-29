#pragma once

namespace pnq
{
    using bytes = std::vector<BYTE>;

    class memory_view final
    {
    public:
        memory_view(const bytes& vector)
            :
            m_data{ vector.empty() ? nullptr : &vector[0] },
            m_size{ vector.size() }
        {
        }

        memory_view(const bytes& vector, size_t size)
            :
            m_data{ vector.empty() ? nullptr : &vector[0] },
            m_size{ size < vector.size() ? size : vector.size() }
        {
        }

        memory_view(const BYTE* data, size_t size)
            :
            m_data{ data },
            m_size{ size }
        {
        }

        memory_view(std::string_view text)
            :
            m_data{ (const BYTE*) text.data() },
            m_size{ text.size() }
        {
        }

        constexpr auto data() const
        {
            return m_data;
        }

        constexpr auto size() const
        {
            return m_size;
        }

        bytes duplicate() const
        {
            // create a new bytes object that is a duplicate of the underlying memory
            return bytes{ m_data, m_data + m_size };
        }

        bool operator==(const memory_view& other) const
        {
            if (this == &other)
                return true;

            const auto a_size{ size() };
            const auto b_size{ other.size() };

            if (a_size != b_size)
                return false;

            return memcmp(data(), other.data(), a_size) == 0;
        }

    private:
        const BYTE* m_data;
        const size_t m_size;
    };
}
