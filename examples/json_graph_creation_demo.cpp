/**
 * NodeFlux Engine - JSON Graph Creation Demo
 * 
 * This example shows how to easily create node graphs using JSON files,
 * demonstrating the simple workflow for procedural modeling setup.
 */

#include "nodeflux/graph/graph_serializer.hpp"
#include "nodeflux/graph/execution_engine.hpp"
#include <iostream>
#include <sstream>

using namespace nodeflux;

/**
 * @brief Helper class for creating graphs with fluent JSON API
 */
class GraphBuilder {
private:
    std::ostringstream json_;
    bool has_nodes_ = false;
    bool has_connections_ = false;
    
public:
    GraphBuilder() {
        json_ << "{\n";
        json_ << "  \"version\": \"1.0\",\n";
        json_ << "  \"nodes\": [\n";
    }
    
    GraphBuilder& add_sphere(const std::string& name, float radius = 1.0F, int segments = 16, float pos_x = 0.0F, float pos_y = 0.0F) {
        if (has_nodes_) json_ << ",\n";
        
        json_ << "    {\n";
        json_ << "      \"id\": " << (has_nodes_ ? get_next_id() : 0) << ",\n";
        json_ << "      \"type\": \"Sphere\",\n";
        json_ << "      \"name\": \"" << name << "\",\n";
        json_ << "      \"position\": [" << pos_x << ", " << pos_y << "],\n";
        json_ << "      \"parameters\": [\n";
        json_ << "        {\"name\": \"radius\", \"type\": \"float\", \"value\": " << radius << "},\n";
        json_ << "        {\"name\": \"segments\", \"type\": \"int\", \"value\": " << segments << "}\n";
        json_ << "      ]\n";
        json_ << "    }";
        
        has_nodes_ = true;
        return *this;
    }
    
    GraphBuilder& add_plane(const std::string& name, float size = 2.0F, int divisions = 10, float pos_x = 0.0F, float pos_y = 0.0F) {
        if (has_nodes_) json_ << ",\n";
        
        json_ << "    {\n";
        json_ << "      \"id\": " << get_next_id() << ",\n";
        json_ << "      \"type\": \"Plane\",\n";
        json_ << "      \"name\": \"" << name << "\",\n";
        json_ << "      \"position\": [" << pos_x << ", " << pos_y << "],\n";
        json_ << "      \"parameters\": [\n";
        json_ << "        {\"name\": \"size\", \"type\": \"float\", \"value\": " << size << "},\n";
        json_ << "        {\"name\": \"divisions\", \"type\": \"int\", \"value\": " << divisions << "}\n";
        json_ << "      ]\n";
        json_ << "    }";
        
        has_nodes_ = true;
        return *this;
    }
    
    GraphBuilder& add_boolean(const std::string& name, const std::string& operation = "union", float pos_x = 0.0F, float pos_y = 0.0F) {
        if (has_nodes_) json_ << ",\n";
        
        json_ << "    {\n";
        json_ << "      \"id\": " << get_next_id() << ",\n";
        json_ << "      \"type\": \"Boolean\",\n";
        json_ << "      \"name\": \"" << name << "\",\n";
        json_ << "      \"position\": [" << pos_x << ", " << pos_y << "],\n";
        json_ << "      \"parameters\": [\n";
        json_ << "        {\"name\": \"operation\", \"type\": \"string\", \"value\": \"" << operation << "\"}\n";
        json_ << "      ]\n";
        json_ << "    }";
        
        has_nodes_ = true;
        return *this;
    }
    
    GraphBuilder& connect(int source_node, int target_node, int source_pin = 0, int target_pin = 0) {
        if (!has_connections_) {
            json_ << "\n  ],\n";
            json_ << "  \"connections\": [\n";
            has_connections_ = true;
        } else {
            json_ << ",\n";
        }
        
        json_ << "    {\n";
        json_ << "      \"id\": " << connection_count_ << ",\n";
        json_ << "      \"source_node\": " << source_node << ",\n";
        json_ << "      \"source_pin\": " << source_pin << ",\n";
        json_ << "      \"target_node\": " << target_node << ",\n";
        json_ << "      \"target_pin\": " << target_pin << "\n";
        json_ << "    }";
        
        connection_count_++;
        return *this;
    }
    
    std::string build() {
        if (!has_connections_) {
            json_ << "\n  ],\n";
            json_ << "  \"connections\": []\n";
        } else {
            json_ << "\n  ]\n";
        }
        json_ << "}\n";
        return json_.str();
    }
    
private:
    int node_count_ = 0;
    int connection_count_ = 0;
    
    int get_next_id() {
        return ++node_count_;
    }
};

/**
 * @brief Simple JSON templates for common workflows
 */
