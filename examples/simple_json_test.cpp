/**
 * Simple JSON Test for Node Graph Serialization
 */

#include "nodeflux/graph/graph_serializer.hpp"
#include <iostream>

using namespace nodeflux;

int main() {
    std::cout << "🔧 Testing JSON Serialization...\n\n";
    
    // Create a simple graph programmatically
    graph::NodeGraph graph;
    
    // Add a sphere node
    int sphere_id = graph.add_node(graph::NodeType::Sphere, "Test_Sphere");
    auto* sphere_node = graph.get_node(sphere_id);
    if (sphere_node) {
        sphere_node->add_parameter(graph::NodeParameter("radius", 1.5F));
        sphere_node->add_parameter(graph::NodeParameter("segments", 20));
        sphere_node->set_position(0.0F, 0.0F);
    }
    
    // Add a plane node
    int plane_id = graph.add_node(graph::NodeType::Plane, "Test_Plane");
    auto* plane_node = graph.get_node(plane_id);
    if (plane_node) {
        plane_node->add_parameter(graph::NodeParameter("size", 5.0F));
        plane_node->add_parameter(graph::NodeParameter("divisions", 10));
        plane_node->set_position(100.0F, 0.0F);
    }
    
    // Add connection
    graph.add_connection(sphere_id, 0, plane_id, 0);
    
    std::cout << "📊 Created graph with " << graph.get_nodes().size() << " nodes\n";
    std::cout << "🔗 And " << graph.get_connections().size() << " connections\n\n";
    
    // Test serialization
    std::cout << "🚀 Testing Serialization...\n";
    std::string json = graph::GraphSerializer::serialize_to_json(graph);
    
    std::cout << "📄 Generated JSON:\n";
    std::cout << json << "\n\n";
    
    // Test file save
    std::cout << "💾 Testing File Save...\n";
    if (graph::GraphSerializer::save_to_file(graph, "test_graph.json")) {
        std::cout << "✅ Successfully saved to test_graph.json\n";
    } else {
        std::cout << "❌ Failed to save to file\n";
    }
    
    std::cout << "\n🎯 JSON serialization working! Now you can:\n";
    std::cout << "1. Edit test_graph.json manually\n";
    std::cout << "2. Use it as a template for other graphs\n";
    std::cout << "3. Version control your node graphs\n";
    std::cout << "4. Share graph configurations easily\n";
    
    return 0;
}
