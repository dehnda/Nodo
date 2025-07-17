/**
 * Simple test to isolate mesh generation issues
 */

#include "nodeflux/geometry/mesh_generator.hpp"
#include <iostream>

int main() {
    std::cout << "Testing basic mesh generation...\n";
    
    // Test box generation (should always work)
    try {
        auto box = nodeflux::geometry::MeshGenerator::box(
            Eigen::Vector3d(-1, -1, -1),
            Eigen::Vector3d(1, 1, 1)
        );
        std::cout << "Box mesh: " << box.vertex_count() << " vertices, " << box.face_count() << " faces\n";
    } catch (const std::exception& e) {
        std::cout << "Box generation failed: " << e.what() << "\n";
    }
    
    // Test sphere generation
    try {
        auto sphere_result = nodeflux::geometry::MeshGenerator::sphere(
            Eigen::Vector3d(0, 0, 0), 1.0, 2
        );
        
        if (sphere_result.has_value()) {
            std::cout << "Sphere mesh: " << sphere_result->vertex_count() << " vertices, " << sphere_result->face_count() << " faces\n";
        } else {
            std::cout << "Sphere generation returned nullopt\n";
            auto error = nodeflux::geometry::MeshGenerator::last_error();
            std::cout << "Error: " << error.message << "\n";
        }
    } catch (const std::exception& e) {
        std::cout << "Sphere generation failed: " << e.what() << "\n";
    }
    
    return 0;
}
