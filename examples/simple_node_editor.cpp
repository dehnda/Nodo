/**
 * Simple Node Editor Demo - Console-based procedural workflow
 * Demonstrates NodeFluxEngine procedural system without UI complexity
 */

#include "nodeflux/ui/node_graph_editor.hpp"
#include "nodeflux/io/obj_exporter.hpp"
#include "nodeflux/geometry/mesh_generator.hpp"
#include "nodeflux/sop/extrude_sop.hpp"
#include "nodeflux/sop/laplacian_sop.hpp"
#include <iostream>
#include <memory>

int main() {
    std::cout << "=== NodeFluxEngine Simple Node Editor Demo ===\n";
    std::cout << "This demo shows the procedural mesh generation system.\n\n";
    
    try {
        // Simulate basic procedural workflow
        std::cout << "1. Creating procedural box...\n";
        constexpr double BOX_SIZE = 1.0;
        auto box_mesh = nodeflux::geometry::MeshGenerator::box(
            Eigen::Vector3d(-BOX_SIZE, -BOX_SIZE, -BOX_SIZE),
            Eigen::Vector3d(BOX_SIZE, BOX_SIZE, BOX_SIZE)
        );
        auto box_shared = std::make_shared<nodeflux::core::Mesh>(std::move(box_mesh));
        
        std::cout << "   ✓ Box: " << box_shared->vertices().rows() << " vertices, "
                  << box_shared->faces().rows() << " faces\n";
        
        // Apply extrusion
        std::cout << "2. Applying face extrusion...\n";
        constexpr double EXTRUDE_DISTANCE = 0.2;
        nodeflux::sop::ExtrudeSOP extrude_sop("extrude_demo");
        extrude_sop.set_input_mesh(box_shared);
        extrude_sop.set_mode(nodeflux::sop::ExtrudeSOP::ExtrusionMode::FACE_NORMALS);
        extrude_sop.set_distance(EXTRUDE_DISTANCE);
        
        auto extruded = extrude_sop.cook();
        if (extruded) {
            std::cout << "   ✓ Extruded: " << extruded->vertices().rows() << " vertices, "
                     << extruded->faces().rows() << " faces\n";
            
            // Apply smoothing
            std::cout << "3. Applying Laplacian smoothing...\n";
            constexpr int SMOOTH_ITERATIONS = 3;
            constexpr double SMOOTH_LAMBDA = 0.5;
            
            nodeflux::sop::LaplacianSOP smooth_sop("smooth_demo");
            smooth_sop.set_input_mesh(extruded);
            smooth_sop.set_method(nodeflux::sop::LaplacianSOP::SmoothingMethod::UNIFORM);
            smooth_sop.set_iterations(SMOOTH_ITERATIONS);
            smooth_sop.set_lambda(SMOOTH_LAMBDA);
            
            auto smoothed = smooth_sop.cook();
            if (smoothed) {
                std::cout << "   ✓ Smoothed: " << smoothed->vertices().rows() << " vertices, "
                         << smoothed->faces().rows() << " faces\n";
                
                // Export final result
                std::cout << "4. Exporting final mesh...\n";
                bool export_success = nodeflux::io::ObjExporter::export_mesh(
                    *smoothed, "procedural_demo_output.obj");
                
                if (export_success) {
                    std::cout << "   ✓ Exported to: procedural_demo_output.obj\n";
                } else {
                    std::cout << "   ✗ Export failed\n";
                }
            } else {
                std::cout << "   ✗ Smoothing failed\n";
            }
        } else {
            std::cout << "   ✗ Extrusion failed\n";
        }
        
        std::cout << "\n=== Demo Complete ===\n";
        std::cout << "This demonstrates the complete procedural pipeline:\n";
        std::cout << "• Box generation with configurable parameters\n";
        std::cout << "• Face extrusion with multiple modes\n";
        std::cout << "• Laplacian smoothing with various methods\n";
        std::cout << "• OBJ export for external applications\n";
        std::cout << "\nThe visual node editor would provide interactive control\n";
        std::cout << "over these parameters with real-time mesh preview.\n";
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
