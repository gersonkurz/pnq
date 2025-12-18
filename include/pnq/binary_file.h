#pragma once

#include <cstring>
#include <format>

#include <pnq/string.h>
#include <pnq/win32/handle.h>
#include <pnq/win32/security_attributes.h>
#include <pnq/memory_view.h>
#include <pnq/logging.h>

namespace pnq
{
    /// Binary file I/O with optional write caching.
    class BinaryFile final
    {
    public:
        BinaryFile()
            : m_cache_write_pos{0}
        {
        }

        ~BinaryFile()
        {
            close();
        }

        BinaryFile(const BinaryFile &) = delete;
        BinaryFile &operator=(const BinaryFile &) = delete;
        BinaryFile(BinaryFile &&) = delete;
        BinaryFile &operator=(BinaryFile &&) = delete;

        /// Open or create file for appending.
        /// @param filename path to file
        /// @return true if successful
        bool create_or_open_for_write_append(std::string_view filename)
        {
            win32::SecurityAttributes sa;

            const auto handle = ::CreateFileW(string::encode_as_utf16(filename).c_str(),
                GENERIC_WRITE,
                FILE_SHARE_READ,
                sa.default_access(),
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                nullptr);
            if (!win32::Handle::is_valid(handle))
            {
                PNQ_LOG_LAST_ERROR( "CreateFile('{}') failed", filename);
                return false;
            }
            m_file.set(handle);
            LARGE_INTEGER offset{0};
            if (!::SetFilePointerEx(handle, offset, nullptr, FILE_END))
            {
                PNQ_LOG_LAST_ERROR( "SetFilePointerEx failed");
                m_file.close();
                return false;
            }
            return true;
        }

        /// Create file for writing (truncates existing).
        /// @param filename path to file
        /// @return true if successful
        bool create_for_writing(std::string_view filename)
        {
            win32::SecurityAttributes sa;

            const auto handle = ::CreateFileW(string::encode_as_utf16(filename).c_str(),
                GENERIC_WRITE,
                FILE_SHARE_READ,
                sa.default_access(),
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                nullptr);
            if (!win32::Handle::is_valid(handle))
            {
                PNQ_LOG_LAST_ERROR( "CreateFile('{}') failed", filename);
                return false;
            }
            m_file.set(handle);
            return true;
        }

        /// Open existing file for reading.
        /// @param filename path to file
        /// @return true if successful
        bool open_for_reading(std::string_view filename)
        {
            win32::SecurityAttributes sa;

            const auto handle = ::CreateFileW(string::encode_as_utf16(filename).c_str(),
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                sa.default_access(),
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr);
            if (!win32::Handle::is_valid(handle))
            {
                PNQ_LOG_LAST_ERROR( "CreateFile('{}') failed", filename);
                return false;
            }
            m_file.set(handle);
            return true;
        }

        /// Get file size in bytes.
        uint64_t get_file_size() const
        {
            LARGE_INTEGER file_size{0};
            if (!GetFileSizeEx(m_file, &file_size))
            {
                PNQ_LOG_LAST_ERROR( "GetFileSizeEx failed");
                return 0;
            }
            return file_size.QuadPart;
        }

        /// Read entire file into buffer.
        /// @param filename path to file
        /// @param result receives the file contents
        /// @param pad_bytes_at_end optional zero-padding at end
        /// @return true if successful
        static bool read(std::string_view filename, bytes &result, size_t pad_bytes_at_end = 0)
        {
            result.clear();

            BinaryFile bf;
            if (!bf.open_for_reading(filename))
                return false;

            const size_t expected_file_size = static_cast<size_t>(bf.get_file_size());

            // Empty file with no padding is valid
            if (expected_file_size == 0 && pad_bytes_at_end == 0)
                return true;

            result.resize(expected_file_size + pad_bytes_at_end);

            if (expected_file_size > 0)
            {
                DWORD bytes_actually_read = 0;
                if (!bf.raw_read(result.data(), static_cast<DWORD>(expected_file_size), bytes_actually_read))
                    return false;

                if (bytes_actually_read < expected_file_size)
                {
                    spdlog::error("unexpected early read-end");
                    return false;
                }
            }

            if (pad_bytes_at_end)
            {
                std::memset(&result[expected_file_size], 0, pad_bytes_at_end);
            }

            return true;
        }

        /// Read into pre-sized buffer.
        /// @param result buffer to read into (size determines bytes to read)
        /// @return true if successful
        bool read(bytes &result) const
        {
            const DWORD bytes_available = static_cast<DWORD>(result.size());
            if (bytes_available == 0)
            {
                spdlog::error("BinaryFile::read() called with empty buffer");
                return false;
            }

            DWORD bytes_actually_read = 0;
            if (!raw_read(result.data(), bytes_available, bytes_actually_read))
                return false;

            if (bytes_actually_read < bytes_available)
            {
                result.resize(bytes_actually_read);
            }
            return true;
        }

