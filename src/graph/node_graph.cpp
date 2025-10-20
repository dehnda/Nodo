/**
 * NodeFlux Engine - Core Node Graph Implementation
 */

#include "nodeflux/graph/node_graph.hpp"
#include <algorithm>
#include <queue>
#include <unordered_set>

namespace nodeflux::graph {

GraphNode::GraphNode(int node_id, NodeType type, const std::string &name)
    : id_(node_id), type_(type), name_(name) {
  setup_pins_for_type();
}

void GraphNode::add_parameter(const NodeParameter &param) {
  auto iterator = std::find_if(parameters_.begin(), parameters_.end(),
                               [&param](const NodeParameter &parameter) {
                                 return parameter.name == param.name;
                               });

  if (iterator != parameters_.end()) {
    *iterator = param;
  } else {
    parameters_.push_back(param);
  }
  needs_update_ = true;
}

std::optional<NodeParameter>
GraphNode::get_parameter(const std::string &name) const {
  auto it =
      std::find_if(parameters_.begin(), parameters_.end(),
                   [&name](const NodeParameter &p) { return p.name == name; });

  return (it != parameters_.end()) ? std::make_optional(*it) : std::nullopt;
}

void GraphNode::set_parameter(const std::string &name,
                              const NodeParameter &param) {
  add_parameter(param);
}

void GraphNode::setup_pins_for_type() {
  input_pins_.clear();
  output_pins_.clear();

  // Setup default parameters and pins based on node type
  switch (type_) {
  case NodeType::Sphere:
    parameters_.emplace_back("radius", 1.0F);
    parameters_.emplace_back("u_segments", 32);
    parameters_.emplace_back("v_segments", 16);
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;

  case NodeType::Box:
    parameters_.emplace_back("width", 1.0F);
    parameters_.emplace_back("height", 1.0F);
    parameters_.emplace_back("depth", 1.0F);
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;

  case NodeType::Cylinder:
    parameters_.emplace_back("radius", 1.0F);
    parameters_.emplace_back("height", 2.0F);
    parameters_.emplace_back("segments", 32);
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;

  case NodeType::Extrude:
    parameters_.emplace_back("distance", 1.0F);
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input", 0});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;

  case NodeType::Smooth:
    parameters_.emplace_back("iterations", 5);
    parameters_.emplace_back("lambda", 0.5F);
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input", 0});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;

  case NodeType::Boolean:
    parameters_.emplace_back("operation",
                             0); // 0=Union, 1=Intersection, 2=Difference
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "A", 0});
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "B", 1});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Result", 0});
    break;

  case NodeType::Transform:
    parameters_.emplace_back("translate_x", 0.0F);
    parameters_.emplace_back("translate_y", 0.0F);
    parameters_.emplace_back("translate_z", 0.0F);
    parameters_.emplace_back("rotate_x", 0.0F);
    parameters_.emplace_back("rotate_y", 0.0F);
    parameters_.emplace_back("rotate_z", 0.0F);
    parameters_.emplace_back("scale_x", 1.0F);
    parameters_.emplace_back("scale_y", 1.0F);
    parameters_.emplace_back("scale_z", 1.0F);
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input", 0});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Output", 0});
    break;

  case NodeType::Plane:
    parameters_.emplace_back("width", 2.0F);
    parameters_.emplace_back("height", 2.0F);
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;

  case NodeType::Torus:
    parameters_.emplace_back("major_radius", 1.0F);
    parameters_.emplace_back("minor_radius", 0.3F);
    parameters_.emplace_back("major_segments", 48);
    parameters_.emplace_back("minor_segments", 24);
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;

  case NodeType::Line:
    parameters_.emplace_back("start_x", 0.0F);
    parameters_.emplace_back("start_y", 0.0F);
    parameters_.emplace_back("start_z", 0.0F);
    parameters_.emplace_back("end_x", 1.0F);
    parameters_.emplace_back("end_y", 0.0F);
    parameters_.emplace_back("end_z", 0.0F);
    parameters_.emplace_back("segments", 10);
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Line", 0});
    break;

  case NodeType::Resample:
    parameters_.emplace_back("mode", 0); // 0=BY_COUNT, 1=BY_LENGTH
    parameters_.emplace_back("point_count", 20);
    parameters_.emplace_back("segment_length", 0.1F);
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input", 0});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Resampled", 0});
    break;

  case NodeType::Merge:
    // Merge node combines multiple meshes into one
    // Start with 2 inputs, but can accept any number via multiple connections
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input 1", 0});
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input 2", 1});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Merged", 0});
    break;

  case NodeType::Array:
    // Array node creates copies in linear, grid, or radial patterns
    parameters_.emplace_back("mode", 0); // 0=Linear, 1=Grid, 2=Radial
    parameters_.emplace_back("count", 5);
    parameters_.emplace_back("offset_x", 2.0F);
    parameters_.emplace_back("offset_y",
                             2.0F); // Changed from 0.0F to 2.0F for grid mode
    parameters_.emplace_back("offset_z", 0.0F);
    // Grid-specific parameters
    parameters_.emplace_back("grid_rows", 3);
    parameters_.emplace_back("grid_cols", 3);
    // Radial-specific parameters
    parameters_.emplace_back("radius", 5.0F);
    parameters_.emplace_back("angle", 360.0F); // Total angle to span
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input", 0});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Array", 0});
    break;

  default:
    // Default: single input/output
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input", 0});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Output", 0});
    break;
  }
}

