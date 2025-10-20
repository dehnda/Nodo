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

# Add subdirectories
add_subdirectory(nodeflux_core)

# Tests
if(NODEFLUX_BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
    add_subdirectory(tests)
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
