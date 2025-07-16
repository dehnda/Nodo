#include "nodeflux/geometry/mesh_generator.hpp"
#include "nodeflux/spatial/enhanced_boolean_ops.hpp"
#include "nodeflux/geometry/mesh_validator.hpp"
#include <iostream>

using namespace nodeflux;

int main() {
    std::cout << "BVH Enhanced Boolean Operations Demo\n";
    std::cout << "===================================\n\n";
    
    // Create two test meshes
    auto box = geometry::MeshGenerator::box(
        Eigen::Vector3d(-0.5, -0.5, -0.5), 
        Eigen::Vector3d(0.5, 0.5, 0.5)
    );
    
    auto sphere_opt = geometry::MeshGenerator::sphere(
        Eigen::Vector3d(0.0, 0.0, 0.0), 
        0.7, 
        3
    );
    
    if (!sphere_opt) {
        std::cout << "Failed to create sphere mesh\n";
        return 1;
    }
    auto sphere = *sphere_opt;
    
    std::cout << "Created test meshes:\n";
    std::cout << "Box: " << box.vertices().rows() << " vertices, " << box.faces().rows() << " faces\n";
    std::cout << "Sphere: " << sphere.vertices().rows() << " vertices, " << sphere.faces().rows() << " faces\n\n";
    
    // Validate original meshes
    auto box_report = geometry::MeshValidator::validate(box);
    auto sphere_report = geometry::MeshValidator::validate(sphere);
    
    std::cout << "Original mesh validation:\n";
    std::cout << "Box closed: " << box_report.is_closed << ", manifold: " << box_report.is_manifold << "\n";
    std::cout << "Sphere closed: " << sphere_report.is_closed << ", manifold: " << sphere_report.is_manifold << "\n\n";
    
    // Configure enhanced boolean parameters
    spatial::EnhancedBooleanOps::BooleanParams params;
    params.tolerance = 1e-9;
    params.build_bvh = true;
    params.validate_input = true;
    params.use_mesh_repair = true;
    params.ensure_manifold = true;
    
    std::cout << "Enhanced Boolean Operation Parameters:\n";
    std::cout << "- Tolerance: " << params.tolerance << "\n";
    std::cout << "- BVH Acceleration: " << (params.build_bvh ? "enabled" : "disabled") << "\n";
    std::cout << "- Input Validation: " << (params.validate_input ? "enabled" : "disabled") << "\n";
    std::cout << "- Mesh Repair: " << (params.use_mesh_repair ? "enabled" : "disabled") << "\n";
    std::cout << "- Ensure Manifold: " << (params.ensure_manifold ? "enabled" : "disabled") << "\n\n";
    
    // Test enhanced union operation
    std::cout << "Testing Enhanced Union Operation...\n";
    auto union_result = spatial::EnhancedBooleanOps::union_meshes(box, sphere, params);
    
    if (union_result) {
        auto union_report = geometry::MeshValidator::validate(*union_result);
        std::cout << "✓ Enhanced union succeeded!\n";
        std::cout << "Result: " << union_result->vertices().rows() << " vertices, " 
                  << union_result->faces().rows() << " faces\n";
        std::cout << "Closed: " << union_report.is_closed << ", manifold: " << union_report.is_manifold << "\n";
    } else {
        std::cout << "✗ Enhanced union failed\n";
        std::cout << "Error: " << spatial::EnhancedBooleanOps::last_error().message << "\n";
    }
    
    std::cout << "\nTesting Enhanced Intersection Operation...\n";
    auto intersection_result = spatial::EnhancedBooleanOps::intersect_meshes(box, sphere, params);
    
    if (intersection_result) {
        auto intersection_report = geometry::MeshValidator::validate(*intersection_result);
        std::cout << "✓ Enhanced intersection succeeded!\n";
        std::cout << "Result: " << intersection_result->vertices().rows() << " vertices, " 
                  << intersection_result->faces().rows() << " faces\n";
        std::cout << "Closed: " << intersection_report.is_closed << ", manifold: " << intersection_report.is_manifold << "\n";
    } else {
        std::cout << "✗ Enhanced intersection failed\n";
        std::cout << "Error: " << spatial::EnhancedBooleanOps::last_error().message << "\n";
    }
    
    std::cout << "\nTesting Enhanced Difference Operation...\n";
    auto difference_result = spatial::EnhancedBooleanOps::subtract_meshes(box, sphere, params);
    
    if (difference_result) {
        auto difference_report = geometry::MeshValidator::validate(*difference_result);
        std::cout << "✓ Enhanced difference succeeded!\n";
        std::cout << "Result: " << difference_result->vertices().rows() << " vertices, " 
                  << difference_result->faces().rows() << " faces\n";
        std::cout << "Closed: " << difference_report.is_closed << ", manifold: " << difference_report.is_manifold << "\n";
    } else {
        std::cout << "✗ Enhanced difference failed\n";
        std::cout << "Error: " << spatial::EnhancedBooleanOps::last_error().message << "\n";
    }
    
    return 0;
}
