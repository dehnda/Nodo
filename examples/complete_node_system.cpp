#include <iostream>
#include <nodeflux/nodes/sphere_node.hpp>
#include <nodeflux/nodes/cylinder_node.hpp>
#include <nodeflux/nodes/plane_node.hpp>
#include <nodeflux/nodes/box_node.hpp>
#include <nodeflux/geometry/boolean_ops.hpp>
#include <nodeflux/io/obj_exporter.hpp>

using namespace nodeflux;

int main() {
    std::cout << "NodeFluxEngine - Complete Node System Demo\n";
    std::cout << "==========================================\n\n";

    // Test all primitive nodes
    {
        std::cout << "=== Primitive Node Tests ===\n";
        
        // Box Node
        std::cout << "Testing BoxNode...\n";
        nodes::BoxNode box_node(2.0, 1.5, 1.0, 2, 2, 2);
        auto box_mesh = box_node.generate();
        if (box_mesh) {
            std::cout << "  Box: " << box_mesh->vertices().rows() << " vertices, " 
                     << box_mesh->faces().rows() << " faces\n";
            io::ObjExporter::export_mesh(*box_mesh, "examples/output/node_box.obj");
        }

        // Sphere Node (UV)
        std::cout << "Testing SphereNode (UV)...\n";
        nodes::SphereNode sphere_node(1.0, 16, 8);
        auto sphere_mesh = sphere_node.generate();
        if (sphere_mesh) {
            std::cout << "  UV Sphere: " << sphere_mesh->vertices().rows() << " vertices, " 
                     << sphere_mesh->faces().rows() << " faces\n";
            io::ObjExporter::export_mesh(*sphere_mesh, "examples/output/node_uv_sphere.obj");
        }

        // Sphere Node (Icosphere)
        std::cout << "Testing SphereNode (Icosphere)...\n";
        auto icosphere_node = nodes::SphereNode::create_icosphere(1.0, 2);
        auto icosphere_mesh = icosphere_node.generate();
        if (icosphere_mesh) {
            std::cout << "  Icosphere: " << icosphere_mesh->vertices().rows() << " vertices, " 
                     << icosphere_mesh->faces().rows() << " faces\n";
            io::ObjExporter::export_mesh(*icosphere_mesh, "examples/output/node_icosphere.obj");
        }

        // Cylinder Node
        std::cout << "Testing CylinderNode...\n";
        nodes::CylinderNode cylinder_node(0.5, 2.0, 12, 3, true, true);
        auto cylinder_mesh = cylinder_node.generate();
        if (cylinder_mesh) {
            std::cout << "  Cylinder: " << cylinder_mesh->vertices().rows() << " vertices, " 
                     << cylinder_mesh->faces().rows() << " faces\n";
            io::ObjExporter::export_mesh(*cylinder_mesh, "examples/output/node_cylinder.obj");
        }

        // Plane Node
        std::cout << "Testing PlaneNode...\n";
        nodes::PlaneNode plane_node(3.0, 3.0, 3, 3);
        auto plane_mesh = plane_node.generate();
        if (plane_mesh) {
            std::cout << "  Plane: " << plane_mesh->vertices().rows() << " vertices, " 
                     << plane_mesh->faces().rows() << " faces\n";
            io::ObjExporter::export_mesh(*plane_mesh, "examples/output/node_plane.obj");
        }
    }

    std::cout << "\n=== Node Parameter Modification ===\n";
    
    // Test parameter modification
    {
        nodes::SphereNode modifiable_sphere;
        
        // Default parameters
        auto default_mesh = modifiable_sphere.generate();
        if (default_mesh) {
            std::cout << "Default sphere: " << default_mesh->vertices().rows() << " vertices\n";
        }
        
        // Modify parameters
        modifiable_sphere.set_radius(1.5);
        modifiable_sphere.set_u_segments(24);
        modifiable_sphere.set_v_segments(12);
        
        auto modified_mesh = modifiable_sphere.generate();
        if (modified_mesh) {
            std::cout << "Modified sphere: " << modified_mesh->vertices().rows() << " vertices\n";
            io::ObjExporter::export_mesh(*modified_mesh, "examples/output/node_modified_sphere.obj");
        }
    }

    std::cout << "\n=== Complex Scene with Nodes ===\n";
    
    // Create a complex scene using boolean operations
    {
        // Create base sphere
        nodes::SphereNode sphere(1.0, 20, 10);
        auto sphere_mesh = sphere.generate();
        
        // Create cylinder to subtract
        nodes::CylinderNode cylinder(0.4, 2.5, 16, 1, true, true);
        auto cylinder_mesh = cylinder.generate();
        
        // Create boxes to add
        auto box_node1 = nodes::BoxNode::create_from_bounds(
            Eigen::Vector3d(-0.8, -0.8, -0.8),
            Eigen::Vector3d(-0.4, 0.8, 0.8),
            2, 2, 2
        );
        auto box_mesh1 = box_node1.generate();
        
        auto box_node2 = nodes::BoxNode::create_from_bounds(
            Eigen::Vector3d(0.4, -0.8, -0.8),
            Eigen::Vector3d(0.8, 0.8, 0.8),
            2, 2, 2
        );
        auto box_mesh2 = box_node2.generate();
        
        if (sphere_mesh && cylinder_mesh && box_mesh1 && box_mesh2) {
            std::cout << "Performing complex boolean operations...\n";
            
            // Step 1: Subtract cylinder from sphere
            auto step1 = geometry::BooleanOps::difference_meshes(*sphere_mesh, *cylinder_mesh);
            if (step1) {
                std::cout << "  After cylinder subtraction: " << step1->vertices().rows() << " vertices\n";
                io::ObjExporter::export_mesh(*step1, "examples/output/scene_step1.obj");
                
                // Step 2: Add first box
                auto step2 = geometry::BooleanOps::union_meshes(*step1, *box_mesh1);
                if (step2) {
                    std::cout << "  After adding box1: " << step2->vertices().rows() << " vertices\n";
                    io::ObjExporter::export_mesh(*step2, "examples/output/scene_step2.obj");
                    
                    // Step 3: Add second box
                    auto final_result = geometry::BooleanOps::union_meshes(*step2, *box_mesh2);
                    if (final_result) {
                        std::cout << "  Final result: " << final_result->vertices().rows() << " vertices, "
                                 << final_result->faces().rows() << " faces\n";
                        io::ObjExporter::export_mesh(*final_result, "examples/output/complex_scene_final.obj");
                    }
                }
            }
        }
    }

    std::cout << "\nNode system demonstration complete!\n";
    std::cout << "Check the examples/output/ directory for all generated meshes.\n";
    
    return 0;
}
