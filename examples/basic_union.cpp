#include "nodeflux/core/mesh.hpp"
#include "nodeflux/geometry/boolean_ops.hpp"
#include "nodeflux/geometry/mesh_generator.hpp"
#include <iostream>

using namespace nodeflux;

int main() {
    try {
        std::cout << "=== NodeFluxEngine Basic Union Example ===\n\n";
        
        // Generate two overlapping boxes
        std::cout << "Generating two overlapping boxes...\n";
        auto box1 = geometry::MeshGenerator::box(
            Eigen::Vector3d(0, 0, 0), 
            Eigen::Vector3d(2, 2, 2)
        );
        auto box2 = geometry::MeshGenerator::box(
            Eigen::Vector3d(1, 1, 1), 
            Eigen::Vector3d(3, 3, 3)
        );
        
        std::cout << "Box 1: " << box1.vertex_count() << " vertices, " 
                  << box1.face_count() << " faces\n";
        std::cout << "Box 2: " << box2.vertex_count() << " vertices, " 
                  << box2.face_count() << " faces\n\n";
        
        // Validate meshes
        std::cout << "Validating meshes...\n";
        bool val1 = geometry::BooleanOps::validate_mesh(box1);
        bool val2 = geometry::BooleanOps::validate_mesh(box2);
        
        if (!val1) {
            std::cerr << "Box1 validation failed: " << geometry::BooleanOps::last_error().description() << "\n";
            return 1;
        }
        if (!val2) {
            std::cerr << "Box2 validation failed: " << geometry::BooleanOps::last_error().description() << "\n";
            return 1;
        }
        
        std::cout << "Both meshes are valid for boolean operations.\n\n";
        
        // Perform union operation
        std::cout << "Performing union operation...\n";
        auto result = geometry::BooleanOps::union_meshes(box1, box2);
        
        if (result) {
            std::cout << "✅ Union successful!\n";
            std::cout << "Result: " << result->vertex_count() << " vertices, "
                      << result->face_count() << " faces\n";
            std::cout << "Volume: " << result->volume() << " cubic units\n";
            std::cout << "Surface area: " << result->surface_area() << " square units\n";
            
            // Validate result
            if (result->is_manifold()) {
                std::cout << "✅ Result mesh is manifold\n";
            }
            if (result->is_closed()) {
                std::cout << "✅ Result mesh is closed\n";
            }
            
        } else {
            std::cerr << "❌ Union failed: " << geometry::BooleanOps::last_error().description() << "\n";
            return 1;
        }
        
        std::cout << "\n=== Example completed successfully! ===\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Unexpected error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
