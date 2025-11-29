#pragma once

#include <string>
#include <Windows.h>
#include <pnq/binary_file.h>

namespace pnq
{
    namespace text_file
    {
        constexpr BYTE UTF16LE_BOM[] = {0xFF, 0xFE};
        constexpr BYTE UTF8_BOM[] = {0xEF, 0xBB, 0xBF};

        /// Create a UTF-8 encoded text file, optionally including a bom. You can pass in
        ///	non-utf8 encoded data; in this case, the bom might be different.
        /// @param filename name of the text file generated
        /// @param text text to be written to the file
        /// @param include_bom flag indicating if you want to include a BOM (only UTF8 BOM supported)
        /// @param code_page code page of incoming text. The text data is NOT translated:
        ///		this flag has impact only W/R the BOM (to understand if a UTF8 BOM makes sense)
        /// @return success of operation
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

        /// Create a UTF-16LE encoded text file, optionally including a bom.
        /// @param filename name of the text file generated
        /// @param text text to be written to the file
        /// @param include_bom flag indicating if you want to include a BOM (only UTF16LE BOM supported)
        /// @return success of operation
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

        /// <summary>
        /// Read text file to string
        /// </summary>
        /// <param name="filename"></param>
        /// <param name="content"></param>
        /// <returns></returns>
        bool read(std::string_view filename, std::string &content);

        // todo: optimized line reader

    } // namespace text_file
} // namespace pnq
