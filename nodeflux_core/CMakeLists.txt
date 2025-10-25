cmake_minimum_required(VERSION 3.20)

# NodeFlux Core Library
project(nodeflux_core VERSION 0.1.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Eigen3 REQUIRED)
find_package(fmt REQUIRED)
find_package(manifold REQUIRED)
find_package(nlohmann_json REQUIRED)

# Core library sources
set(NODEFLUX_CORE_SOURCES
    # Core
    src/core/attribute_descriptor.cpp
    src/core/attribute_group.cpp
    src/core/attribute_interpolation.cpp
    src/core/attribute_promotion.cpp
    src/core/attribute_set.cpp
    src/core/element_topology.cpp
    src/core/error.cpp
    src/core/geometry_attributes.cpp
    src/core/math.cpp
    src/core/mesh.cpp

    # Geometry generators
    src/geometry/boolean_ops.cpp
    src/geometry/box_generator.cpp
    src/geometry/cylinder_generator.cpp
    src/geometry/mesh_generator.cpp
    src/geometry/mesh_generator_new.cpp
    src/geometry/mesh_repairer.cpp
    src/geometry/mesh_validator.cpp
    src/geometry/plane_generator.cpp
    src/geometry/sphere_generator.cpp
    src/geometry/torus_generator.cpp

    # Graph
    src/graph/execution_engine.cpp
    src/graph/graph_serializer.cpp
    src/graph/node_graph.cpp

    # IO
    src/io/obj_exporter.cpp
    src/io/obj_importer.cpp

    # SOPs
    src/sop/array_sop.cpp
    src/sop/boolean_sop.cpp
    src/sop/copy_to_points_sop.cpp
    src/sop/extrude_sop.cpp
    src/sop/laplacian_sop.cpp
    src/sop/line_sop.cpp
    src/sop/mirror_sop.cpp
    src/sop/noise_displacement_sop.cpp
    src/sop/polyextrude_sop.cpp
    src/sop/resample_sop.cpp
    src/sop/scatter_sop.cpp
    src/sop/sop_factory.cpp
    src/sop/subdivision_sop.cpp
    src/sop/transform_sop.cpp

    # Spatial
    src/spatial/bvh.cpp
    src/spatial/enhanced_boolean_ops.cpp
)

# Core library headers (for IDE support)
set(NODEFLUX_CORE_HEADERS
    include/nodeflux/core/error.hpp
    include/nodeflux/core/geometry_attributes.hpp
    include/nodeflux/core/math.hpp
    include/nodeflux/core/mesh.hpp
    # Add more headers as needed
)

# Create the core library as a static library
add_library(nodeflux_core STATIC ${NODEFLUX_CORE_SOURCES} ${NODEFLUX_CORE_HEADERS})

# Set include directories
target_include_directories(nodeflux_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(nodeflux_core
    PUBLIC
        Eigen3::Eigen
        fmt::fmt
        nlohmann_json::nlohmann_json
        manifold::manifold
)

# Compiler warnings
if(MSVC)
    target_compile_options(nodeflux_core PRIVATE /W4)
else()
    target_compile_options(nodeflux_core PRIVATE
        -Wall
        -Wextra
        -pedantic
        -Wswitch-enum  # Warn if switch on enum misses cases
    )
endif()

# Export compile commands for clangd/IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Installation (optional - for packaging the library)
install(TARGETS nodeflux_core
    EXPORT nodeflux_coreTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

# ============================================================================
# Benchmarks
# ============================================================================

# Attribute system benchmark
add_executable(benchmark_attributes
    src/benchmarks/benchmark_attributes.cpp
)

target_link_libraries(benchmark_attributes
    PRIVATE
        nodeflux_core
)

target_compile_features(benchmark_attributes PRIVATE cxx_std_20)

# Compiler warnings for benchmarks
if(MSVC)
    target_compile_options(benchmark_attributes PRIVATE /W4)
else()
    target_compile_options(benchmark_attributes PRIVATE -Wall -Wextra -Wpedantic)
endif()
