# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Working style

- Direct, concise, technical - two professionals talking
- No sycophancy, no hand-holding explanations
- Disagree openly when warranted
- If something is wrong or could be better, say so
- Concise one-liner commits, not storyboards (imperative, capitalized: `Add string::contains`, `Fix edge cases: ...`)

## What this is

Windows-first, header-only C++23 utility library (MIT). Everything lives under `include/pnq/`. Consumers get it via
`add_subdirectory` or `FetchContent` and link `pnq::pnq` (an INTERFACE target). There are no .cpp files except the tests.

## Build and test

```
build.cmd                 # configure + build Debug for x64 AND ARM64 with tests (build-x64/, build-arm64/)
build.cmd clean           # rm both build dirs
test-x64.cmd              # ctest on build-x64 (--output-on-failure)
test-arm64.cmd            # only on ARM64 hardware
```

Direct equivalents:

```
cmake -B build-x64 -A x64 -DPNQ_BUILD_TESTS=ON [-DPNQ_USE_QUILL=ON]
cmake --build build-x64 --config Debug
ctest --test-dir build-x64 --build-config Debug --output-on-failure
```

Run a single test (Catch2 test-case discovery is enabled on native x64 builds, so ctest -R works too):

```
build-x64\tests\Debug\pnq_tests.exe "string::split"
build-x64\tests\Debug\pnq_tests.exe "[registry]"
ctest --test-dir build-x64 -C Debug -R "string::split"
```

Notes:
- First configure needs network: FetchContent pulls spdlog (or Quill with `PNQ_USE_QUILL=ON`), tomlplusplus, Catch2.
  `find_package` is tried first; the install/export rules only activate when deps came from `find_package`
  (and they reference `cmake/pnqConfig.cmake.in`, which is gitignored and absent - irrelevant for standalone builds).
- `/W4 /WX` on MSVC, `-Wall -Wextra -Wpedantic -Werror` elsewhere. Warnings are build failures.
- `UNICODE`/`_UNICODE` are defined by the target.
- ARM64 built on an x64 host is treated as cross-compiling: tests register as one ctest entry, no per-case discovery.
- Some `[registry]` and `[service]` tests touch the live registry/SCM; `registry::take_ownership_and_delete` is `[!mayfail]`
  because it needs elevation.
- Format with `clang-format -i <files>` (Allman braces, 4 spaces, 160 cols, `SortIncludes: Never` - include order is
  deliberate, don't reorder).

## Architecture

### Three umbrella headers, deliberately separate

- `pnq/pnq.h` - pulls in `<Windows.h>` and the whole general toolkit (string, path, file, text_file, config, console,
  win32 RAII wrappers, sqlite, ...). Also defines the project-wide macros: `PNQ_NEW`, `PNQ_FUNCTION_CONTEXT`,
  `PNQ_LOG_WIN_ERROR`, `PNQ_LOG_LAST_ERROR`, `PNQ_DECLARE_NON_COPYABLE`, `pnq::truncate_cast`, and `namespace fs = std::filesystem`.
- `pnq/regis3.h` - the .REG toolkit. Not included by `pnq.h`. Include order inside it matters (types → value → key_entry →
  parser → [Windows-only: iterators, key, importer] → exporter).
- `pnq/win32/service.h` and `pnq/hosts_file.h` - also not in `pnq.h`; include explicitly.

### regis3 is two layers

Cross-platform core (`types.h`, `value.h`, `key_entry.h`, `parser.h`, `regfile_*_exporter` in `exporter.h`) has no
Windows.h dependency - `types.h` redefines the `REG_*` constants under `#ifndef` guards for non-Windows. The live-registry
layer (`key.h`, `iterators.h`, `importer.h`, `registry_exporter`) is gated on `PNQ_PLATFORM_WINDOWS` from `platform.h`.
`docs/cross-platform-plan.md` is the milestone plan behind this split (macOS target); keep new regis3 code on the right side of it.

`key_entry` trees are intrusive ref-counted (`RefCountImpl` in `ref_counted.h`): allocate with `PNQ_NEW`, manage with
`PNQ_ADDREF`/`PNQ_RELEASE`. Importers return a root the caller must release.

### Logging is a compile-time backend switch

`log.h` maps `PNQ_LOG_{TRACE,DEBUG,INFO,WARN,ERROR,CRITICAL}` onto spdlog (default) or Quill (`PNQ_USE_QUILL`). The Quill code targets the
v11 API but the FetchContent fallback in `CMakeLists.txt` still pins `v7.5.0` - a standalone Quill build via FetchContent
will not compile until that tag is bumped. `logging.h` layers `initialize_logging`, `enable_console_logging`, `reconfigure_logging_for_file`, and
`report_windows_error` on top, with a full `#ifdef PNQ_USE_QUILL / #else` split. Any change to logging must be made in
both branches and built both ways. `file.h`, `path.h`, `binary_file.h` and `app_init.h` log directly, so `log.h` is a
transitive dependency of most of the library.

### Config system

`config/section.h` + `typed_value.h` + `typed_vector_value.h`: a struct hierarchy of `Section`s with `TypedValue<T>` members
registers itself with its parent at construction. `toml_backend.h` (tomlplusplus) is the only `config_backend` implementation.
`app_init.h`'s `AppInit` wires logging + config load/save + app paths together for executables.

### Opt-in SQLite

`sqlite/sqlite.h` is empty unless `__has_include(<sqlite3.h>)` is true. The consumer provides sqlite3; pnq never fetches it.

### Strings

Everything public is UTF-8 `std::string`. `win32/wstr_param.h` does the UTF-16 conversion at Win32 call sites
(implicit, SSO-style). `platform.h` provides `pnq::char16`/`string16` (wchar_t on Windows, char16_t elsewhere) for
code meant to be portable; `unicode.h` is the portable conversion layer, `string::encode_as_utf8/utf16` are the legacy wrappers.

## Known open defects

`pnq-todo.md` has one verified defect left (item 7, P2): `create_service` treats `start_type == 0` as "unset", so a
boot-start driver is silently downgraded to demand-start. Read it before touching `create_service`.

## Tests

Single file: `tests/test_main.cpp` (Catch2 v3, `TEST_CASE`/`SECTION`, tags like `[string]`, `[registry]`, `[service]`,
`[hosts]`). Behavioral changes get a case there. File-creating tests use the system temp dir and clean up.