// NodeGraph Implementation

int NodeGraph::add_node(NodeType type, const std::string &name) {
  const int node_id = next_node_id_++;
  const std::string node_name = name.empty() ? generate_node_name(type) : name;

  auto node = std::make_unique<GraphNode>(node_id, type, node_name);
  nodes_.push_back(std::move(node));

  notify_node_changed(node_id);
  return node_id;
}

bool NodeGraph::remove_node(int node_id) {
  // Remove all connections to/from this node
  remove_connections_to_node(node_id);

  // Remove the node
  auto it = std::find_if(nodes_.begin(), nodes_.end(),
                         [node_id](const std::unique_ptr<GraphNode> &node) {
                           return node->get_id() == node_id;
                         });

  if (it != nodes_.end()) {
    nodes_.erase(it);
    notify_node_changed(node_id);
    return true;
  }

  return false;
}

GraphNode *NodeGraph::get_node(int node_id) {
  auto it = std::find_if(nodes_.begin(), nodes_.end(),
                         [node_id](const std::unique_ptr<GraphNode> &node) {
                           return node->get_id() == node_id;
                         });

  return (it != nodes_.end()) ? it->get() : nullptr;
}

const GraphNode *NodeGraph::get_node(int node_id) const {
  auto it = std::find_if(nodes_.begin(), nodes_.end(),
                         [node_id](const std::unique_ptr<GraphNode> &node) {
                           return node->get_id() == node_id;
                         });

  return (it != nodes_.end()) ? it->get() : nullptr;
}

int NodeGraph::add_connection(int source_node_id, int source_pin,
                              int target_node_id, int target_pin) {
  // Validate nodes exist
  if (!get_node(source_node_id) || !get_node(target_node_id)) {
    return -1;
  }

  // Check for existing connection to target pin
  auto existing =
      std::find_if(connections_.begin(), connections_.end(),
                   [target_node_id, target_pin](const NodeConnection &conn) {
                     return conn.target_node_id == target_node_id &&
                            conn.target_pin_index == target_pin;
                   });

  if (existing != connections_.end()) {
    // Remove existing connection (only one input per pin)
    remove_connection(existing->id);
  }

  // Add new connection
  const int connection_id = next_connection_id_++;
  connections_.push_back(
      {connection_id, source_node_id, source_pin, target_node_id, target_pin});

  // Mark target node for update
  if (auto *target_node = get_node(target_node_id)) {
    target_node->mark_for_update();
  }

  notify_connection_changed(connection_id);
  return connection_id;
}

bool NodeGraph::remove_connection(int connection_id) {
  auto it = std::find_if(connections_.begin(), connections_.end(),
                         [connection_id](const NodeConnection &conn) {
                           return conn.id == connection_id;
                         });

  if (it != connections_.end()) {
    // Mark target node for update
    if (auto *target_node = get_node(it->target_node_id)) {
      target_node->mark_for_update();
    }

    connections_.erase(it);
    notify_connection_changed(connection_id);
    return true;
  }

  return false;
}

