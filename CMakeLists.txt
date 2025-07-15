cmake_minimum_required(VERSION 3.20)
project(NodeFluxEngine VERSION 1.0.0 LANGUAGES CXX)

# Modern C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(NODEFLUX_BUILD_EXAMPLES "Build example applications" ON)
option(NODEFLUX_BUILD_TESTS "Build unit tests" ON)
option(NODEFLUX_BUILD_UI "Build UI components" OFF)

# Find packages via vcpkg
find_package(Eigen3 REQUIRED)
find_package(fmt REQUIRED)

# Use FetchContent for CGAL (since vcpkg has issues on Linux)
include(FetchContent)
FetchContent_Declare(
    CGAL
    GIT_REPOSITORY https://github.com/CGAL/cgal.git
    GIT_TAG v5.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(CGAL)

# Core library
add_library(nodeflux_core
    src/core/mesh.cpp
    src/core/error.cpp
    src/geometry/boolean_ops.cpp
    src/geometry/mesh_generator.cpp
)

target_include_directories(nodeflux_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${cgal_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(nodeflux_core
    PUBLIC
        Eigen3::Eigen
        fmt::fmt
        gmp
        mpfr
)

# Compiler features and warnings
target_compile_features(nodeflux_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(nodeflux_core PRIVATE /W4)
else()
    target_compile_options(nodeflux_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Examples
if(NODEFLUX_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(NODEFLUX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# UI components (optional)
if(NODEFLUX_BUILD_UI)
    add_subdirectory(ui)
endif()

# Installation
include(GNUInstallDirs)
install(TARGETS nodeflux_core
    EXPORT NodeFluxEngineTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets
install(EXPORT NodeFluxEngineTargets
    FILE NodeFluxEngineTargets.cmake
    NAMESPACE NodeFluxEngine::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NodeFluxEngine
)
