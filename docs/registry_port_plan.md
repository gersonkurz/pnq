# pnq::registry - Port Plan from C# regis3

## Overview

Port the C# regis3 library to C++23 header-only pnq style. The regis3 library provides:
- Parsing and exporting .REG files (REGEDIT4 and Windows Registry Editor 5.00 formats)
- Live Windows registry access (read/write/enumerate)
- In-memory representation of registry keys and values

**Source:** C:\Projects\2025\12\regdiff\regdiff\regis3\ (C# reference implementation, battle-tested 10+ years)
**Draft C++ port:** C:\Projects\2025\12\pnq\temp\registry\ (incomplete, needs refactoring)
**Target:** C:\Projects\2025\12\pnq\include\pnq\registry\

## Architecture

### Header Include Order (enforced by structure)

```
pnq/registry.h                    <- Main include, pulls everything in order
  pnq/registry/types.h            <- Enums, constants, forward declarations
  pnq/registry/value.h            <- Registry value (REG_SZ, REG_DWORD, etc.)
  pnq/registry/key_entry.h        <- Tree node for in-memory representation
  pnq/registry/iterators.h        <- key_iterator, value_iterator, enumerators
  pnq/registry/key.h              <- Live registry HKEY wrapper
  pnq/registry/parser.h           <- State machine parser for .REG files
  pnq/registry/importer.h         <- Import from file/string/registry
  pnq/registry/exporter.h         <- Export to file/string/registry
```

### Key Mappings from C# to C++/pnq

| C# regis3 | C++ pnq | Notes |
|-----------|---------|-------|
| `RegValueEntry` | `pnq::registry::value` | Registry value with type |
| `RegKeyEntry` | `pnq::registry::key_entry` | Tree node, uses RefCountImpl |
| `RegValueEntryKind` | `REG_*` Windows constants | Use native Windows defines |
| `IRegistryImporter` | `regfile_import_interface` | Pure virtual interface |
| `IRegistryExporter` | `regfile_export_interface` | Pure virtual interface |
| `RegFileParser` | `regfile_parser` | State machine, inherits abstract_parser |
| `RegFileImportOptions` | `REGFILE_IMPORT_OPTIONS` | Flags enum |
| `RegFileExportOptions` | `REGFILE_EXPORT_OPTIONS` | Flags enum |
| `Regis3` (static utils) | Functions in `pnq::registry` namespace | Hive mapping, etc. |
| `RegEnvReplace` | `pnq::string::Expander` | Already implemented in pnq |

### pnq Equivalents

| ngbtools | pnq | Header |
|----------|-----|--------|
| `dynamic_object_impl` | `pnq::RefCountImpl` | `<pnq/ref_counted.h>` |
| `NGBT_ADDREF/RELEASE` | `PNQ_ADDREF/PNQ_RELEASE` | `<pnq/ref_counted.h>` |
| `DBG_NEW` | `PNQ_NEW` | `<pnq/pnq.h>` |
| `string::writer` | `pnq::string::Writer` | `<pnq/string_writer.h>` |
| `string::from_textfile` | `pnq::text_file::read_auto` | `<pnq/text_file.h>` |
| `wstr_param` | `pnq::wstr_param` | `<pnq/win32/wstr_param.h>` |
| `logging::*` | `pnq::logging::*` | `<pnq/logging.h>` |
| `truncate_cast<>` | `pnq::truncate_cast<>` | `<pnq/pnq.h>` |

---

## Milestones

### Milestone 1: Foundation Types
**Files:** `types.h`, `value.h`

**types.h:**
- [ ] Forward declarations for all classes
- [ ] `REGFILE_IMPORT_OPTIONS` enum with flags
- [ ] `REGFILE_EXPORT_OPTIONS` enum with flags
- [ ] `REG_UNKNOWN`, `REG_ESCAPED_DWORD`, `REG_ESCAPED_QWORD` inline constants
- [ ] `is_string_type()` helper
- [ ] `has_flag()` template for enum flags

**value.h:**
- [ ] `value` class with name, type, data (bytes)
- [ ] Constructors: default, named, from registry key
- [ ] Type setters: `set_none()`, `set_dword()`, `set_qword()`, `set_string()`, `set_expanded_string()`, `set_multi_string()`, `set_binary_type()`
- [ ] Type getters: `get_dword()`, `get_qword()`, `get_string()`, `get_binary_type()`, `get_type()`
- [ ] `is_default_value()` - checks if name is empty
- [ ] `m_remove_flag` for diff/merge operations
- [ ] `as_string()` - export as .REG format (forward declared, impl in exporter.h)

**C# Reference:** `RegValueEntry.cs` lines 1-200

---

### Milestone 2: Key Entry (Tree Node)
**Files:** `key_entry.h`

- [ ] `key_entry` class inheriting from `pnq::RefCountImpl`
- [ ] Members: `m_name`, `m_parent` (key_entry*), `m_keys` (map), `m_values` (map), `m_default_value`, `m_remove_flag`
- [ ] Constructor with parent and name
- [ ] `find_or_create_key(path)` - navigate/create key hierarchy, handles `-KEY` removal syntax
- [ ] `find_or_create_value(name)` - get or create named value
- [ ] `get_path()` - reconstruct full path from parents
- [ ] `clone(parent)` - deep copy
- [ ] `ask_to_add_key()`, `ask_to_remove_key()` - for diff/merge
- [ ] `ask_to_add_value()`, `ask_to_remove_value()` - for diff/merge
- [ ] Destructor: release parent, clean up maps

**C# Reference:** `RegKeyEntry.cs` - note the Path property implementation, FindOrCreateKey logic

---

### Milestone 3: Live Registry Access
**Files:** `iterators.h`, `key.h`

**iterators.h:**
- [ ] `value_iterator` - lazy enumeration with `RegEnumValueW`
- [ ] `value_enumerator` - begin()/end() wrapper
- [ ] `key_iterator` - lazy enumeration with `RegEnumKeyExW`
- [ ] `key_enumerator` - begin()/end() wrapper

**key.h:**
- [ ] `key` class - RAII wrapper for HKEY
- [ ] `known_hives` map: HKEY_LOCAL_MACHINE, HKLM, etc.
- [ ] `open_for_reading()` - KEY_READ access
- [ ] `open_for_writing()` - KEY_WRITE|DELETE, creates if not exists
- [ ] `close()` - RegCloseKey
- [ ] `get(name, value&)` - read value from registry
- [ ] `set(name, value)` - write value to registry (handles remove_flag)
- [ ] `delete_subkey(name)` - RegDeleteTree
- [ ] `delete_recursive(path)` - static helper
- [ ] `enum_values()`, `enum_keys()` - return enumerators
- [ ] Convenience: `get_string()`, `get_dword()`, `set_string()`, `set_dword()`

**C# Reference:** `Regis3.cs` for hive mapping, `RegKeyEntry.cs` WriteToTheRegistry()

---

### Milestone 4: Parser Infrastructure
**Files:** `parser.h`

**abstract_parser** (generic, could move to pnq core later):
- [ ] Member function pointer typedef for state
- [ ] `parse_text(string)`, `parse_file(filename)`
- [ ] `set_current_state()` template
- [ ] `syntax_error()` with line/column reporting
- [ ] `m_buffer` (char_buffer or use string::Writer)
- [ ] Line/column tracking

**regfile_parser:**
- [ ] Inherits abstract_parser
- [ ] States: header, newline, start_of_line, key_path, value_name, equal_sign, value_definition, string_value, hex_value, multibyte_value, comments
- [ ] `m_result` - root key_entry
- [ ] `m_current_key`, `m_current_value` - parsing context
- [ ] `get_result()` - returns parsed tree (with addref)
- [ ] `cleanup()` - unwrap single root key
- [ ] `create_byte_array_from_buffer()` - **FIX BUG**: use hex_char_to_byte()

**C# Reference:** `RegFileParser.cs` - extensive state machine, study carefully

---

### Milestone 5: Import
**Files:** `importer.h`

- [ ] `regfile_import_interface` - pure virtual `import()` method
- [ ] `regfile_importer` base class - holds content, parser, result
- [ ] `regfile_format4_importer` - REGEDIT4 header
- [ ] `regfile_format5_importer` - Windows Registry Editor Version 5.00 header
- [ ] `registry_importer` - **NEW**: read live registry into key_entry tree
- [ ] `create_importer_from_file()` - factory, auto-detect format by header
- [ ] `create_importer_from_string()` - factory for string content

**registry_importer implementation (from C# RegistryImporter.cs):**
- [ ] Takes registry path as input
- [ ] Recursively reads keys and values using key::enum_keys/enum_values
- [ ] Builds key_entry tree

**C# Reference:** `RegFileImporter.cs`, `RegistryImporter.cs`

---

### Milestone 6: Export
**Files:** `exporter.h`

- [ ] `regfile_export_interface` - pure virtual `perform_export()` method
- [ ] `regfile_exporter` base class:
  - [ ] `export_to_writer()` - header + recursive content
  - [ ] `export_to_writer_recursive()` - key + values + subkeys
  - [ ] `export_to_writer(value*)` - single value formatting
  - [ ] `write_hex_encoded_value()` - hex(N): format with line wrapping
  - [ ] `reg_escape_string()` - escape backslash and quotes
  - [ ] `perform_export()` - **COMPLETE**: write to file with encoding
- [ ] `regfile_format4_exporter` - CP_ACP encoding
- [ ] `regfile_format5_exporter` - UTF-8/UTF-16 encoding
- [ ] `registry_exporter` - write key_entry tree to live registry
  - [ ] `export_recursive()` - handles remove_flag for deletions

**sorted_string_mapI helper:**
- [ ] Template for sorting unordered_map by key for deterministic output

**C# Reference:** `RegFileExporter.cs`, `RegFileFormat4Exporter.cs`, `RegFileFormat5Exporter.cs`

---

### Milestone 7: Main Header & Tests
**Files:** `registry.h`, tests

**registry.h:**
- [ ] Include all sub-headers in correct order
- [ ] Namespace using declarations if needed

**Tests:**
- [ ] Parse REGEDIT4 format file
- [ ] Parse Windows Registry Editor Version 5.00 format file
- [ ] Round-trip: parse -> export -> parse -> compare
- [ ] Read live registry key
- [ ] Write to registry (in safe test location like HKCU\Software\pnq_test)
- [ ] Variable expansion in values (string::Expander integration)
- [ ] Error handling: malformed .REG files
- [ ] Remove flag handling ([-KEY] and "value"=-)

---

## Known Issues to Fix

1. **Bug in create_byte_array_from_buffer()** (regfile_parser.cpp:474-485):
   ```cpp
   // WRONG: uses raw char value
   value = m[i];
   // SHOULD BE:
   value = hex_char_to_byte(m[i]);
   ```

2. **Incomplete perform_export()** (regfile_exporter.cpp:59-61):
   ```cpp
   // TODO: write to file with proper encoding
   if (!string::is_empty(m_filename))
   {
       // now, write to file in proper encoding
   }
   ```
   Need to use `pnq::text_file::write_utf8()` or write UTF-16 with BOM

3. **Missing RegistryImporter**: C# has `RegistryImporter.cs` that reads live registry into key_entry tree. The C++ draft only has the reverse (registry_exporter writes key_entry to registry).

4. **REG_QWORD export bug** (regfile_exporter.cpp:151):
   ```cpp
   // Says "dword" but should be "qword" or hex(b)
   output.append_formatted("{0}=dword:{1:016x}\r\n", name, value->get_qword());
   ```

---

## C# Reference Files Summary

| C# File | Purpose | Key Methods |
|---------|---------|-------------|
| `RegValueEntry.cs` | Value with type | SetStringValue, AsByteArray, WriteRegFileFormat |
| `RegKeyEntry.cs` | Tree node | FindOrCreateKey, Path property, WriteToTheRegistry |
| `RegFileParser.cs` | State machine | Import, all state handlers |
| `RegFileImporter.cs` | Base importer | Import() |
| `RegistryImporter.cs` | Live registry read | ImportRecursive |
| `RegFileExporter.cs` | Base exporter | Export, ExportRecursive |
| `Regis3.cs` | Static utils | OpenRegistryHive, hive name mappings |
| `RegEnvReplace.cs` | Variable subst | Replace (use pnq::string::Expander instead) |

---

## Progress Tracking

- [x] Milestone 1: Foundation Types (types.h, value.h) - DONE
- [x] Milestone 2: Key Entry (key_entry.h) - DONE
- [x] Milestone 3: Live Registry Access (iterators.h, key.h) - DONE
- [x] Milestone 4: Parser Infrastructure (parser.h) - DONE
- [x] Milestone 5: Import (importer.h) - DONE
- [x] Milestone 6: Export (exporter.h) - DONE
- [x] Milestone 7: Main Header & Tests - DONE (193 assertions)

---

## Notes

- All classes in `pnq::registry` namespace (not pnq::win32::registry)
- Use `PNQ_DECLARE_NON_COPYABLE` macro
- No std::unique_ptr - raw pointers with RefCountImpl
- Header-only with inline functions
- C++23 features welcome

---

## References

- **Excellent .REG file format reference**: https://gist.github.com/SalviaSage/8eba542dc27eea3379a1f7dad3f729a0
  - Covers encoding differences (REGEDIT4 = ANSI/CP_ACP, Version 5.00 = UTF-16LE with BOM)
  - Documents all value types and hex encoding quirks
  - Lists edge cases and format details
  - **TODO**: After port is complete, review implementation against this reference to ensure all quirks are handled
