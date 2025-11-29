#pragma once

#include <format>
#include <pnq/string.h>
#include <pnq/win32/handle.h>
#include <pnq/win32/security_attributes.h>
#include <pnq/memory_view.h>
#include <pnq/logging.h>

namespace pnq
{
    class BinaryFile final
    {
    public:
        /// Default constructor
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

        bool create_or_open_for_write_append(std::string_view filename)
        {
            win32::SecurityAttributes sa;

            const auto handle{::CreateFileW(string::encode_as_utf16(filename).c_str(),
                GENERIC_WRITE,
                FILE_SHARE_READ,
                sa.default_access(),
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                nullptr)};
            if (!win32::Handle::is_valid(handle))
            {
                logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, std::format("CreateFile({}) failed", filename));
                return false;
            }
            m_file.set(handle);
            const auto result{::SetFilePointer(handle, 0, nullptr, FILE_END)};
            if (result == INVALID_SET_FILE_POINTER)
            {
                logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, "SetFilePointer() failed");
                m_file.close();
                return false;
            }
            return true;
        }
        bool create_for_writing(std::string_view filename)
        {
            win32::SecurityAttributes sa;

            const auto handle{::CreateFileW(string::encode_as_utf16(filename).c_str(),
                GENERIC_WRITE,
                FILE_SHARE_READ,
                sa.default_access(),
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                nullptr)};
            if (!win32::Handle::is_valid(handle))
            {
                logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, std::format("CreateFile({}) failed", filename));
                return false;
            }
            m_file.set(handle);
            return true;
        }
        bool open_for_reading(std::string_view filename)
        {
            win32::SecurityAttributes sa;

            const auto handle{::CreateFileW(string::encode_as_utf16(filename).c_str(),
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                sa.default_access(),
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr)};
            if (!win32::Handle::is_valid(handle))
            {
                logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, std::format("CreateFile('%s') failed", filename));
                return false;
            }
            m_file.set(handle);
            return true;
        }
        uint64_t get_file_size() const
        {
            LARGE_INTEGER file_size{0};
            if (!GetFileSizeEx(m_file, &file_size))
            {
                logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, "GetFileSizeEx() failed");
                return 0;
            }
            return file_size.QuadPart;
        }
        static bool read(std::string_view filename, bytes &result, size_t pad_bytes_at_end = 0)
        {
            result.clear();

            BinaryFile bf;
            if (!bf.open_for_reading(filename))
                return false;

            const size_t expected_file_size{static_cast<size_t>(bf.get_file_size())};
            result.resize(expected_file_size + pad_bytes_at_end);

            // TODO: better handling of Out-of-memory-conditions
            if (result.empty())
                return false;

            DWORD bytes_actually_read{0};
            if (!bf.raw_read(result.data(), static_cast<DWORD>(expected_file_size), bytes_actually_read))
                return false;

            if (bytes_actually_read < expected_file_size)
            {
                // we should know about this, but:
                spdlog::error("unexpected early read-end");
                return false;
            }

            if (pad_bytes_at_end)
            {
                ZeroMemory(&result[expected_file_size], pad_bytes_at_end);
            }

            return true;
        }
        bool read(bytes &result) const
        {
            const DWORD dwBytesAvailable = (DWORD)result.size();
            if (dwBytesAvailable == 0)
            {
                spdlog::error("binary_file::Read() called, but read buffer is empty");
                return false;
            }

            DWORD dwBytesActuallyRead = 0;
            if (!raw_read(&result[0], dwBytesAvailable, dwBytesActuallyRead))
                return false;

            if (dwBytesActuallyRead < dwBytesAvailable)
            {
                result.resize(dwBytesActuallyRead);
            }
            return true;
        }
        static bool write(std::string_view filename, const memory_view &result)
        {
            BinaryFile bf;

            if (!bf.create_for_writing(filename))
                return false;

            return bf.raw_write(result.data(), result.size());
        }
        bool write(const BYTE *memory, size_t size)
        {
            if (m_cache.empty())
            {
                return raw_write(memory, size);
            }
            return cached_write(memory, size);
        }
        bool write(const memory_view &data)
        {
            return write(data.data(), data.size());
        }
        bool write(LPCSTR text)
        {
            return write(reinterpret_cast<const BYTE *>(text), string::length(text));
        }

        uint64_t get_absolute_file_position() const
        {
            const LARGE_INTEGER liOfs{0};
            LARGE_INTEGER liNew{0};
            if (!SetFilePointerEx(m_file, liOfs, &liNew, FILE_CURRENT))
            {
                logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, "GetFilePosition() failed");
            }
            return liNew.QuadPart;
        }
        bool set_absolute_file_position(uint64_t position) const
        {
            LARGE_INTEGER liOfs;
            liOfs.QuadPart = position;
            LARGE_INTEGER liNew{0};
            if (!SetFilePointerEx(m_file, liOfs, &liNew, FILE_BEGIN))
            {
                logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, "SetFilePositionEx() failed");
                return false;
            }
            return true;
        }

        void close()
        {
            flush();
            m_file.close();
        }

        bool is_valid() const
        {
            return m_file.is_valid();
        }

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

        bool has_cache() const
        {
            return !m_cache.empty();
        }

        bool flush()
        {
            if (!has_cache() || !m_cache_write_pos)
                return true;

            const bool result{raw_write(m_cache.data(), m_cache_write_pos)};
            m_cache_write_pos = 0;
            return result;
        }

    private:
        bool raw_write(const BYTE *memory, size_t size) const
        {
            if (!m_file.is_valid())
                return false;

            DWORD bytes_written{0};
            if (!::WriteFile(m_file, memory, static_cast<DWORD>(size), &bytes_written, nullptr))
            {
                logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, "WriteFile() failed");
                return false;
            }
            return true;
        }
        bool cached_write(const BYTE *memory, size_t size)
        {
            if (!size || !memory)
                return true;

            const auto cacheSize{m_cache.size()};
            if (!cacheSize)
                return false;

            // if we can write it to the cache, do so now
            if (m_cache_write_pos + size < cacheSize)
            {
                memcpy(&m_cache[m_cache_write_pos], memory, size);
                m_cache_write_pos += size;
                return true;
            }

            // we need to flush at some point anyway
            flush();

            // determine how many blocks need to be written directly
            const auto remainingBytesForCache = size % cacheSize;
            if (const auto bytesToWriteImmediately = size - remainingBytesForCache)
            {
                if (!raw_write(memory, bytesToWriteImmediately))
                {
                    return false;
                }
                memory += bytesToWriteImmediately;
            }

            if (remainingBytesForCache)
            {
                memcpy(m_cache.data(), memory, remainingBytesForCache);
                m_cache_write_pos = remainingBytesForCache;
            }
            return true;
        }
        bool raw_read(LPBYTE lpbData, DWORD dwBytesToRead, DWORD &dwBytesActuallyRead) const
        {
            if (!::ReadFile(m_file, lpbData, dwBytesToRead, &dwBytesActuallyRead, nullptr))
            {
                logging::report_windows_error(GetLastError(), PNQ_FUNCTION_CONTEXT, "ReadFile() failed");
                return false;
            }
            return true;
        }

    private:
        win32::Handle m_file;
        bytes m_cache;
        size_t m_cache_write_pos;
    };
} // namespace pnq
