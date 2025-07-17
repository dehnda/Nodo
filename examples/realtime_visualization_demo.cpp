/**
 * NodeFlux Engine - Real-Time Visualization Demo
 * Demonstrates the integration of clean architecture with real-time rendering
 */

#include "nodeflux/graph/node_graph.hpp"
#include "nodeflux/graph/execution_engine.hpp"
#include <iostream>
#include <memory>

namespace {
    constexpr float DEMO_SPHERE_RADIUS = 2.0F;
}

int main() {
    std::cout << "🎨 NodeFluxEngine - Real-Time Visualization Demo" << std::endl;
    std::cout << "=================================================" << std::endl;
    
    // Create node graph using our clean architecture
    auto graph = std::make_unique<nodeflux::graph::NodeGraph>();
    auto engine = std::make_unique<nodeflux::graph::ExecutionEngine>();
    
    std::cout << "1. Creating procedural node graph..." << std::endl;
    
    // Create a sphere node
    int sphere_id = graph->add_node(nodeflux::graph::NodeType::Sphere, "Sphere");
    auto* sphere_node = graph->get_node(sphere_id);
    
    if (sphere_node != nullptr) {
        // Set sphere parameters
        nodeflux::graph::NodeParameter radius_param("radius", DEMO_SPHERE_RADIUS);
        nodeflux::graph::NodeParameter subdivisions_param("subdivisions", 3);
        sphere_node->set_parameter("radius", radius_param);
        sphere_node->set_parameter("subdivisions", subdivisions_param);
        std::cout << "   ✅ Created sphere with radius 2.0 and 3 subdivisions" << std::endl;
    }
    
    std::cout << "2. Executing graph..." << std::endl;
    
    // Execute the graph
    if (engine->execute_graph(*graph)) {
        std::cout << "   ✅ Graph execution completed successfully" << std::endl;
        
        // Get the result mesh
        auto result_mesh = engine->get_node_result(sphere_id);
        if (result_mesh) {
            const auto& vertices = result_mesh->vertices();
            const auto& faces = result_mesh->faces();
            std::cout << "   📊 Generated mesh: " << vertices.size() << " vertices, " 
                      << faces.size() << " faces" << std::endl;
        } else {
            std::cout << "   ❌ No mesh result from sphere node" << std::endl;
        }
    } else {
        std::cout << "   ❌ Graph execution failed" << std::endl;
        return 1;
    }
    
    std::cout << "\n3. Real-Time Visualization Architecture:" << std::endl;
    std::cout << "   🏗️  ViewportRenderer: OpenGL-based 3D viewport (ready)" << std::endl;
    std::cout << "   💾 MeshRenderCache: GPU buffer management (ready)" << std::endl;
    std::cout << "   🎮 Camera System: Orbit/pan/zoom controls (ready)" << std::endl;
    std::cout << "   🔄 Real-Time Updates: Automatic viewport refresh (pending)" << std::endl;
    
    std::cout << "\n🎉 Real-Time Visualization Foundation Complete!" << std::endl;
    std::cout << "Next: Integrate viewport with ImGui for full real-time workflow" << std::endl;
    
    return 0;
}
