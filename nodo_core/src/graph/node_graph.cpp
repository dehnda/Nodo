/**
 * Nodo - Core Node Graph Implementation
 */

#include "nodo/graph/node_graph.hpp"

#include "nodo/sop/sop_factory.hpp"

#include <queue>

#include <algorithm>
#include <functional>
#include <iostream>
#include <unordered_set>
#include <variant>

namespace nodo::graph {

std::string get_node_type_name(NodeType type) {
  return sop::SOPFactory::get_display_name(type);
}

namespace {

std::string generate_name_from_type(NodeType type) {
  return get_node_type_name(type);
}

} // anonymous namespace

GraphNode::GraphNode(int node_id, NodeType type) : id_(node_id), type_(type) {
  setup_pins_for_type();

  // Create SOP immediately (no longer lazy) so parameters are available
  sop_instance_ = sop::SOPFactory::create(type_, generate_name_from_type(type_));
}

void GraphNode::setup_pins_for_type() {
  input_pins_.clear();
  output_pins_.clear();

  // Query the SOP for its input configuration
  auto config = sop::SOPFactory::get_input_config(type_);

  // Create input pins based on configuration type
  switch (config.type) {
    case sop::SOPNode::InputType::NONE:
      // Generator node - no inputs
      break;

    case sop::SOPNode::InputType::SINGLE:
      // Standard single input node (Transform, Subdivide, etc.)
      input_pins_.push_back({NodePin::Type::Input, NodePin::DataType::Mesh, "Input", 0});
      break;

    case sop::SOPNode::InputType::DUAL:
      // Dual input node (Boolean, CopyToPoints, etc.)
      input_pins_.push_back({NodePin::Type::Input, NodePin::DataType::Mesh, "Input A", 0});
      input_pins_.push_back({NodePin::Type::Input, NodePin::DataType::Mesh, "Input B", 1});
      break;

    case sop::SOPNode::InputType::MULTI_DYNAMIC:
      // Dynamic multi-input node (Merge, etc.)
      // Single wide input pin that accepts multiple connections
      input_pins_.push_back({NodePin::Type::Input, NodePin::DataType::Mesh, "Inputs", 0});
      break;

    case sop::SOPNode::InputType::MULTI_FIXED:
      // Fixed multi-input node (Switch, etc.)
      // Multiple separate pins, start with initial_pins
      for (int i = 0; i < config.initial_pins; ++i) {
        input_pins_.push_back({NodePin::Type::Input, NodePin::DataType::Mesh, "Input " + std::to_string(i + 1), i});
      }
      break;
  }

  // All nodes have one output (for now)
  output_pins_.push_back({NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
}

const sop::SOPNode::ParameterMap& GraphNode::get_parameters() const {
  static const sop::SOPNode::ParameterMap empty_map;
  return sop_instance_ ? sop_instance_->get_parameters() : empty_map;
}

const std::vector<sop::SOPNode::ParameterDefinition>& GraphNode::get_parameter_definitions() const {
  static const std::vector<sop::SOPNode::ParameterDefinition> empty_vec;
  return sop_instance_ ? sop_instance_->get_parameter_definitions() : empty_vec;
}

int NodeGraph::add_node(NodeType type, const std::string& name) {
  int id = next_node_id_++;
  return add_node_with_id(id, type, name);
}

int NodeGraph::add_node_with_id(int node_id, NodeType type, const std::string& name) {
  // Use provided name or generate one from the type
  std::string base_name = name.empty() ? get_node_type_name(type) : name;

  // Always ensure the name is unique
  std::string final_name = generate_unique_node_name(base_name);

  auto node = std::make_unique<GraphNode>(node_id, type);
  node->set_name(final_name);

  nodes_.push_back(std::move(node));
  notify_node_changed(node_id);

  return node_id;
}

bool NodeGraph::remove_node(int node_id) {
  // Remove all connections to/from this node
  remove_connections_to_node(node_id);

  // Remove the node
  auto it = std::find_if(nodes_.begin(), nodes_.end(),
                         [node_id](const std::unique_ptr<GraphNode>& node) { return node->get_id() == node_id; });

  if (it != nodes_.end()) {
    nodes_.erase(it);
    notify_node_changed(node_id);
    return true;
  }

  return false;
}

GraphNode* NodeGraph::get_node(int node_id) {
  auto it = std::find_if(nodes_.begin(), nodes_.end(),
                         [node_id](const std::unique_ptr<GraphNode>& node) { return node->get_id() == node_id; });

  return (it != nodes_.end()) ? it->get() : nullptr;
}

const GraphNode* NodeGraph::get_node(int node_id) const {
  auto it = std::find_if(nodes_.begin(), nodes_.end(),
                         [node_id](const std::unique_ptr<GraphNode>& node) { return node->get_id() == node_id; });

  return (it != nodes_.end()) ? it->get() : nullptr;
}

int NodeGraph::add_connection(int source_node_id, int source_pin, int target_node_id, int target_pin) {
  // Validate nodes exist
  if (!get_node(source_node_id) || !get_node(target_node_id)) {
    return -1;
  }

  // Get target node's input configuration
  auto* target_node = get_node(target_node_id);
  if (!target_node) {
    return -1;
  }

  auto config = sop::SOPFactory::get_input_config(target_node->get_type());

  // Check for existing connection to target pin
  auto existing =
      std::find_if(connections_.begin(), connections_.end(), [target_node_id, target_pin](const NodeConnection& conn) {
        return conn.target_node_id == target_node_id && conn.target_pin_index == target_pin;
      });

  if (existing != connections_.end()) {
    // Behavior depends on input type:
    // - NONE: Should never happen (no inputs)
    // - SINGLE, DUAL, MULTI_FIXED: Replace existing connection (one per pin)
    // - MULTI_DYNAMIC: Allow multiple connections to same pin (don't replace)
    if (config.type != sop::SOPNode::InputType::MULTI_DYNAMIC) {
      // For single/dual/fixed inputs, remove existing connection (only one per
      // pin)
      remove_connection(existing->id);
    }
    // For MULTI_DYNAMIC, we allow multiple connections to the same pin
    // so we don't remove the existing connection
  }

  // Add new connection
  const int connection_id = next_connection_id_++;
  connections_.push_back({connection_id, source_node_id, source_pin, target_node_id, target_pin});

  // Mark target node for update
  if (target_node) {
    target_node->mark_for_update();
  }

  notify_connection_changed(connection_id);
  return connection_id;
}

bool NodeGraph::remove_connection(int connection_id) {
  auto it = std::find_if(connections_.begin(), connections_.end(),
                         [connection_id](const NodeConnection& conn) { return conn.id == connection_id; });

  if (it != connections_.end()) {
    // Mark target node for update
    if (auto* target_node = get_node(it->target_node_id)) {
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

  for (const auto& conn : connections_) {
    if (conn.target_node_id == node_id) {
      input_nodes.push_back(conn.source_node_id);
    }
  }

  return input_nodes;
}

std::vector<int> NodeGraph::get_output_nodes(int node_id) const {
  std::vector<int> output_nodes;

  for (const auto& conn : connections_) {
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
  for (const auto& node : nodes_) {
    if (!visited.count(node->get_id())) {
      if (!visit(node->get_id())) {
        // Cycle detected, return empty result
        return {};
      }
    }
  }

  return result;
}

std::vector<int> NodeGraph::get_upstream_dependencies(int node_id) const {
  // Verify node exists
  const auto* target_node = get_node(node_id);
  if (target_node == nullptr) {
    return {}; // Node doesn't exist
  }

  // Use depth-first search to collect all upstream nodes
  std::unordered_set<int> visited;
  std::vector<int> dependencies;

  // Recursive DFS function
  std::function<void(int)> collect_dependencies = [&](int current_id) {
    // Skip if already visited
    if (visited.count(current_id) > 0) {
      return;
    }

    visited.insert(current_id);

    // Get all input nodes
    const auto input_nodes = get_input_nodes(current_id);

    // Recursively visit all inputs first (depth-first)
    for (int input_id : input_nodes) {
      collect_dependencies(input_id);
    }

    // Add current node after its dependencies
    dependencies.push_back(current_id);
  };

  // Start DFS from the target node
  collect_dependencies(node_id);

  // Result is already in topological order (dependencies first)
  return dependencies;
}

void NodeGraph::clear() {
  nodes_.clear();
  connections_.clear();
  next_node_id_ = 1;
  next_connection_id_ = 1;
}

bool NodeGraph::is_valid() const {
  return !has_cycles();
}

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
  // Get display name from SOP metadata - single source of truth
  return sop::SOPFactory::get_display_name(type);
}

std::string NodeGraph::generate_unique_node_name(const std::string& base_name) const {
  // Check if base name is already unique
  bool name_exists = false;
  for (const auto& node : nodes_) {
    if (node->get_name() == base_name) {
      name_exists = true;
      break;
    }
  }

  if (!name_exists) {
    return base_name;
  }

  // Find the next available number suffix
  // Try: base_name1, base_name2, base_name3, etc.
  int suffix = 1;
  while (true) {
    std::string candidate = base_name + std::to_string(suffix);

    bool exists = false;
    for (const auto& node : nodes_) {
      if (node->get_name() == candidate) {
        exists = true;
        break;
      }
    }

    if (!exists) {
      return candidate;
    }

    suffix++;
  }
}

void NodeGraph::set_display_node(int node_id) {
  // Clear display flag from all nodes
  for (auto& node : nodes_) {
    node->set_display_flag(false);
  }

  // Set display flag on the specified node
  auto* node = get_node(node_id);
  if (node != nullptr) {
    node->set_display_flag(true);
    notify_node_changed(node_id);
  }
}

int NodeGraph::get_display_node() const {
  for (const auto& node : nodes_) {
    if (node->has_display_flag()) {
      return node->get_id();
    }
  }
  return -1; // No display node set
}

// Graph Parameters (M3.2)

bool NodeGraph::add_graph_parameter(const GraphParameter& parameter) {
  const std::string& name = parameter.get_name();

  // Validate parameter name (future-proof for subgraphs)
  if (!is_valid_parameter_name(name)) {
    std::cerr << "Invalid parameter name: '" << name << "' (dots and reserved words not allowed)\n";
    return false;
  }

  // Check if parameter with this name already exists
  auto it = std::find_if(graph_parameters_.begin(), graph_parameters_.end(),
                         [&name](const GraphParameter& p) { return p.get_name() == name; });

  if (it != graph_parameters_.end()) {
    // Update existing parameter
    *it = parameter;
  } else {
    // Add new parameter
    graph_parameters_.push_back(parameter);
  }

  return true;
}

bool NodeGraph::remove_graph_parameter(const std::string& name) {
  auto it = std::find_if(graph_parameters_.begin(), graph_parameters_.end(),
                         [&name](const GraphParameter& p) { return p.get_name() == name; });

  if (it != graph_parameters_.end()) {
    graph_parameters_.erase(it);
    return true;
  }

  return false;
}

const GraphParameter* NodeGraph::get_graph_parameter(const std::string& name) const {
  auto it = std::find_if(graph_parameters_.begin(), graph_parameters_.end(),
                         [&name](const GraphParameter& p) { return p.get_name() == name; });

  if (it != graph_parameters_.end()) {
    return &(*it);
  }

  return nullptr;
}

GraphParameter* NodeGraph::get_graph_parameter(const std::string& name) {
  auto it = std::find_if(graph_parameters_.begin(), graph_parameters_.end(),
                         [&name](const GraphParameter& p) { return p.get_name() == name; });

  if (it != graph_parameters_.end()) {
    return &(*it);
  }

  return nullptr;
}

bool NodeGraph::has_graph_parameter(const std::string& name) const {
  return get_graph_parameter(name) != nullptr;
}

bool NodeGraph::is_valid_parameter_name(const std::string& name) {
  // Empty names not allowed
  if (name.empty()) {
    return false;
  }

  // Check for reserved characters (dot reserved for future scoping:
  // $parent.param)
  if (name.find('.') != std::string::npos) {
    return false;
  }

  // Reserved words (for future subgraph scoping)
  if (name == "parent" || name == "root" || name == "this") {
    return false;
  }

  // Must start with letter or underscore
  if (!std::isalpha(name[0]) && name[0] != '_') {
    return false;
  }

  // Rest can be alphanumeric or underscore
  for (char character : name) {
    if (!std::isalnum(character) && character != '_') {
      return false;
    }
  }

  return true;
}

std::optional<std::string> NodeGraph::resolve_parameter_path(int current_node_id, const std::string& path) const {
  // Parse the path into node name and parameter name
  // Supported formats:
  // - "/NodeName/param" (absolute)
  // - "../NodeName/param" (relative - sibling)
  // - "NodeName/param" (relative - sibling, same as ../)

  std::string node_name;
  std::string param_name;

  // Find the last slash to split node/param
  size_t last_slash = path.rfind('/');
  if (last_slash == std::string::npos) {
    // No slash - invalid path for ch()
    return std::nullopt;
  }

  param_name = path.substr(last_slash + 1);
  std::string node_path = path.substr(0, last_slash);

  // Handle absolute vs relative paths
  if (!node_path.empty() && node_path[0] == '/') {
    // Absolute path: "/NodeName"
    node_name = node_path.substr(1); // Remove leading slash
  } else {
    // Relative path: "../NodeName" or "NodeName"
    // Remove "../" prefix if present
    if (node_path.size() >= 3 && node_path.substr(0, 3) == "../") {
      node_name = node_path.substr(3);
    } else {
      node_name = node_path;
    }
  }

  // Find the target node by name
  const GraphNode* target_node = nullptr;
  for (const auto& node : nodes_) {
    if (node->get_name() == node_name) {
      target_node = node.get();
      break;
    }
  }

  if (target_node == nullptr) {
    // Node not found
    return std::nullopt;
  }

  // Get the parameter from the target node's SOP
  const auto* sop = target_node->get_sop();
  if (!sop) {
    return std::nullopt;
  }

  const auto& param_values = sop->get_parameters();
  auto param_it = param_values.find(param_name);
  if (param_it == param_values.end()) {
    // Parameter not found
    return std::nullopt;
  }

  // Convert parameter value to string based on variant type
  const auto& value = param_it->second;

  if (std::holds_alternative<float>(value)) {
    return std::to_string(std::get<float>(value));
  } else if (std::holds_alternative<int>(value)) {
    return std::to_string(std::get<int>(value));
  } else if (std::holds_alternative<bool>(value)) {
    return std::get<bool>(value) ? "1" : "0";
  } else if (std::holds_alternative<std::string>(value)) {
    return std::get<std::string>(value);
  } else if (std::holds_alternative<Eigen::Vector3f>(value)) {
    const auto& vec = std::get<Eigen::Vector3f>(value);
    return std::to_string(vec.x()) + "," + std::to_string(vec.y()) + "," + std::to_string(vec.z());
  }

  return std::nullopt;
}

// ===== Persistent SOPNode Implementation =====

sop::SOPNode* GraphNode::get_sop() {
  return sop_instance_.get();
}

const sop::SOPNode* GraphNode::get_sop() const {
  return sop_instance_.get();
}

} // namespace nodo::graph
