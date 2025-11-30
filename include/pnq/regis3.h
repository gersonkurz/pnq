#pragma once

/// @file pnq/regis3.h
/// @brief Main include for pnq::regis3 - .REG file parsing, in-memory representation, and registry access
///
/// This header includes all regis3 functionality in the correct order.
/// Include this file to get the complete API.
///
/// Cross-platform components (available everywhere):
/// - types.h: Constants, options, type definitions
/// - value.h: Registry value representation
/// - key_entry.h: In-memory registry key tree
/// - parser.h: .REG file parser
/// - exporter.h: regfile_format4_exporter, regfile_format5_exporter
///
/// Windows-only components:
/// - iterators.h: Live registry enumeration
/// - key.h: Live registry key access (RAII wrapper for HKEY)
/// - importer.h: Live registry import
/// - exporter.h: registry_exporter (writes to live registry)

#include <pnq/regis3/types.h>
#include <pnq/regis3/value.h>
#include <pnq/regis3/key_entry.h>
#include <pnq/regis3/parser.h>

#ifdef PNQ_PLATFORM_WINDOWS
#include <pnq/regis3/iterators.h>
#include <pnq/regis3/key.h>
#include <pnq/regis3/importer.h>
#endif

#include <pnq/regis3/exporter.h>
