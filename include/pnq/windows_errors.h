#pragma once

#include <Windows.h>
#include <string>
#include <format>
#include <pnq/string.h>

namespace pnq
{
    namespace windows
    {
        /// Convert HRESULT to human-readable error string (US English).
        /// @param hResult the HRESULT to convert
        /// @return error message, or hex code if lookup fails
        inline std::string hresult_as_string(HRESULT hResult)
        {
            constexpr size_t buffer_size = 1024;
            wchar_t buffer[buffer_size];

            constexpr DWORD language_id = 0x409; // US English

            DWORD len = ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, hResult, language_id, buffer, buffer_size, nullptr);
            if (!len)
            {
                len = ::FormatMessageW(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                    GetModuleHandle(TEXT("NTDLL.DLL")),
                    hResult,
                    language_id,
                    buffer,
                    buffer_size,
                    nullptr);
            }

            if (!len)
                return std::format("{:#x} ({})", hResult, hResult);

            // Trim trailing \r\n
            while (len > 0 && (buffer[len - 1] == L'\r' || buffer[len - 1] == L'\n'))
                --len;

            return string::encode_as_utf8(std::wstring_view(buffer, len));
        }
    }
}
