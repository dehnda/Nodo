/**
 * NodeFlux Engine - Clean Architecture Example
 * Demonstrates the new separated data model and execution engine
 */

#include "nodeflux/graph/node_graph.hpp"
#include "nodeflux/graph/execution_engine.hpp"
#include "nodeflux/io/obj_exporter.hpp"
#include <iostream>
#include <memory>

int main() {
    std::cout << "🏗️ NodeFluxEngine - Clean Architecture Demo\n";
    std::cout << "============================================\n\n";

    // Create the data model (pure C++, no UI dependencies)
    auto graph = std::make_unique<nodeflux::graph::NodeGraph>();
    
    // Set up callbacks to monitor changes
    graph->set_node_changed_callback([](int node_id) {
        std::cout << "📢 Node " << node_id << " changed\n";
    });
    
    graph->set_connection_changed_callback([](int connection_id) {
        std::cout << "🔗 Connection " << connection_id << " changed\n";
    });

    // Create the execution engine
    auto engine = std::make_unique<nodeflux::graph::ExecutionEngine>();
    
    // Set up execution callbacks
    engine->set_progress_callback([](int completed, int total) {
        std::cout << "⚡ Execution progress: " << completed << "/" << total << " nodes\n";
    });
    
    engine->set_error_callback([](const std::string& error, int node_id) {
        std::cout << "❌ Error in node " << node_id << ": " << error << "\n";
    });

    std::cout << "1. Creating procedural node graph...\n";
    
    // Create nodes using the clean data model
    int sphere_id = graph->add_node(nodeflux::graph::NodeType::Sphere, "MySphere");
    int extrude_id = graph->add_node(nodeflux::graph::NodeType::Extrude, "MyExtrude");
    int smooth_id = graph->add_node(nodeflux::graph::NodeType::Smooth, "MySmooth");
    
    std::cout << "   Created nodes: Sphere(" << sphere_id << ") -> Extrude(" << extrude_id << ") -> Smooth(" << smooth_id << ")\n";

    // Modify node parameters
    if (auto* sphere_node = graph->get_node(sphere_id)) {
        sphere_node->set_parameter("radius", nodeflux::graph::NodeParameter("radius", 1.5F));
        sphere_node->set_parameter("subdivisions", nodeflux::graph::NodeParameter("subdivisions", 3)); // Valid: 0-5
    }
    
    if (auto* extrude_node = graph->get_node(extrude_id)) {
        extrude_node->set_parameter("distance", nodeflux::graph::NodeParameter("distance", 0.5F));
    }

    std::cout << "\n2. Connecting nodes...\n";
    
    // Create connections
    int conn1 = graph->add_connection(sphere_id, 0, extrude_id, 0);
    int conn2 = graph->add_connection(extrude_id, 0, smooth_id, 0);
    
    std::cout << "   Created connections: " << conn1 << ", " << conn2 << "\n";

    std::cout << "\n3. Validating graph structure...\n";
    
    // Validate the graph
    if (graph->is_valid()) {
        std::cout << "   ✅ Graph is valid (no cycles detected)\n";
        
        auto execution_order = graph->get_execution_order();
        std::cout << "   📋 Execution order: ";
        for (size_t i = 0; i < execution_order.size(); ++i) {
            std::cout << execution_order[i];
            if (i < execution_order.size() - 1) std::cout << " -> ";
        }
        std::cout << "\n";
    } else {
        std::cout << "   ❌ Graph is invalid (cycles detected)\n";
        return 1;
    }

    std::cout << "\n4. Executing graph...\n";
    
    // Execute the entire graph
    if (engine->execute_graph(*graph)) {
        std::cout << "   ✅ Graph execution completed successfully\n";
        
        // Get final result
        auto final_mesh = engine->get_node_result(smooth_id);
        if (final_mesh) {
            std::cout << "   📊 Final mesh stats:\n";
            std::cout << "      Vertices: " << final_mesh->vertex_count() << "\n";
            std::cout << "      Faces: " << final_mesh->face_count() << "\n";
            
            // Export the result
            std::cout << "\n5. Exporting result...\n";
            if (nodeflux::io::ObjExporter::export_mesh(*final_mesh, "clean_architecture_demo.obj")) {
                std::cout << "   ✅ Exported to: clean_architecture_demo.obj\n";
            } else {
                std::cout << "   ❌ Export failed\n";
            }
        } else {
            std::cout << "   ❌ No mesh result from final node\n";
        }
    } else {
        std::cout << "   ❌ Graph execution failed\n";
        return 1;
    }

    std::cout << "\n6. Testing parameter updates...\n";
    
    // Test parameter changes and re-execution
    if (auto* sphere_node = graph->get_node(sphere_id)) {
        std::cout << "   Changing sphere radius to 2.0...\n";
        sphere_node->set_parameter("radius", nodeflux::graph::NodeParameter("radius", 2.0F));
        
        // Re-execute only affected nodes
        if (engine->execute_graph(*graph)) {
            auto updated_mesh = engine->get_node_result(smooth_id);
            if (updated_mesh) {
                std::cout << "   ✅ Updated mesh stats:\n";
                std::cout << "      Vertices: " << updated_mesh->vertex_count() << "\n";
                std::cout << "      Faces: " << updated_mesh->face_count() << "\n";
            }
        }
    }

    std::cout << "\n🎉 Clean Architecture Demo Complete!\n\n";
    std::cout << "Key Benefits Demonstrated:\n";
    std::cout << "• ✅ Separation of data model and execution logic\n";
    std::cout << "• ✅ Event-driven change notifications\n";
    std::cout << "• ✅ Dependency resolution and topological sorting\n";
    std::cout << "• ✅ Parameter modification and re-execution\n";
    std::cout << "• ✅ No UI coupling - works headless\n";
    std::cout << "• ✅ Ready for serialization and real-time rendering\n";

    return 0;
}
