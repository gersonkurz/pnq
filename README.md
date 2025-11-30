# pnq

Small C++23 header-only library used in my projects that I find useful, and that is provided to the public in MIT license form in the hope it will be useful to you, too!

So, given that literally hundreds of these projects already exist in github, what makes *pnq* special?

## regis3 - .REG file toolkit

Not just a registry wrapper. Most "registry libraries" wrap `RegOpenKeyEx` so you don't forget `RegCloseKey`. That's not what this is.

`pnq::regis3` is a complete toolkit for .REG file parsing, in-memory representation, and diff/merge operations. It's a port of [my battle-tested C# regis3 library](https://github.com/gersonkurz/regdiff) to modern C++.

What you get:
- State machine parser that handles both REGEDIT4 (ANSI) and Windows Registry Editor 5.00 (UTF-16LE) formats
- In-memory tree representation (`key_entry`) with reference counting
- Diff/merge API: `ask_to_add_value()`, `ask_to_remove_value()` - designed for comparing registry snapshots
- Round-trip fidelity: parse → modify → export preserves formatting
- Handles [all the quirks](https://gist.github.com/SalviaSage/8eba542dc27eea3379a1f7dad3f729a0) - line continuation, hex encoding, escaped strings, the lot

The live registry access (`pnq::regis3::key`) is there too, but it's a supporting player.

## Text file encoding that just works

`text_file::read_auto` detects BOM (UTF-8, UTF-16LE) and converts everything to UTF-8. `write_utf16` and `write_ansi` go the other way. If you've ever debugged why your config file works on one machine but not another because of encoding, you'll appreciate this.

## string::Expander

Environment variable expansion with custom variable support:

```cpp
std::unordered_map<std::string, std::string> vars{{"VERSION", "1.0"}};
pnq::string::Expander e(vars);
e.expand_dollar(true);  // enable ${VAR} syntax too
auto result = e.expand("%APPDATA%\\MyApp\\${VERSION}\\config.toml");
```

Handles `%%` and `$$` escapes, unclosed delimiters, unknown variables (left as-is).

## path::normalize

Combines environment expansion with path canonicalization:

```cpp
auto path = pnq::path::normalize("%APPDIR%\\..\\other\\file.txt");
// Expands %APPDIR%, resolves .., normalizes slashes
```

Built-in variables: `%CD%`, `%APPDIR%`, `%SYSDIR%`. Custom variables override everything.

## wstr_param

The UTF-8 to UTF-16 dance for Win32 APIs gets old fast:

```cpp
// Before: MultiByteToWideChar everywhere
// After:
RegOpenKeyExW(hive, wstr_param{utf8_path}, ...);
```

Implicit conversion, stack-allocated for short strings, heap for long ones.

## RefCountImpl

COM-style reference counting without COM. Clean pattern for tree structures:

```cpp
class my_node : public pnq::RefCountImpl { ... };
auto* node = PNQ_NEW my_node();
node->retain();   // or PNQ_ADDREF(node)
node->release();  // or PNQ_RELEASE(node)
```

## TOML-based configuration

The config system lets you define your entire configuration *specification* in one header - schema, defaults, names, types, hierarchy. One source of truth:

```cpp
struct AppSettings : public pnq::config::Section
{
    AppSettings() : Section{} {}

    struct DatabaseSettings : public Section
    {
        DatabaseSettings(Section* parent) : Section{parent, "Database"} {}

        TypedValue<std::string> filename{this, "Filename", ""};
        TypedValue<int> port{this, "Port", 5432};
        TypedValue<bool> useSSL{this, "UseSSL", true};

    } database{this};

    struct UISettings : public Section
    {
        UISettings(Section* parent) : Section{parent, "UI"} {}

        TypedValue<std::string> theme{this, "Theme", "dark"};
        TypedValue<int> fontSize{this, "FontSize", 12};
        TypedValueVector<ColumnSection> columns{this, "Columns"};  // arrays too

    } ui{this};
};

extern AppSettings settings;  // single global instance
```

Then use it directly - no string keys to typo, no casting, no validation code:

```cpp
auto port = settings.database.port.value();      // int, with default
settings.ui.theme.setValue("light");             // type-checked
settings.save("config.toml");                    // serializes to TOML
```

The struct hierarchy maps directly to TOML sections. Enums work too with `enumAsString`/`enumFromString` hooks. Documentation coming - but the pattern above is the whole idea.

## Console output

Handles the quirks of Win32 console output better than `printf()` and friends. If you ever tried to print a Euro symbol (€) on the Windows command line and got garbage, you know the pain. This actually works.

## The name

It's a reference to [my website](https://p-nand-q.com) that *any moment now* I will revitalize. Promised!

## Dependencies

- [spdlog](https://github.com/gabime/spdlog)
- [tomlplusplus](https://github.com/marzer/tomlplusplus)

## Integration

### FetchContent (recommended)

```cmake
# Provide dependencies first
find_package(spdlog REQUIRED)
find_package(tomlplusplus REQUIRED)

FetchContent_Declare(pnq GIT_REPOSITORY <url> GIT_TAG main)
FetchContent_MakeAvailable(pnq)
target_link_libraries(your_target PRIVATE pnq::pnq)
```

### add_subdirectory

```cmake
add_subdirectory(path/to/pnq)
target_link_libraries(your_target PRIVATE pnq::pnq)
```

## Build

```bash
cmake -B build-x64 -A x64
cmake --build build-x64
```

Architecture options:
- `-A x64` - 64-bit x86
- `-A ARM64` - 64-bit ARM
- `-A Win32` - 32-bit x86

## Test

```bash
cmake -B build-x64 -A x64 -DPNQ_BUILD_TESTS=ON
cmake --build build-x64
ctest --test-dir build-x64
```
