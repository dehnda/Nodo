cmake_minimum_required(VERSION 3.20)

# Unit Tests using Google Test
include(GoogleTest)

add_executable(nodeflux_tests
    test_mesh.cpp
    test_boolean_ops.cpp
    test_math.cpp
    test_mesh_generator.cpp
    test_mesh_validator.cpp
    test_mesh_repairer.cpp
    test_nodes.cpp
    test_bvh.cpp
    test_obj_export.cpp
)

target_link_libraries(nodeflux_tests
    PRIVATE
        nodeflux_core
        GTest::gtest_main
        GTest::gtest
)

target_compile_features(nodeflux_tests PRIVATE cxx_std_20)

# Compiler warnings
if(MSVC)
    target_compile_options(nodeflux_tests PRIVATE /W4)
else()
    target_compile_options(nodeflux_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Discover tests for CTest
gtest_discover_tests(nodeflux_tests)

# Demo program to show OBJ output
add_executable(show_obj_output show_obj_output.cpp)
target_link_libraries(show_obj_output PRIVATE nodeflux_core)
target_compile_features(show_obj_output PRIVATE cxx_std_20)
