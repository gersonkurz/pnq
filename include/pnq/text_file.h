#pragma once

#include <string>
#include <Windows.h>
#include <pnq/binary_file.h>
#include <pnq/string.h>

namespace pnq
{
    namespace text_file
    {
        constexpr BYTE UTF16LE_BOM[] = {0xFF, 0xFE};
        constexpr BYTE UTF8_BOM[] = {0xEF, 0xBB, 0xBF};

        /// Read a text file, auto-detecting encoding via BOM.
        /// Converts UTF-16LE to UTF-8 if needed. Files without BOM are assumed UTF-8.
        /// @param filename path to the file
        /// @return file contents as UTF-8 string, or empty on failure
        inline std::string read_auto(std::string_view filename)
        {
            bytes data;
            if (!BinaryFile::read(filename, data))
                return {};

            if (data.size() >= 3 && memcmp(data.data(), UTF8_BOM, 3) == 0)
            {
                // UTF-8 with BOM - skip BOM
                return std::string(reinterpret_cast<const char *>(data.data() + 3), data.size() - 3);
            }
            if (data.size() >= 2 && memcmp(data.data(), UTF16LE_BOM, 2) == 0)
            {
                // UTF-16LE - convert to UTF-8
                std::wstring_view wide(reinterpret_cast<const wchar_t *>(data.data() + 2), (data.size() - 2) / sizeof(wchar_t));
                return string::encode_as_utf8(wide);
            }
            // No BOM - assume UTF-8
            return std::string(reinterpret_cast<const char *>(data.data()), data.size());
        }

        /// Create a UTF-8 encoded text file, optionally including a BOM.
        /// @param filename name of the text file to create
        /// @param text UTF-8 text to write
        /// @param include_bom whether to include UTF-8 BOM (default: true)
        /// @return true on success, false on failure
        inline bool write_utf8(std::string_view filename, std::string_view text, bool include_bom = true)
        {
            BinaryFile output;
            if (!output.create_for_writing(filename))
                return false;

            if (include_bom)
            {
                output.write({UTF8_BOM, std::size(UTF8_BOM)});
            }
            return output.write(text);
        }

        /// Create a UTF-16LE encoded text file, optionally including a BOM.
        /// @param filename name of the text file to create
        /// @param text UTF-16 text to write
        /// @param include_bom whether to include UTF-16LE BOM (default: true)
        /// @return true on success, false on failure
        inline bool write_utf16(std::string_view filename, std::wstring_view text, bool include_bom = true)
        {
            BinaryFile output;
            if (!output.create_for_writing(filename))
                return false;

            if (include_bom)
            {
                output.write({UTF16LE_BOM, std::size(UTF16LE_BOM)});
            }
            return output.write(memory_view{reinterpret_cast<const BYTE *>(text.data()), text.length() * sizeof(WCHAR)});
        }
    } // namespace text_file
} // namespace pnq
