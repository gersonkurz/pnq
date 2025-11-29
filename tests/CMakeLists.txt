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

include(Catch)
catch_discover_tests(pnq_tests)
