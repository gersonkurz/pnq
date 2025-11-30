# Cross-Platform Support Plan

## Goal
Enable pnq to build on macOS (Intel + ARM64) in addition to Windows (x64 + ARM64), with .REG file parsing/generation working everywhere and live registry access on Windows only.

## Key Decisions

1. **UTF-16 Character Type**: Use `pnq::char16` typedef - `wchar_t` on Windows (for Win32 API interop), `char16_t` on macOS. This gives us a consistent 16-bit type everywhere.

2. **UTF-16 String Types**: Define `pnq::string16` and `pnq::string16_view` as `std::basic_string<char16>` and `std::basic_string_view<char16>`.

3. **Unicode Conversion**: Platform-native APIs - Win32 `MultiByteToWideChar`/`WideCharToMultiByte` on Windows, CoreFoundation on macOS.

4. **Registry Type Constants**: On Windows, use the existing `REG_*` constants from `<Windows.h>`. On other platforms, define them ourselves in `regis3/types.h` (values match Windows exactly). Use `#ifndef REG_SZ` guards.

5. **Byte Order**: Require little-endian. Static assert to catch violations. (Windows, Intel Macs, and Apple Silicon are all LE.)

6. **Codepage Support**: `CP_ACP` and other codepage conversions are Windows-only. No macOS equivalent needed for .REG handling.

7. **String Utilities Portability**:
   - Case-insensitive compare: `_strnicmp` (Win) → `strncasecmp` (POSIX)
   - Uppercase/lowercase: `_strupr_s`/`_strlwr_s` (Win) → manual `std::toupper`/`std::tolower` loop (portable)

8. **Platform-Specific Code**: Use `#ifndef PNQ_PLATFORM_WINDOWS` guards for macOS-specific implementations, keeping Windows code as the default/primary path.

9. **Test Exclusions**: Windows-only tests guarded with `#ifndef PNQ_PLATFORM_WINDOWS` to skip on macOS.

## Milestones

Each milestone ends with: build x64 + ARM64, run ctests, commit.

---

### Milestone 0: Build Scripts
Create build scripts for from-scratch compilation on both platforms.

**Windows (`build.cmd`):**
- [ ] Clean build directories
- [ ] Configure CMake for x64, build, run ctests
- [ ] Configure CMake for ARM64, build, run ctests

**macOS (`justfile`):**
- [ ] Clean build directories
- [ ] Configure CMake for ARM64 (native), build, run ctests
- [ ] Configure CMake for x86_64 (cross-compile), build, run ctests

**Validation:** Run build scripts, all tests pass on all architectures.

---

### Milestone 1: Platform Foundation
Add platform detection and portable types without breaking anything.

- [ ] Create `pnq/platform.h` with:
  - `PNQ_PLATFORM_WINDOWS`, `PNQ_PLATFORM_MACOS` macros
  - `PNQ_ARCH_X64`, `PNQ_ARCH_ARM64` macros
  - `pnq::char16` typedef (wchar_t on Windows, char16_t elsewhere)
  - `pnq::string16`, `pnq::string16_view` type aliases
  - Static assert for little-endian

**Validation:** Build x64 + ARM64, run ctests, commit.

---

### Milestone 2: Unicode Abstraction
Create cross-platform UTF-8 <-> UTF-16 conversion.

- [ ] Create `pnq/unicode.h` with:
  - `unicode::to_utf8(string16_view)`
  - `unicode::to_utf16(string_view)`
  - Windows impl uses `MultiByteToWideChar`/`WideCharToMultiByte`
  - macOS impl (guarded with `#ifndef PNQ_PLATFORM_WINDOWS`) uses CoreFoundation
- [ ] Keep existing `string::encode_as_utf8/utf16` as wrappers (for compatibility)
- [ ] Add unit tests for unicode conversion

**Validation:** Build x64 + ARM64, run ctests, commit.

---

### Milestone 3: Registry Types Independence
Make `regis3/types.h` work without Windows.h on non-Windows platforms.

