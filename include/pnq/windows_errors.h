#pragma once

#include "Windows.h"
#include <string>
#include <format>
#include <pnq/string.h>

namespace pnq
{
    namespace windows
    {
        inline std::string hresult_as_string(HRESULT hResult)
        {
            std::vector<wchar_t> buffer;
            buffer.resize(1024);

            // US English, see https://learn.microsoft.com/en-us/windows/win32/msi/localizing-the-error-and-actiontext-tables
            constexpr DWORD language_id{0x409};

            const auto buffer_size{static_cast<DWORD>(buffer.size())};
            if (!::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, hResult, language_id, buffer.data(), buffer_size, nullptr))
            {
                if (!::FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                        GetModuleHandle(TEXT("NTDLL.DLL")),
                        hResult,
                        language_id,
                        buffer.data(),
                        buffer_size,
                        nullptr))
                {
                    return std::format("{0:#x} ({0})", hResult);
                }
            }
            return string::encode_as_utf8(buffer.data());
        }
    } // namespace windows
} // namespace pnq
