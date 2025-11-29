cmake_minimum_required(VERSION 3.25)
project(pnq
    VERSION 0.1.0
    DESCRIPTION "C++23 helper library"
    LANGUAGES CXX
)

# Library target
add_library(pnq INTERFACE)
add_library(pnq::pnq ALIAS pnq)

target_include_directories(pnq INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(pnq INTERFACE cxx_std_23)

target_compile_definitions(pnq INTERFACE
    UNICODE
    _UNICODE
)

# Dependencies - try find_package first, fall back to FetchContent for standalone builds
include(FetchContent)

find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
    message(STATUS "spdlog not found, fetching...")
    FetchContent_Declare(spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.0
    )
    FetchContent_MakeAvailable(spdlog)
endif()

find_package(tomlplusplus QUIET)
if(NOT tomlplusplus_FOUND)
    message(STATUS "tomlplusplus not found, fetching...")
    FetchContent_Declare(tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(tomlplusplus)
endif()

target_link_libraries(pnq INTERFACE
    spdlog::spdlog
    tomlplusplus::tomlplusplus
)

# Install rules - only when deps came from find_package (not FetchContent)
if(spdlog_FOUND AND tomlplusplus_FOUND)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(TARGETS pnq
        EXPORT pnqTargets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT pnqTargets
        FILE pnqTargets.cmake
        NAMESPACE pnq::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pnq
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/pnqConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/pnqConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pnq
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/pnqConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/pnqConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/pnqConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pnq
    )
endif()

# Tests
option(PNQ_BUILD_TESTS "Build tests" OFF)
if(PNQ_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
