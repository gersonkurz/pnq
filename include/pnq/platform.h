#pragma once

/// @file pnq/platform.h
/// @brief Platform detection and portable type definitions

#include <cstdint>
#include <string>
#include <string_view>

// ============================================================================
// Platform detection
// ============================================================================

#if defined(_WIN32) || defined(_WIN64)
    #define PNQ_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #define PNQ_PLATFORM_MACOS 1
#elif defined(__linux__)
    #define PNQ_PLATFORM_LINUX 1
#else
    #error "Unsupported platform"
#endif

// ============================================================================
// Architecture detection
// ============================================================================

#if defined(_M_X64) || defined(__x86_64__)
    #define PNQ_ARCH_X64 1
#elif defined(_M_ARM64) || defined(__aarch64__)
    #define PNQ_ARCH_ARM64 1
#elif defined(_M_IX86) || defined(__i386__)
    #define PNQ_ARCH_X86 1
#else
    #error "Unsupported architecture"
#endif

// ============================================================================
// Endianness check
// ============================================================================

// We require little-endian (Windows, Intel Macs, Apple Silicon are all LE)
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    #error "pnq requires a little-endian platform"
#endif

namespace pnq
{
    // ========================================================================
    // UTF-16 types
    // ========================================================================

    /// 16-bit character type for UTF-16 data.
    /// On Windows, this is wchar_t (for direct Win32 API interop).
    /// On other platforms, this is char16_t.
#ifdef PNQ_PLATFORM_WINDOWS
    using char16 = wchar_t;
#else
    using char16 = char16_t;
#endif

    /// UTF-16 string type using pnq::char16.
    using string16 = std::basic_string<char16>;

    /// UTF-16 string view type using pnq::char16.
    using string16_view = std::basic_string_view<char16>;

} // namespace pnq
