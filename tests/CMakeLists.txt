cmake_minimum_required(VERSION 3.20)

# Unit Tests using Google Test
include(GoogleTest)

add_executable(nodeflux_tests
    test_mesh.cpp
    test_boolean_ops.cpp
    test_mesh_generator.cpp
    test_mesh_validator.cpp
    test_mesh_repairer.cpp
    test_nodes.cpp
    test_bvh.cpp
)

target_link_libraries(nodeflux_tests
    PRIVATE
        nodeflux_core
        gtest_main
        gtest
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
