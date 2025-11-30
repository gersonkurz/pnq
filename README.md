# pnq

Small C++23 header-only library used in my projects that I find useful, and that is provided to the public in MIT license form in the hope it will be useful to you, too! 

So, given that literally hundreds of these projects already exist in github, what makes /pnq/ special?

- It allows you to handle .REG files with confidence. Actually, it's a port of [my older regis3 C# library](https://github.com/gersonkurz/regdiff) to C++, modernized, header-only. And it covers even [the rough edges of the registry specification](https://gist.github.com/SalviaSage/8eba542dc27eea3379a1f7dad3f729a0) perfectly fine.

- It features a pretty neat configuration system that I will document soon.

- It handles the quirks of Win32 console output much better and more complete than printf() and friends. If you ever tried to print a Euro symbol on the command line, you will appreciate the work.

- The name. it's of course a reference to [my website](https://p-nand-q.com) that *any moment now* I will revitalize. Promised!


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
