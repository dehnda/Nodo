cmake_minimum_required(VERSION 3.20)

# Unit Tests using Google Test
include(GoogleTest)

# Windows: Define M_PI before any includes
if(MSVC)
    add_compile_definitions(_USE_MATH_DEFINES)
endif()

add_executable(nodo_tests
    # Core Math & Utilities
    test_math.cpp

    # Mesh & Geometry Core
    test_mesh.cpp
    test_mesh_generator.cpp
    test_element_topology.cpp
    test_geometry_container.cpp

    # Processing (PMP Integration)
    test_pmp_converter.cpp

    # Attribute System
    test_attribute_types.cpp
    test_attribute_storage.cpp
    test_attribute_promotion.cpp
    test_attribute_group.cpp
    test_attribute_interpolation.cpp
    test_attribute_system_integration.cpp

    # Expression System
    test_expression_evaluator.cpp
    # test_expression_validator.cpp  # TODO: Move ExpressionValidator to nodo_core or create nodo_studio_lib

    # Graph & Execution
    test_nodes.cpp
    test_execution_engine_bridge.cpp
    test_graph_serialization.cpp
    test_selective_execution.cpp
    test_host_interface.cpp
    test_ch_references.cpp

    # SOP Nodes - Generators
    # (none yet - sphere, box, etc. are tested in test_nodes.cpp)

    # SOP Nodes - Modifiers
    test_array_sop.cpp
    test_boolean_sop.cpp
    # test_extrude_sop.cpp  # TODO: Fix protected execute() access
    test_polyextrude_edges.cpp
    test_noise_displacement.cpp
    test_wrangle_sop.cpp
    test_sop_group_filtering.cpp
    test_bevel_sop.cpp

    # Import/Export
    test_obj_export.cpp
)

target_link_libraries(nodo_tests
    PRIVATE
        nodo_core
        GTest::gtest_main
        GTest::gtest
)

target_compile_features(nodo_tests PRIVATE cxx_std_20)

# Compiler warnings
if(MSVC)
    target_compile_options(nodo_tests PRIVATE /W4)
else()
    target_compile_options(nodo_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Discover tests for CTest
gtest_discover_tests(nodo_tests)

# Demo program to show OBJ output
add_executable(show_obj_output show_obj_output.cpp)
target_link_libraries(show_obj_output PRIVATE nodo_core)
target_compile_features(show_obj_output PRIVATE cxx_std_20)

# Debug boolean operations
add_executable(debug_boolean debug_boolean.cpp)
target_link_libraries(debug_boolean PRIVATE nodo_core)
target_compile_features(debug_boolean PRIVATE cxx_std_20)
