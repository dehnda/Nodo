/**
 * Large Graph Generator
 * Creates test graphs with many nodes for performance testing
 */

#include "nodo/graph/graph_serializer.hpp"
#include "nodo/graph/node_graph.hpp"

#include <iostream>

using namespace nodo::graph;

// Generate a chain of transform nodes
void generate_transform_chain(NodeGraph& graph, int chain_length) {
  int prev_node_id = -1;

  for (int i = 0; i < chain_length; ++i) {
    if (i == 0) {
      // First node: Sphere generator
      int sphere_id = graph.add_node(NodeType::Sphere);
      auto* sphere = graph.get_node(sphere_id);
      sphere->set_parameter("radius", 1.0F);
      sphere->set_parameter("u_segments", 32);
      sphere->set_parameter("v_segments", 64);
      prev_node_id = sphere_id;
    } else {
      // Transform nodes
      int transform_id = graph.add_node(NodeType::Transform);
      auto* transform = graph.get_node(transform_id);
      transform->set_parameter("translate_x", 0.01F * static_cast<float>(i));
      transform->set_parameter("translate_y", 0.0F);
      transform->set_parameter("translate_z", 0.0F);
      transform->set_parameter("rotate_x", 0.0F);
      transform->set_parameter("rotate_y", 1.0F * static_cast<float>(i));
      transform->set_parameter("rotate_z", 0.0F);
      transform->set_parameter("scale", 1.0F);

      // Connect to previous node
      graph.add_connection(prev_node_id, 0, transform_id, 0);
      prev_node_id = transform_id;
    }
  }

  // Set last node as display
  graph.set_display_node(prev_node_id);
}

// Generate a scatter + copy pattern
void generate_scatter_grid(NodeGraph& graph, int grid_size) {
  // Create base geometry (box)
  int box_id = graph.add_node(NodeType::Box);
  auto* box = graph.get_node(box_id);
  box->set_parameter("size_x", NodeParameter::from_float(0.1F));
  box->set_parameter("size_y", NodeParameter::from_float(0.1F));
  box->set_parameter("size_z", NodeParameter::from_float(0.1F));

  int prev_display = box_id;

  // Create grid pattern with scatter + copy
  for (int i = 0; i < grid_size; ++i) {
    // Create points to scatter on
    int sphere_id = graph.add_node(NodeType::Sphere);
    auto* sphere = graph.get_node(sphere_id);
    sphere->set_parameter("radius", NodeParameter::from_float(2.0F));
    sphere->set_parameter("u_segments", NodeParameter::from_int(8));
    sphere->set_parameter("v_segments", NodeParameter::from_int(16));

    // Scatter points
    int scatter_id = graph.add_node(NodeType::Scatter);
    auto* scatter = graph.get_node(scatter_id);
    scatter->set_parameter("count", NodeParameter::from_int(20));
    scatter->set_parameter("seed", NodeParameter::from_int(i));
    graph.add_connection(sphere_id, 0, scatter_id, 0);

    // Copy to points
    int copy_id = graph.add_node(NodeType::CopyToPoints);
    graph.add_connection(box_id, 0, copy_id, 0);
    graph.add_connection(scatter_id, 0, copy_id, 1);

    prev_display = copy_id;
  }

  graph.set_display_node(prev_display);
}

int main(int argc, char* argv[]) {
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
