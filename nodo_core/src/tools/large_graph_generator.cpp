/**
 * Large Graph Generator
 * Creates test graphs with many nodes for performance testing
 */

#include "nodo/graph/graph_serializer.hpp"
#include "nodo/graph/node_graph.hpp"
#include <iostream>

using namespace nodo::graph;

// Generate a chain of transform nodes
void generate_transform_chain(NodeGraph &graph, int chain_length) {
  int prev_node_id = -1;

  for (int i = 0; i < chain_length; ++i) {
    int node_id = graph.get_next_node_id();

    if (i == 0) {
      // First node: Sphere generator
      auto sphere = graph.create_node(NodeType::Sphere, "Sphere");
      sphere->set_parameter("radius", 1.0f);
      sphere->set_parameter("rows", 32);
      sphere->set_parameter("columns", 64);
      prev_node_id = sphere->get_id();
    } else {
      // Transform nodes
      auto transform = graph.create_node(NodeType::Transform,
                                         "Transform_" + std::to_string(i));
      transform->set_parameter("translate_x", 0.01f * i);
      transform->set_parameter("translate_y", 0.0f);
      transform->set_parameter("translate_z", 0.0f);
      transform->set_parameter("rotate_x", 0.0f);
      transform->set_parameter("rotate_y", 1.0f * i);
      transform->set_parameter("rotate_z", 0.0f);
      transform->set_parameter("scale", 1.0f);

      // Connect to previous node
      graph.create_connection(prev_node_id, 0, transform->get_id(), 0);
      prev_node_id = transform->get_id();
    }
  }

  // Set last node as display
  graph.set_display_node(prev_node_id);
}

// Generate a scatter + copy pattern
void generate_scatter_grid(NodeGraph &graph, int grid_size) {
  // Create base geometry (box)
  auto box = graph.create_node(NodeType::Box, "Box");
  box->set_parameter("size_x", 0.1f);
  box->set_parameter("size_y", 0.1f);
  box->set_parameter("size_z", 0.1f);

  int prev_display = box->get_id();

  // Create grid pattern with scatter + copy
  for (int i = 0; i < grid_size; ++i) {
    // Create points to scatter on
    auto sphere =
        graph.create_node(NodeType::Sphere, "Sphere_" + std::to_string(i));
    sphere->set_parameter("radius", 2.0f);
    sphere->set_parameter("rows", 8);
    sphere->set_parameter("columns", 16);

    // Scatter points
    auto scatter =
        graph.create_node(NodeType::Scatter, "Scatter_" + std::to_string(i));
    scatter->set_parameter("count", 20);
    scatter->set_parameter("seed", i);
    graph.create_connection(sphere->get_id(), 0, scatter->get_id(), 0);

    // Copy to points
    auto copy =
        graph.create_node(NodeType::CopyToPoints, "Copy_" + std::to_string(i));
    graph.create_connection(box->get_id(), 0, copy->get_id(), 0);
    graph.create_connection(scatter->get_id(), 0, copy->get_id(), 1);

    prev_display = copy->get_id();
  }

  graph.set_display_node(prev_display);
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cout << "Large Graph Generator\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << argv[0] << " <type> <size> <output.nfg>\n\n";
    std::cout << "Types:\n";
    std::cout << "  chain     - Linear chain of transform nodes\n";
    std::cout << "  scatter   - Scatter + copy pattern\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << argv[0] << " chain 100 large_chain.nfg\n";
    std::cout << "  " << argv[0] << " scatter 10 large_scatter.nfg\n";
    return 1;
  }

  std::string type = argv[1];
  int size = std::stoi(argv[2]);
  std::string output_file = argv[3];

  NodeGraph graph;

  std::cout << "Generating " << type << " graph with size " << size << "...\n";

  if (type == "chain") {
    generate_transform_chain(graph, size);
  } else if (type == "scatter") {
    generate_scatter_grid(graph, size);
  } else {
    std::cerr << "Unknown type: " << type << "\n";
    return 1;
  }

  std::cout << "Generated " << graph.get_nodes().size() << " nodes\n";

  // Save to file
  if (GraphSerializer::save_to_file(graph, output_file)) {
    std::cout << "✓ Saved to: " << output_file << "\n";
    return 0;
  } else {
    std::cerr << "✗ Failed to save graph\n";
    return 1;
  }
}
