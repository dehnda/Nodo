/**
 * JSON Template Loader Demo
 * Shows how to easily load and use JSON graph templates
 */

#include "nodeflux/graph/graph_serializer.hpp"
#include <iostream>
#include <filesystem>

using namespace nodeflux;

void demonstrate_template_loading() {
    std::cout << "🎯 JSON Template Loading Demo\n";
    std::cout << "=============================\n\n";
    
    // List of available templates
    std::vector<std::string> templates = {
        "templates/simple_sphere.json",
        "templates/boolean_union.json", 
        "templates/procedural_instancing.json"
    };
    
    for (const auto& template_path : templates) {
        std::cout << "📂 Loading template: " << template_path << "\n";
        
        // Check if file exists
        if (!std::filesystem::exists(template_path)) {
            std::cout << "❌ Template file not found: " << template_path << "\n\n";
            continue;
        }
        
        // Load the graph
        auto graph = graph::GraphSerializer::load_from_file(template_path);
        if (graph.has_value()) {
            std::cout << "✅ Successfully loaded graph\n";
            std::cout << "   Nodes: " << graph->get_nodes().size() << "\n";
            std::cout << "   Connections: " << graph->get_connections().size() << "\n";
            
            // You could now modify the graph programmatically
            // or execute it with the ExecutionEngine
            
            // Example: Save a modified version
            std::string output_name = "loaded_" + std::filesystem::path(template_path).filename().string();
            if (graph::GraphSerializer::save_to_file(*graph, output_name)) {
                std::cout << "💾 Saved copy to: " << output_name << "\n";
            }
        } else {
            std::cout << "❌ Failed to load template\n";
        }
        std::cout << "\n";
    }
    
    std::cout << "🎉 Template Loading Complete!\n";
    std::cout << "=============================\n\n";
    
    std::cout << "💡 Now you can:\n";
    std::cout << "1. Edit the JSON files directly to create custom workflows\n";
    std::cout << "2. Copy templates and modify them for new projects\n";
    std::cout << "3. Create a library of reusable procedural patterns\n";
    std::cout << "4. Share workflows with team members as JSON files\n";
    std::cout << "5. Version control your procedural modeling setups\n";
}

int main() {
    try {
        demonstrate_template_loading();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "❌ Error: " << error.what() << "\n";
        return 1;
    }
}
