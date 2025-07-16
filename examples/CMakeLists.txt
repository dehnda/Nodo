# Examples

add_executable(basic_union basic_union.cpp)
target_link_libraries(basic_union nodeflux_core)

add_executable(debug_boolean debug_boolean.cpp)
target_link_libraries(debug_boolean nodeflux_core)

add_executable(primitive_generators primitive_generators.cpp)
target_link_libraries(primitive_generators nodeflux_core)

add_executable(primitive_boolean_test primitive_boolean_test.cpp)
target_link_libraries(primitive_boolean_test nodeflux_core)

add_executable(complete_node_system complete_node_system.cpp)
target_link_libraries(complete_node_system nodeflux_core)

add_executable(mesh_validation_demo mesh_validation_demo.cpp)
target_link_libraries(mesh_validation_demo nodeflux_core)

add_executable(torus_demo torus_demo.cpp)
target_link_libraries(torus_demo nodeflux_core)

add_executable(bvh_boolean_demo bvh_boolean_demo.cpp)
target_link_libraries(bvh_boolean_demo nodeflux_core)

add_executable(performance_benchmark performance_benchmark.cpp)
target_link_libraries(performance_benchmark nodeflux_core)

add_executable(test_mesh_generation test_mesh_generation.cpp)
target_link_libraries(test_mesh_generation nodeflux_core)

add_executable(gpu_compute_demo gpu_compute_demo.cpp)
target_link_libraries(gpu_compute_demo nodeflux_core)

add_executable(gpu_accelerated_demo gpu_accelerated_demo.cpp)
target_link_libraries(gpu_accelerated_demo nodeflux_core)

# GPU Mesh Generator comprehensive demo
add_executable(gpu_mesh_generator_demo gpu_mesh_generator_demo.cpp)
target_link_libraries(gpu_mesh_generator_demo nodeflux_core)

# GPU Performance scaling test
add_executable(gpu_performance_scaling_test gpu_performance_scaling_test.cpp)
target_link_libraries(gpu_performance_scaling_test nodeflux_core)

# GPU Complete suite test - all primitives
add_executable(gpu_complete_suite_test gpu_complete_suite_test.cpp)
target_link_libraries(gpu_complete_suite_test nodeflux_core)

# SOP Procedural System Demo
add_executable(sop_procedural_demo sop_procedural_demo.cpp)
target_link_libraries(sop_procedural_demo nodeflux_core)

# SOP Material System Demo
add_executable(sop_material_demo sop_material_demo.cpp)
target_link_libraries(sop_material_demo nodeflux_core)

# SOP Header-Based System Demo
add_executable(sop_header_demo sop_header_demo.cpp)
target_link_libraries(sop_header_demo nodeflux_core)

# Simple SOP Test - Quick test of new SOP implementations
add_executable(simple_sop_test simple_sop_test.cpp)
target_link_libraries(simple_sop_test nodeflux_core)

# Week 2 Working SOP Demo - Focused demo of working implementations
add_executable(week2_working_sop_demo week2_working_sop_demo.cpp)
target_link_libraries(week2_working_sop_demo nodeflux_core)

# Week 3 Advanced SOP Demo - ExtrudeSOP, LaplacianSOP, advanced workflows
add_executable(week3_advanced_sop_demo week3_advanced_sop_demo.cpp)
target_link_libraries(week3_advanced_sop_demo nodeflux_core)
