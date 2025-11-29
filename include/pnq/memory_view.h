#pragma once

#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

namespace pnq
{
    /// Byte vector type alias.
    using bytes = std::vector<std::uint8_t>;

    /// Non-owning view over a contiguous byte range.
    /// Similar to std::span<const std::uint8_t> but lighter weight.
    class memory_view final
    {
    public:
        /// Construct from a byte vector.
        memory_view(const bytes &vector)
            : m_data{vector.data()},
              m_size{vector.size()}
        {
        }

        /// Construct from a byte vector with explicit size limit.
        memory_view(const bytes &vector, size_t size)
            : m_data{vector.data()},
              m_size{size < vector.size() ? size : vector.size()}
        {
        }

        /// Construct from raw pointer and size.
        memory_view(const std::uint8_t *data, size_t size)
            : m_data{data},
              m_size{size}
        {
        }

        /// Construct from string_view (view the raw bytes).
        memory_view(std::string_view text)
            : m_data{reinterpret_cast<const std::uint8_t *>(text.data())},
              m_size{text.size()}
        {
        }

        /// Get pointer to data.
        constexpr auto data() const { return m_data; }

        /// Get size in bytes.
        constexpr auto size() const { return m_size; }

        /// Check if empty.
        constexpr bool empty() const { return m_size == 0; }

        /// Create an owning copy of the data.
        bytes duplicate() const
        {
            return bytes{m_data, m_data + m_size};
        }

        /// Byte-wise equality comparison.
        bool operator==(const memory_view &other) const
        {
            if (m_size != other.m_size)
                return false;
            if (m_data == other.m_data)
                return true;
            return std::memcmp(m_data, other.m_data, m_size) == 0;
        }

    private:
        const std::uint8_t *m_data;
        const size_t m_size;
    };
} // namespace pnq