bool NodeGraph::remove_connections_to_node(int node_id) {
  bool removed_any = false;

  auto it = connections_.begin();
  while (it != connections_.end()) {
    if (it->source_node_id == node_id || it->target_node_id == node_id) {
      const int connection_id = it->id;
      it = connections_.erase(it);
      notify_connection_changed(connection_id);
      removed_any = true;
    } else {
      ++it;
    }
  }

  return removed_any;
}

std::vector<int> NodeGraph::get_input_nodes(int node_id) const {
  std::vector<int> input_nodes;

  for (const auto &conn : connections_) {
    if (conn.target_node_id == node_id) {
      input_nodes.push_back(conn.source_node_id);
    }
  }

  return input_nodes;
}

std::vector<int> NodeGraph::get_output_nodes(int node_id) const {
  std::vector<int> output_nodes;

  for (const auto &conn : connections_) {
    if (conn.source_node_id == node_id) {
      output_nodes.push_back(conn.target_node_id);
    }
  }

  return output_nodes;
}

std::vector<int> NodeGraph::get_execution_order() const {
  std::vector<int> result;
  std::unordered_set<int> visited;
  std::unordered_set<int> temp_visited;

  // Topological sort using DFS
  std::function<bool(int)> visit;
  visit = [&](int node_id) -> bool {
    if (temp_visited.count(node_id)) {
      return false; // Cycle detected
    }
    if (visited.count(node_id)) {
      return true; // Already processed
    }

    temp_visited.insert(node_id);

    // Visit all dependencies first
    const auto input_nodes = get_input_nodes(node_id);
    for (int input_node : input_nodes) {
      if (!visit(input_node)) {
        return false; // Cycle detected
      }
    }

    temp_visited.erase(node_id);
    visited.insert(node_id);
    result.push_back(node_id);
    return true;
  };

  // Visit all nodes
  for (const auto &node : nodes_) {
    if (!visited.count(node->get_id())) {
      if (!visit(node->get_id())) {
        // Cycle detected, return empty result
        return {};
      }
    }
  }

  return result;
}

void NodeGraph::clear() {
  nodes_.clear();
  connections_.clear();
  next_node_id_ = 1;
  next_connection_id_ = 1;
}

bool NodeGraph::is_valid() const { return !has_cycles(); }

bool NodeGraph::has_cycles() const {
  const auto execution_order = get_execution_order();
  return execution_order.empty() && !nodes_.empty();
}

void NodeGraph::notify_node_changed(int node_id) {
  if (node_changed_callback_) {
    node_changed_callback_(node_id);
  }
}

void NodeGraph::notify_connection_changed(int connection_id) {
  if (connection_changed_callback_) {
    connection_changed_callback_(connection_id);
  }
}

std::string NodeGraph::generate_node_name(NodeType type) const {
  constexpr const char *TYPE_NAMES[] = {
      "Sphere",  "Box",     "Cylinder",  "Plane",     "Torus",
      "Extrude", "Smooth",  "Subdivide", "Transform", "Array",
      "Mirror",  "Boolean", "Merge",     "Switch"};

  const int type_index = static_cast<int>(type);
  if (type_index >= 0 && type_index < static_cast<int>(std::size(TYPE_NAMES))) {
    return TYPE_NAMES[type_index];
  }

  return "Unknown";
}

void NodeGraph::set_display_node(int node_id) {
  // Clear display flag from all nodes
  for (auto &node : nodes_) {
    node->set_display_flag(false);
  }

  // Set display flag on the specified node
  auto *node = get_node(node_id);
  if (node != nullptr) {
    node->set_display_flag(true);
    notify_node_changed(node_id);
  }
}

int NodeGraph::get_display_node() const {
  for (const auto &node : nodes_) {
    if (node->has_display_flag()) {
      return node->get_id();
    }
  }
  return -1; // No display node set
}

} // namespace nodeflux::graph
