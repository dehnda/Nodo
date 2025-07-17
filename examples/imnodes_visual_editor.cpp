/**
 * NodeFluxEngine - ImNodes Visual Editor Demo
 * Complete visual node editor using ImNodes for procedural modeling
 */

#include "nodeflux/ui/node_graph_editor.hpp"
#include "nodeflux/io/obj_exporter.hpp"
#include <iostream>
#include <memory>

int main() {
    std::cout << "=== NodeFluxEngine ImNodes Visual Editor Demo ===\n";
    std::cout << "This demo showcases the visual node editor with ImNodes integration.\n\n";
    
    try {
        // Create the node editor
        auto editor = std::make_unique<nodeflux::ui::NodeGraphEditor>();
        
        // Initialize ImNodes
        editor->initialize();
        
        std::cout << "✓ ImNodes context initialized successfully\n";
        
        // Add some sample nodes to demonstrate the system
        std::cout << "1. Creating sample node graph...\n";
        
        int sphere_id = editor->add_node(nodeflux::ui::NodeType::Sphere, ImVec2(100, 100));
        int extrude_id = editor->add_node(nodeflux::ui::NodeType::Extrude, ImVec2(300, 100));
        int smooth_id = editor->add_node(nodeflux::ui::NodeType::Smooth, ImVec2(500, 100));
        
        std::cout << "   ✓ Added Sphere node (ID: " << sphere_id << ")\n";
        std::cout << "   ✓ Added Extrude node (ID: " << extrude_id << ")\n";
        std::cout << "   ✓ Added Smooth node (ID: " << smooth_id << ")\n";
        
        // Execute the graph to generate meshes
        std::cout << "2. Executing node graph...\n";
        editor->execute_graph();
        
        // Check if we got valid output meshes
        auto sphere_mesh = editor->get_node_output(sphere_id);
        auto extrude_mesh = editor->get_node_output(extrude_id);
        auto smooth_mesh = editor->get_node_output(smooth_id);
        
        if (sphere_mesh) {
            std::cout << "   ✓ Sphere node output: " << sphere_mesh->vertices().rows() << " vertices, "
                     << sphere_mesh->faces().rows() << " faces\n";
        }
        
        if (extrude_mesh) {
            std::cout << "   ✓ Extrude node output: " << extrude_mesh->vertices().rows() << " vertices, "
                     << extrude_mesh->faces().rows() << " faces\n";
        }
        
        if (smooth_mesh) {
            std::cout << "   ✓ Smooth node output: " << smooth_mesh->vertices().rows() << " vertices, "
                     << smooth_mesh->faces().rows() << " faces\n";
        }
        
        // Export the final result
        if (sphere_mesh) {
            std::cout << "3. Exporting sphere mesh...\n";
            bool export_success = nodeflux::io::ObjExporter::export_mesh(
                *sphere_mesh, "imnodes_sphere_output.obj");
            
            if (export_success) {
                std::cout << "   ✓ Exported to: imnodes_sphere_output.obj\n";
            } else {
                std::cout << "   ✗ Export failed\n";
            }
        }
        
        // Show editor stats
        std::cout << "\n=== Editor Statistics ===\n";
        std::cout << "Total nodes: " << editor->get_node_count() << "\n";
        std::cout << "Total links: " << editor->get_link_count() << "\n";
        
        // Cleanup
        editor->shutdown();
        std::cout << "✓ ImNodes context cleaned up\n";
        
        std::cout << "\n=== Demo Complete ===\n";
        std::cout << "This demonstrates the ImNodes integration:\n";
        std::cout << "• Visual node graph editor with ImNodes\n";
        std::cout << "• Real-time procedural mesh generation\n";
        std::cout << "• Node execution and caching system\n";
        std::cout << "• Mesh export capabilities\n";
        std::cout << "\nIn a full application, this would be integrated with:\n";
        std::cout << "• GLFW window management\n";
        std::cout << "• OpenGL mesh rendering\n";
        std::cout << "• Real-time parameter editing\n";
        std::cout << "• Interactive node connection creation\n";
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
