cmake_minimum_required(VERSION 3.20)

# Nodo Core Library
project(nodo_core VERSION 0.1.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Eigen3 REQUIRED)
find_package(fmt REQUIRED)
find_package(manifold REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(exprtk REQUIRED)
find_package(pmp REQUIRED)

# Core library sources
set(NODO_CORE_SOURCES
    # Core
    src/core/attribute_descriptor.cpp
    src/core/attribute_group.cpp
    src/core/attribute_interpolation.cpp
    src/core/attribute_promotion.cpp
    src/core/attribute_set.cpp
    src/core/element_topology.cpp
    src/core/error.cpp
    src/core/geometry_attributes.cpp
    src/core/geometry_container.cpp
    src/core/math.cpp
    src/core/mesh.cpp

    # Host Interface (M2.1)
    src/IHostInterface.cpp

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

    # Processing
    src/processing/pmp_converter.cpp
    src/processing/decimation.cpp

    # SOPs
    src/sop/array_sop.cpp
    src/sop/boolean_sop.cpp
    src/sop/copy_to_points_sop.cpp
    src/sop/decimation_sop.cpp
    src/sop/extrude_sop.cpp
    src/sop/group_sop.cpp
    src/sop/laplacian_sop.cpp
    src/sop/line_sop.cpp
    src/sop/merge_sop.cpp
    src/sop/mirror_sop.cpp
    src/sop/noise_displacement_sop.cpp
    src/sop/normal_sop.cpp
    src/sop/bevel_sop.cpp
    src/sop/polyextrude_sop.cpp
    src/sop/resample_sop.cpp
    src/sop/scatter_sop.cpp
    src/sop/sop_factory.cpp
    src/sop/subdivision_sop.cpp
    src/sop/transform_sop.cpp
    src/sop/uv_unwrap_sop.cpp
    src/sop/wrangle_sop.cpp

    # Spatial
    src/spatial/bvh.cpp
    src/spatial/enhanced_boolean_ops.cpp

    # Expressions (M3.3 Phase 2)
    src/expressions/ExpressionEvaluator.cpp
)

# Core library headers (for IDE support)
set(NODO_CORE_HEADERS
    include/nodo/core/error.hpp
    include/nodo/core/geometry_attributes.hpp
    include/nodo/core/math.hpp
    include/nodo/core/mesh.hpp
    # Add more headers as needed
)

# Create the core library as a static library
add_library(nodo_core STATIC ${NODO_CORE_SOURCES} ${NODO_CORE_HEADERS})

# Set include directories
target_include_directories(nodo_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(nodo_core
    PUBLIC
        Eigen3::Eigen
        fmt::fmt
        nlohmann_json::nlohmann_json
        manifold::manifold
        exprtk::exprtk
        xatlas
        pmp::pmp
)

# Compiler warnings
if(MSVC)
    target_compile_options(nodo_core PRIVATE /W4 /bigobj)
else()
    target_compile_options(nodo_core PRIVATE
        -Wall
        -Wextra
        -pedantic
        -Wswitch-enum  # Warn if switch on enum misses cases
    )
endif()

# Export compile commands for clangd/IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Installation (optional - for packaging the library)
install(TARGETS nodo_core
    EXPORT nodo_coreTargets
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
        nodo_core
)

target_compile_features(benchmark_attributes PRIVATE cxx_std_20)

# Compiler warnings for benchmarks
if(MSVC)
    target_compile_options(benchmark_attributes PRIVATE /W4)
else()
    target_compile_options(benchmark_attributes PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Performance benchmark (M2.3)
add_executable(performance_benchmark
    src/benchmark/performance_benchmark.cpp
)

target_link_libraries(performance_benchmark
    PRIVATE
        nodo_core
)

target_compile_features(performance_benchmark PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(performance_benchmark PRIVATE /W4)
else()
    target_compile_options(performance_benchmark PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ============================================================================
# Tools
# ============================================================================

# Large graph generator for performance testing
# TODO: Update to new NodeGraph API (add_node, set_parameter with NodeParameter)
# add_executable(large_graph_generator
#     src/tools/large_graph_generator.cpp
# )
#
# target_link_libraries(large_graph_generator
#     PRIVATE
#         nodo_core
# )
#
# target_compile_features(large_graph_generator PRIVATE cxx_std_20)
#
# if(MSVC)
#     target_compile_options(large_graph_generator PRIVATE /W4)
# else()
#     target_compile_options(large_graph_generator PRIVATE -Wall -Wextra -Wpedantic)
# endif()