        /// Write entire buffer to new file.
        /// @param filename path to file
        /// @param data data to write
        /// @return true if successful
        static bool write(std::string_view filename, const memory_view &data)
        {
            BinaryFile bf;

            if (!bf.create_for_writing(filename))
                return false;

            return bf.raw_write(data.data(), data.size());
        }

        /// Write bytes (uses cache if enabled).
        /// @param memory pointer to data
        /// @param size number of bytes
        /// @return true if successful
        bool write(const std::uint8_t *memory, size_t size)
        {
            if (m_cache.empty())
            {
                return raw_write(memory, size);
            }
            return cached_write(memory, size);
        }

        /// Write memory_view.
        bool write(const memory_view &data)
        {
            return write(data.data(), data.size());
        }

        /// Write C string.
        bool write(const char *text)
        {
            return write(reinterpret_cast<const std::uint8_t *>(text), string::length(text));
        }

        /// Get current file position.
        uint64_t get_absolute_file_position() const
        {
            const LARGE_INTEGER offset{0};
            LARGE_INTEGER new_pos{0};
            if (!SetFilePointerEx(m_file, offset, &new_pos, FILE_CURRENT))
            {
                PNQ_LOG_LAST_ERROR( "GetFilePosition failed");
            }
            return new_pos.QuadPart;
        }

        /// Set absolute file position.
        /// @param position byte offset from start
        /// @return true if successful
        bool set_absolute_file_position(uint64_t position) const
        {
            LARGE_INTEGER offset;
            offset.QuadPart = position;
            LARGE_INTEGER new_pos{0};
            if (!SetFilePointerEx(m_file, offset, &new_pos, FILE_BEGIN))
            {
                PNQ_LOG_LAST_ERROR( "SetFilePositionEx failed");
                return false;
            }
            return true;
        }

        /// Flush cache and close file.
        void close()
        {
            flush();
            m_file.close();
        }

        /// Check if file handle is valid.
        bool is_valid() const
        {
            return m_file.is_valid();
        }

        /// Set write cache size (0 disables caching).
        void set_cache_size(size_t size)
        {
            if (size == 0)
            {
                m_cache.clear();
            }
            else
            {
                m_cache.resize(size);
            }
        }

        /// Check if write caching is enabled.
        bool has_cache() const
        {
            return !m_cache.empty();
        }

        /// Flush write cache to disk.
        bool flush()
        {
            if (!has_cache() || !m_cache_write_pos)
                return true;

            const bool result = raw_write(m_cache.data(), m_cache_write_pos);
            m_cache_write_pos = 0;
            return result;
        }

    private:
        bool raw_write(const std::uint8_t *memory, size_t size) const
        {
            if (!m_file.is_valid())
                return false;

            DWORD bytes_written = 0;
            if (!::WriteFile(m_file, memory, static_cast<DWORD>(size), &bytes_written, nullptr))
            {
                PNQ_LOG_LAST_ERROR( "WriteFile failed");
                return false;
            }
            return true;
        }

        bool cached_write(const std::uint8_t *memory, size_t size)
        {
            if (!size || !memory)
                return true;

            const auto cache_size = m_cache.size();
            if (!cache_size)
                return false;

            // If we can write it to the cache, do so now
            if (m_cache_write_pos + size < cache_size)
            {
                std::memcpy(&m_cache[m_cache_write_pos], memory, size);
                m_cache_write_pos += size;
                return true;
            }

            // We need to flush at some point anyway
            flush();

            // Determine how many blocks need to be written directly
            const auto remaining_bytes_for_cache = size % cache_size;
            if (const auto bytes_to_write_immediately = size - remaining_bytes_for_cache)
            {
                if (!raw_write(memory, bytes_to_write_immediately))
                {
                    return false;
                }
                memory += bytes_to_write_immediately;
            }

            if (remaining_bytes_for_cache)
            {
                std::memcpy(m_cache.data(), memory, remaining_bytes_for_cache);
                m_cache_write_pos = remaining_bytes_for_cache;
            }
            return true;
        }

        bool raw_read(std::uint8_t *data, DWORD bytes_to_read, DWORD &bytes_actually_read) const
        {
            if (!::ReadFile(m_file, data, bytes_to_read, &bytes_actually_read, nullptr))
            {
                PNQ_LOG_LAST_ERROR( "ReadFile failed");
                return false;
            }
            return true;
        }

        win32::Handle m_file;
        bytes m_cache;
        size_t m_cache_write_pos;
    };
} // namespace pnq
