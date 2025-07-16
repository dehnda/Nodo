#include "nodeflux/benchmarks/performance_benchmark.hpp"
#include "nodeflux/geometry/mesh_generator.hpp"
#include <iostream>

using namespace nodeflux;

int main() {
    std::cout << "Test mesh generation for different complexity levels\n";
    std::cout << "=================================================\n\n";
    
    // Test all complexity levels
    auto simple_mesh = benchmarks::PerformanceBenchmark::create_test_mesh(
        benchmarks::PerformanceBenchmark::ComplexityLevel::Simple);
    auto medium_mesh = benchmarks::PerformanceBenchmark::create_test_mesh(
        benchmarks::PerformanceBenchmark::ComplexityLevel::Medium);
    auto complex_mesh = benchmarks::PerformanceBenchmark::create_test_mesh(
        benchmarks::PerformanceBenchmark::ComplexityLevel::Complex);
    auto very_complex_mesh = benchmarks::PerformanceBenchmark::create_test_mesh(
        benchmarks::PerformanceBenchmark::ComplexityLevel::VeryComplex);
    
    std::cout << "Simple mesh: " << simple_mesh.vertices().rows() << " vertices, " 
              << simple_mesh.faces().rows() << " faces\n";
    std::cout << "Medium mesh: " << medium_mesh.vertices().rows() << " vertices, " 
              << medium_mesh.faces().rows() << " faces\n";
    std::cout << "Complex mesh: " << complex_mesh.vertices().rows() << " vertices, " 
              << complex_mesh.faces().rows() << " faces\n";
    std::cout << "Very complex mesh: " << very_complex_mesh.vertices().rows() << " vertices, " 
              << very_complex_mesh.faces().rows() << " faces\n\n";
    
    // Test specific triangle counts
    auto mesh_100 = benchmarks::PerformanceBenchmark::create_test_mesh(100);
    auto mesh_1000 = benchmarks::PerformanceBenchmark::create_test_mesh(1000);
    auto mesh_10000 = benchmarks::PerformanceBenchmark::create_test_mesh(10000);
    
    std::cout << "Target 100 triangles: " << mesh_100.vertices().rows() << " vertices, " 
              << mesh_100.faces().rows() << " faces\n";
    std::cout << "Target 1000 triangles: " << mesh_1000.vertices().rows() << " vertices, " 
              << mesh_1000.faces().rows() << " faces\n";
    std::cout << "Target 10000 triangles: " << mesh_10000.vertices().rows() << " vertices, " 
              << mesh_10000.faces().rows() << " faces\n";
    
    return 0;
}
