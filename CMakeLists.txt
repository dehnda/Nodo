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

# Conan dependencies
find_package(Eigen3 REQUIRED)
find_package(fmt REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(opengl_system REQUIRED)
find_package(gmp REQUIRED)
find_package(mpfr REQUIRED)
find_package(cgal REQUIRED)
if(NODEFLUX_BUILD_TESTS)
    find_package(GTest REQUIRED)
endif()

# All dependencies now managed by Conan - see conanfile.txt
# No more FetchContent needed for external libraries

# Core library
add_library(nodeflux_core
    # Core system files
    src/core/mesh.cpp
    src/core/error.cpp

    # Geometry generators
    src/geometry/boolean_ops.cpp
    src/geometry/mesh_generator.cpp
    src/geometry/sphere_generator.cpp
    src/geometry/cylinder_generator.cpp
    src/geometry/plane_generator.cpp
    src/geometry/box_generator.cpp
    src/geometry/torus_generator.cpp
    src/geometry/mesh_validator.cpp
    src/geometry/mesh_repairer.cpp

    # Spatial data structures
    src/spatial/bvh.cpp
    src/spatial/enhanced_boolean_ops.cpp

    # Performance and benchmarking
    src/benchmarks/performance_benchmark.cpp

    # GPU acceleration
    src/gpu/compute_device.cpp
    src/gpu/gl_context.cpp
    src/gpu/gpu_mesh_generator.cpp

    # I/O operations
    src/io/obj_exporter.cpp

    # Node system
    src/nodes/sphere_node.cpp
    src/nodes/cylinder_node.cpp
    src/nodes/plane_node.cpp
    src/nodes/box_node.cpp
    src/nodes/torus_node.cpp

    # SOP components
    src/sop/ArraySOP.cpp
    src/sop/BooleanSOP.cpp
    src/sop/MirrorSOP.cpp
    src/sop/NoiseDisplacementSOP.cpp
    src/sop/SubdivisionSOP.cpp
)

target_include_directories(nodeflux_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(nodeflux_core
    PUBLIC
        Eigen3::Eigen
        fmt::fmt
        glfw
        GLEW::GLEW
        opengl::opengl
        gmp::gmp
        mpfr::mpfr
        CGAL::CGAL
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
