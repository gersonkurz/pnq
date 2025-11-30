include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(pnq_tests
    test_main.cpp
)

target_link_libraries(pnq_tests PRIVATE
    pnq::pnq
    Catch2::Catch2WithMain
)

# catch_discover_tests runs the executable at build time, which fails for cross-compilation.
# Use simple add_test instead - we lose per-test granularity but it works everywhere.
# Note: VS with -A ARM64 on x64 host doesn't set CMAKE_CROSSCOMPILING, so check generator platform.
set(PNQ_CROSSCOMPILING FALSE)
if(CMAKE_CROSSCOMPILING)
    set(PNQ_CROSSCOMPILING TRUE)
elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64" AND CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64")
    set(PNQ_CROSSCOMPILING TRUE)
endif()

if(PNQ_CROSSCOMPILING)
    add_test(NAME pnq_tests COMMAND pnq_tests)
else()
    include(Catch)
    catch_discover_tests(pnq_tests)
endif()