- [ ] Add `#ifndef REG_SZ` guards around constant definitions
- [ ] Define `REG_NONE`, `REG_SZ`, `REG_EXPAND_SZ`, `REG_BINARY`, `REG_DWORD`, `REG_QWORD`, `REG_MULTI_SZ`, etc.
- [ ] On Windows: constants come from `<Windows.h>` as usual
- [ ] On other platforms: our definitions provide the values

**Validation:** Build x64 + ARM64, run ctests, commit.

---

### Milestone 4: Value Class Portability
Make `regis3/value.h` cross-platform.

- [ ] Replace `std::wstring` with `pnq::string16`
- [ ] Replace `wchar_t` with `pnq::char16`
- [ ] Replace `string::encode_as_*` with `unicode::to_*`
- [ ] Remove unnecessary includes (`pnq.h`, `wstring.h`)

**Validation:** Build x64 + ARM64, run ctests, commit.

---

### Milestone 5: Guard Windows-Only Components
Properly isolate Windows-specific code.

- [ ] Add `#ifdef PNQ_PLATFORM_WINDOWS` guards to:
  - `regis3/key.h`
  - `regis3/iterators.h`
  - `regis3/importer.h`
- [ ] Update `regis3.h` main header to conditionally include
- [ ] Split `regis3/exporter.h`:
  - `regfile_exporter` family stays cross-platform
  - `registry_exporter` gets Windows guard

**Validation:** Build x64 + ARM64, run ctests, commit.

---

### Milestone 6: String Utilities Portability
Make `string.h` cross-platform.

- [ ] `equals_nocase`: Use `strncasecmp` on macOS (`#ifndef PNQ_PLATFORM_WINDOWS`)
- [ ] `uppercase`/`lowercase`: Use portable `std::toupper`/`std::tolower` loops
- [ ] Guard any remaining Windows-only functions

**Validation:** Build x64 + ARM64, run ctests, commit.

---

### Milestone 7: Text File Portability
Make text file I/O cross-platform (needed for .REG export).

- [ ] Update `text_file.h` to use portable types
- [ ] `write_utf8()` - straightforward
- [ ] `write_utf16()` - use `pnq::char16`
- [ ] `write_ansi()` - Windows-only (guard it)
- [ ] `read_auto()` - use `pnq::unicode`

**Validation:** Build x64 + ARM64, run ctests, commit.

---

### Milestone 8: Binary File Portability
Make `binary_file.h` cross-platform.

- [ ] Audit for Windows dependencies
- [ ] Add macOS implementations with `#ifndef PNQ_PLATFORM_WINDOWS` guards

**Validation:** Build x64 + ARM64, run ctests, commit.

---

### Milestone 9: Test Exclusions
Guard Windows-only tests.

- [ ] Identify tests that use live registry or Windows APIs
- [ ] Add `#ifndef PNQ_PLATFORM_WINDOWS` guards to skip on macOS
- [ ] Ensure remaining tests pass on both platforms

**Validation:** Build x64 + ARM64, run ctests, commit.

---

### Milestone 10: CMake Cross-Platform Build
Add macOS build support to CMake.

- [ ] Platform detection in CMakeLists.txt
- [ ] Conditional source inclusion
- [ ] Link CoreFoundation on macOS

**Validation:** Builds on both platforms, tests pass.

---

### Milestone 11: CI/CD
Add GitHub Actions for multi-platform builds.

- [ ] Windows x64 build + test
- [ ] Windows ARM64 build (cross-compile or native)
- [ ] macOS Intel build + test
- [ ] macOS ARM64 build + test

---

## Dependencies Between Milestones

```
0 (Build Scripts) - do first
    ↓
1 (Platform)
    ↓
2 (Unicode) ← depends on char16 from M1
    ↓
3 (Types) ← can parallel with M2, but do after for cleaner commits
    ↓
4 (Value) ← depends on M2 + M3
    ↓
5 (Guards) ← depends on M1 for PNQ_PLATFORM_WINDOWS
    ↓
6 (String Utils) ← depends on M1
    ↓
7 (Text File) ← depends on M2, M6
    ↓
8 (Binary File) ← independent of regis3 changes
    ↓
9 (Test Exclusions) ← depends on M1
    ↓
10 (CMake) ← depends on all above
    ↓
11 (CI/CD) ← depends on M10
```
