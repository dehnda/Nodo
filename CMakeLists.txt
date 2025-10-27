cmake_minimum_required(VERSION 3.20)
project(NodeFluxEngine VERSION 1.0.0 LANGUAGES CXX)

# Modern C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(NODEFLUX_BUILD_EXAMPLES "Build example applications" OFF)
option(NODEFLUX_BUILD_TESTS "Build unit tests" ON)
option(NODEFLUX_BUILD_STUDIO "Build Qt Studio application" ON)

# External dependencies
# xatlas - UV unwrapping library
add_library(xatlas STATIC
    external/xatlas/xatlas.cpp
)
target_include_directories(xatlas PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/xatlas>
    $<INSTALL_INTERFACE:include/xatlas>
)
# xatlas has some warnings, disable them
if(MSVC)
    target_compile_options(xatlas PRIVATE /W0)
else()
    target_compile_options(xatlas PRIVATE -w)
endif()
# Add to install set so nodeflux_core can reference it
install(TARGETS xatlas EXPORT NodeFluxEngineTargets)

# Add subdirectories
add_subdirectory(nodeflux_core)

# Tests
if(NODEFLUX_BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
    add_subdirectory(tests)
    # Verify target to run tests in the current configuration
    add_custom_target(verify-release
        COMMENT "Running unit tests before release build"
        DEPENDS nodeflux_tests
        COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIG> --output-on-failure --test-dir ${CMAKE_BINARY_DIR}
        VERBATIM
    )
else()
    # Stub target that explains how to enable tests
    add_custom_target(verify-release
        COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --red "Tests are disabled. Reconfigure with:"
        COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --red "  Conan: -o nodefluxengine:with_tests=True"
        COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --red "  CMake: -D NODEFLUX_BUILD_TESTS=ON (auto if using Conan option)"
        VERBATIM
    )
endif()

# Qt Studio Application
if(NODEFLUX_BUILD_STUDIO)
    add_subdirectory(nodeflux_studio)
endif()

# Installation
include(GNUInstallDirs)
install(TARGETS nodeflux_core
    EXPORT NodeFluxEngineTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY nodeflux_core/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets
install(EXPORT NodeFluxEngineTargets
    FILE NodeFluxEngineTargets.cmake
    NAMESPACE NodeFluxEngine::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NodeFluxEngine
)
