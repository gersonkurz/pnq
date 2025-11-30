#pragma once

/// @file pnq/unicode.h
/// @brief Cross-platform Unicode conversion functions (UTF-8 <-> UTF-16)

#include <pnq/platform.h>

#ifdef PNQ_PLATFORM_WINDOWS
    #include <Windows.h>
#endif

#ifndef PNQ_PLATFORM_WINDOWS
    #include <CoreFoundation/CoreFoundation.h>
    #include <cstring>
#endif

namespace pnq
{
    namespace unicode
    {
        // ====================================================================
        // UTF-8 <-> UTF-16 Conversion
        // ====================================================================

#ifdef PNQ_PLATFORM_WINDOWS

        /// Convert UTF-16 to UTF-8.
        inline std::string to_utf8(string16_view input)
        {
            if (input.empty())
                return {};

            const int required_size = WideCharToMultiByte(
                CP_UTF8, 0,
                input.data(), static_cast<int>(input.size()),
                nullptr, 0, nullptr, nullptr);

            if (required_size <= 0)
                return {};

            std::string result(required_size, '\0');
            const int rc = WideCharToMultiByte(
                CP_UTF8, 0,
                input.data(), static_cast<int>(input.size()),
                result.data(), required_size,
                nullptr, nullptr);

            if (rc <= 0)
                return {};

            return result;
        }

        /// Convert UTF-8 to UTF-16.
        inline string16 to_utf16(std::string_view input)
        {
            if (input.empty())
                return {};

            const int required_size = MultiByteToWideChar(
                CP_UTF8, 0,
                input.data(), static_cast<int>(input.size()),
                nullptr, 0);

            if (required_size <= 0)
                return {};

            string16 result(required_size, L'\0');
            const int rc = MultiByteToWideChar(
                CP_UTF8, 0,
                input.data(), static_cast<int>(input.size()),
                result.data(), required_size);

            if (rc <= 0)
                return {};

            return result;
        }

        /// Convert from a specific codepage to UTF-16.
        inline string16 to_utf16(std::string_view input, unsigned int codepage)
        {
            if (input.empty())
                return {};

            const int required_size = MultiByteToWideChar(
                codepage, 0,
                input.data(), static_cast<int>(input.size()),
                nullptr, 0);

            if (required_size <= 0)
                return {};

            string16 result(required_size, L'\0');
            const int rc = MultiByteToWideChar(
                codepage, 0,
                input.data(), static_cast<int>(input.size()),
                result.data(), required_size);

            if (rc <= 0)
                return {};

            return result;
        }

        /// Convert UTF-16 to a specific codepage.
        inline std::string from_utf16(string16_view input, unsigned int codepage)
        {
            if (input.empty())
                return {};

            const int required_size = WideCharToMultiByte(
                codepage, 0,
                input.data(), static_cast<int>(input.size()),
                nullptr, 0, nullptr, nullptr);

            if (required_size <= 0)
                return {};

            std::string result(required_size, '\0');
            const int rc = WideCharToMultiByte(
                codepage, 0,
                input.data(), static_cast<int>(input.size()),
                result.data(), required_size,
                nullptr, nullptr);

            if (rc <= 0)
                return {};

            return result;
        }

#endif // PNQ_PLATFORM_WINDOWS

#ifndef PNQ_PLATFORM_WINDOWS

        /// Convert UTF-16 to UTF-8 (macOS implementation using CoreFoundation).
        inline std::string to_utf8(string16_view input)
        {
            if (input.empty())
                return {};

            CFStringRef cfstr = CFStringCreateWithBytes(
                kCFAllocatorDefault,
                reinterpret_cast<const UInt8*>(input.data()),
                input.size() * sizeof(char16_t),
                kCFStringEncodingUTF16LE,
                false);

            if (!cfstr)
                return {};

            CFIndex length = CFStringGetLength(cfstr);
            CFIndex max_size = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;

            std::string result(max_size, '\0');
            Boolean success = CFStringGetCString(cfstr, result.data(), max_size, kCFStringEncodingUTF8);
            CFRelease(cfstr);

            if (!success)
                return {};

            result.resize(std::strlen(result.c_str()));
            return result;
        }

        /// Convert UTF-8 to UTF-16 (macOS implementation using CoreFoundation).
        inline string16 to_utf16(std::string_view input)
        {
            if (input.empty())
                return {};

            CFStringRef cfstr = CFStringCreateWithBytes(
                kCFAllocatorDefault,
                reinterpret_cast<const UInt8*>(input.data()),
                input.size(),
                kCFStringEncodingUTF8,
                false);

            if (!cfstr)
                return {};

            CFIndex length = CFStringGetLength(cfstr);
            string16 result(length, u'\0');

            CFStringGetBytes(
                cfstr,
                CFRangeMake(0, length),
                kCFStringEncodingUTF16LE,
                '?',
                false,
                reinterpret_cast<UInt8*>(result.data()),
                length * sizeof(char16_t),
                nullptr);

            CFRelease(cfstr);
            return result;
        }

#endif // !PNQ_PLATFORM_WINDOWS

    } // namespace unicode
} // namespace pnq
