#pragma once

/// @file pnq/registry/types.h
/// @brief Foundation types for pnq::registry - enums, constants, forward declarations

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pnq
{
    namespace registry
    {
        // Forward declarations
        class value;
        class key_entry;
        class key;
        class regfile_parser;
        class regfile_importer;
        class regfile_exporter;
        class registry_importer;
        class registry_exporter;

        // =====================================================================
        // Constants
        // =====================================================================

        /// Sentinel value indicating unknown/uninitialized registry value type.
        inline constexpr uint32_t REG_TYPE_UNKNOWN = static_cast<uint32_t>(-1);

        /// Marker for escaped DWORD variables ($$VAR$$ syntax in .REG files).
        /// The actual value is stored as a string to be expanded at write time.
        inline constexpr uint32_t REG_ESCAPED_DWORD = static_cast<uint32_t>(-2);

        /// Marker for escaped QWORD variables ($$VAR$$ syntax in .REG files).
        inline constexpr uint32_t REG_ESCAPED_QWORD = static_cast<uint32_t>(-3);

        // =====================================================================
        // Import Options
        // =====================================================================

        /// Parser options for .REG file import.
        enum class import_options : uint32_t
        {
            /// No specific options.
            none = 0,

            /// Allow hashtag-style line comments (# comment).
            allow_hashtag_comments = 1,

            /// Allow semicolon-style line comments (; comment).
            allow_semicolon_comments = 2,

            /// Be more relaxed about whitespaces in .REG files.
            /// Recommended for manually edited files.
            ignore_whitespaces = 4,

            /// Allow variable names for non-string variables.
            /// Enables syntax like: "value"=dword:$$VARIABLE$$
            allow_variable_names_for_non_string_variables = 8,
        };

        /// Bitwise OR for import_options.
        inline constexpr import_options operator|(import_options a, import_options b)
        {
            return static_cast<import_options>(
                static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
        }

        /// Bitwise AND for import_options.
        inline constexpr import_options operator&(import_options a, import_options b)
        {
            return static_cast<import_options>(
                static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
        }

        /// Check if a flag is set in import_options.
        inline constexpr bool has_flag(import_options set, import_options test)
        {
            return (static_cast<uint32_t>(set) & static_cast<uint32_t>(test)) ==
                   static_cast<uint32_t>(test);
        }

        // =====================================================================
        // Export Options
        // =====================================================================

        /// Export options for .REG file generation.
        enum class export_options : uint32_t
        {
            /// No specific options.
            none = 0,

            /// Skip keys that have no values.
            no_empty_keys = 1,
        };

        /// Bitwise OR for export_options.
        inline constexpr export_options operator|(export_options a, export_options b)
        {
            return static_cast<export_options>(
                static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
        }

        /// Bitwise AND for export_options.
        inline constexpr export_options operator&(export_options a, export_options b)
        {
            return static_cast<export_options>(
                static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
        }

        /// Check if a flag is set in export_options.
        inline constexpr bool has_flag(export_options set, export_options test)
        {
            return (static_cast<uint32_t>(set) & static_cast<uint32_t>(test)) ==
                   static_cast<uint32_t>(test);
        }

        // =====================================================================
        // Helpers
        // =====================================================================

        /// Check if a registry value type is a string type (REG_SZ or REG_EXPAND_SZ).
        /// @param type Windows registry type constant
        /// @return true if the type represents a string value
        inline bool is_string_type(uint32_t type)
        {
            return (type == REG_SZ) || (type == REG_EXPAND_SZ);
        }

        /// Type alias for raw byte storage.
        using bytes = std::vector<std::uint8_t>;

    } // namespace registry
} // namespace pnq
