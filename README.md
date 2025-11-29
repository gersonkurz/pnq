# pnq

Small C++23 tools header library used in my projects.

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
