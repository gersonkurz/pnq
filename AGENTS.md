# Repository Guidelines

## Project Structure & Module Organization

`pnq` is a Windows-oriented, header-only C++23 library. Public code lives under `include/pnq/`; keep feature-specific APIs in matching subdirectories such as `config/`, `regis3/`, `sqlite/`, and `win32/`. The umbrella headers are `include/pnq/pnq.h` and `include/pnq/regis3.h`. Catch2 tests are currently collected in `tests/test_main.cpp`, with their target defined in `tests/CMakeLists.txt`. Design notes belong in `docs/`; `logo.png` and top-level Markdown files are repository assets and documentation. Treat `build-x64/` and `build-arm64/` as generated output.

## Build, Test, and Development Commands

- `build.cmd` configures and builds Debug targets for x64 and ARM64 with tests enabled.
- `build.cmd clean` removes both generated build directories.
- `cmake -B build-x64 -A x64 -DPNQ_BUILD_TESTS=ON` configures an x64 test build.
- `cmake --build build-x64 --config Debug` compiles that configuration.
- `test-x64.cmd` runs x64 tests with failure details; use `test-arm64.cmd` only on ARM64 hardware or suitable emulation.
- `ctest --test-dir build-x64 --build-config Debug --output-on-failure` is the direct test equivalent.

CMake may fetch spdlog (or Quill), tomlplusplus, and Catch2, so first-time configuration requires network access. Select Quill with `-DPNQ_USE_QUILL=ON`.

## Coding Style & Naming Conventions

Use four-space indentation, Allman braces, and a 160-column limit as specified by `.clang-format`; do not reorder includes automatically. Format changed C++ files with `clang-format -i <files>`. CMake enforces C++23 and treats warnings as errors (`/W4 /WX` on MSVC). Follow existing lowercase `snake_case` names for headers, functions, namespaces, and most types. Preserve established API casing where present, such as `Database` and `AppInit`.

## Testing Guidelines

Add focused Catch2 `TEST_CASE` and `SECTION` blocks for every behavioral change. Use descriptive case names and subsystem tags such as `[string]`, `[windows]`, or `[text_file]`. Tests that create files must use the system temporary directory and clean up afterward. There is no stated coverage threshold; regression coverage and passing x64 tests are the minimum expectation.

## Commit & Pull Request Guidelines

History uses concise, capitalized, imperative subjects, for example `Add string::contains` or `Fix edge cases`. Keep each commit narrowly scoped. Pull requests should explain the behavior and motivation, identify affected headers, link relevant issues, and report commands and architectures tested. Call out Windows-version, encoding, registry, or logging-backend implications; screenshots are needed only for documentation or visible-output changes.
