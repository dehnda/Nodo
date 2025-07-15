#include "nodeflux/core/mesh.hpp"
#include "nodeflux/geometry/boolean_ops.hpp"
#include "nodeflux/geometry/mesh_generator.hpp"
#include "nodeflux/io/obj_exporter.hpp"
#include <iostream>

using namespace nodeflux;

int main() {
    try {
        std::cout << "=== Boolean Operations Debug Test ===\n\n";
        
        // Test with non-overlapping boxes first
        std::cout << "TEST 1: Non-overlapping boxes\n";
        auto box1 = geometry::MeshGenerator::box(
            Eigen::Vector3d(0, 0, 0), 
            Eigen::Vector3d(1, 1, 1)
        );
        auto box2 = geometry::MeshGenerator::box(
            Eigen::Vector3d(2, 0, 0), 
            Eigen::Vector3d(3, 1, 1)
        );
        
        std::cout << "Box1: Volume = " << box1.volume() << ", vertices = " << box1.vertex_count() << "\n";
        std::cout << "Box2: Volume = " << box2.volume() << ", vertices = " << box2.vertex_count() << "\n";
        
        auto union_result = geometry::BooleanOps::union_meshes(box1, box2);
        if (union_result) {
            std::cout << "Union: Volume = " << union_result->volume() << ", vertices = " << union_result->vertex_count() << "\n";
            std::cout << "Expected volume = " << (box1.volume() + box2.volume()) << "\n\n";
        } else {
            std::cout << "Union failed: " << geometry::BooleanOps::last_error().description() << "\n\n";
        }
        
        // Test with overlapping boxes
        std::cout << "TEST 2: Overlapping boxes (original test)\n";
        auto box3 = geometry::MeshGenerator::box(
            Eigen::Vector3d(0, 0, 0), 
            Eigen::Vector3d(2, 2, 2)
        );
        auto box4 = geometry::MeshGenerator::box(
            Eigen::Vector3d(1, 1, 1), 
            Eigen::Vector3d(3, 3, 3)
        );
        
        std::cout << "Box3: Volume = " << box3.volume() << ", vertices = " << box3.vertex_count() << "\n";
        std::cout << "Box4: Volume = " << box4.volume() << ", vertices = " << box4.vertex_count() << "\n";
        
        auto union_result2 = geometry::BooleanOps::union_meshes(box3, box4);
        if (union_result2) {
            std::cout << "Union: Volume = " << union_result2->volume() << ", vertices = " << union_result2->vertex_count() << "\n";
            std::cout << "Expected volume = " << (box3.volume() + box4.volume() - 1.0) << " (8 + 8 - 1 overlap)\n";
            
            // Test intersection for comparison
            auto intersection = geometry::BooleanOps::intersect_meshes(box3, box4);
            if (intersection) {
                std::cout << "Intersection: Volume = " << intersection->volume() << ", vertices = " << intersection->vertex_count() << "\n";
                std::cout << "Expected intersection volume = 1.0\n";
            }
        } else {
            std::cout << "Union failed: " << geometry::BooleanOps::last_error().description() << "\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