std::string create_simple_sphere_json() {
    return "{\n"
           "  \"version\": \"1.0\",\n"
           "  \"nodes\": [\n"
           "    {\n"
           "      \"id\": 0,\n"
           "      \"type\": \"Sphere\",\n"
           "      \"name\": \"Basic_Sphere\",\n"
           "      \"position\": [0.0, 0.0],\n"
           "      \"parameters\": [\n"
           "        {\"name\": \"radius\", \"type\": \"float\", \"value\": 1.5},\n"
           "        {\"name\": \"segments\", \"type\": \"int\", \"value\": 20}\n"
           "      ]\n"
           "    }\n"
           "  ],\n"
           "  \"connections\": []\n"
           "}";
}

/**
 * @brief Demonstration of JSON-based graph creation
 */
void demonstrate_json_graph_creation() {
    std::cout << "🎯 NodeFlux JSON Graph Creation Demo\n";
    std::cout << "===================================\n\n";
    
    // =================================================================
    // Method 1: Using Simple JSON Template
    // =================================================================
    std::cout << "📋 Method 1: Using Simple JSON Template\n";
    std::cout << "---------------------------------------\n";
    
    std::cout << "\n🔸 Creating Simple Sphere from JSON template...\n";
    auto sphere_json = create_simple_sphere_json();
    std::cout << "\n📄 Generated JSON:\n" << sphere_json << "\n\n";
    
    auto sphere_graph = graph::GraphSerializer::deserialize_from_json(sphere_json);
    if (sphere_graph.has_value()) {
        std::cout << "✅ Sphere graph loaded with " << sphere_graph->get_nodes().size() << " nodes\n";
        
        // Save to file for inspection
        if (graph::GraphSerializer::save_to_file(*sphere_graph, "simple_sphere_graph.json")) {
            std::cout << "💾 Saved to: simple_sphere_graph.json\n";
        }
    } else {
        std::cout << "❌ Failed to load sphere graph\n";
    }
    
    // =================================================================
    // Method 2: Using Fluent Builder API
    // =================================================================
    std::cout << "\n📋 Method 2: Using Fluent Builder API\n";
    std::cout << "------------------------------------\n";
    
    std::cout << "\n🔸 Building Custom Graph with Fluent API...\n";
    
    auto custom_json = GraphBuilder()
        .add_sphere("Main_Sphere", 2.0F, 24, -150.0F, 0.0F)
        .add_plane("Ground_Plane", 5.0F, 30, -150.0F, 150.0F)
        .add_boolean("Union_Op", "union", 50.0F, 75.0F)
        .connect(0, 2, 0, 0)  // sphere -> boolean
        .connect(1, 2, 0, 1)  // plane -> boolean
        .build();
    
    std::cout << "\n📄 Generated JSON:\n";
    std::cout << custom_json << "\n";
    
    auto custom_graph = graph::GraphSerializer::deserialize_from_json(custom_json);
    if (custom_graph.has_value()) {
        std::cout << "✅ Custom graph created with " << custom_graph->get_nodes().size() << " nodes\n";
        
        if (graph::GraphSerializer::save_to_file(*custom_graph, "custom_fluent_graph.json")) {
            std::cout << "💾 Saved to: custom_fluent_graph.json\n";
        }
    }
    
    // =================================================================
    // Method 3: Loading and Executing Graphs
    // =================================================================
    std::cout << "\n📋 Method 3: Loading and Executing Graphs\n";
    std::cout << "-----------------------------------------\n";
    
    std::cout << "\n🔸 Loading graph from file...\n";
    auto loaded_graph = graph::GraphSerializer::load_from_file("simple_sphere_graph.json");
    if (loaded_graph.has_value()) {
        std::cout << "✅ Graph loaded from file successfully\n";
        
        // You can now execute this graph with ExecutionEngine
        graph::ExecutionEngine engine;
        // engine.execute_graph(*loaded_graph); // Would execute the procedural workflow
        
        std::cout << "🚀 Graph ready for execution with ExecutionEngine\n";
    }
    
    std::cout << "\n🎉 JSON Graph Creation Demo Complete!\n";
    std::cout << "====================================\n";
    
    std::cout << "\n💡 What You Can Do Now:\n";
    std::cout << "1. Edit the generated JSON files to modify graphs\n";
    std::cout << "2. Create your own JSON templates for common workflows\n";
    std::cout << "3. Use the GraphBuilder for programmatic graph creation\n";
    std::cout << "4. Load graphs dynamically at runtime\n";
    std::cout << "5. Share graph configurations as simple JSON files\n";
    std::cout << "6. Version control your procedural workflows\n";
    
    std::cout << "\n🎯 This gives you exactly what you wanted:\n";
    std::cout << "✅ Easy graph creation via JSON\n";
    std::cout << "✅ Human-readable graph representation\n";
    std::cout << "✅ Simple save/load functionality\n";
    std::cout << "✅ Fluent API for programmatic creation\n";
    std::cout << "✅ Template system for common patterns\n";
}

int main() {
    try {
        demonstrate_json_graph_creation();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "❌ Error in JSON graph demo: " << error.what() << "\n";
        return 1;
    }
}
