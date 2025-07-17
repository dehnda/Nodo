#include "nodeflux/graph/node_graph.hpp"
#include <iostream>

int main() {
    nodeflux::graph::NodeGraph graph;
    int transform_id = graph.add_node(nodeflux::graph::NodeType::Transform, "Test Transform");
    
    auto* node = graph.get_node(transform_id);
    if (node) {
        std::cout << "Transform node created with:" << std::endl;
        std::cout << "  Input pins: " << node->get_input_pins().size() << std::endl;
        std::cout << "  Output pins: " << node->get_output_pins().size() << std::endl;
        
        for (const auto& pin : node->get_input_pins()) {
            std::cout << "  Input pin: " << pin.name << std::endl;
        }
        
        for (const auto& pin : node->get_output_pins()) {
            std::cout << "  Output pin: " << pin.name << std::endl;
        }
    }
    
    return 0;
}
