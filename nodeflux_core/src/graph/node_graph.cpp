/**
 * NodeFlux Engine - Core Node Graph Implementation
 */

#include "nodeflux/graph/node_graph.hpp"
#include "nodeflux/sop/sop_factory.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <queue>
#include <unordered_set>
#include <variant>

namespace nodeflux::graph {

namespace {

/**
 * @brief Convert SOP ParameterDefinition to GraphNode NodeParameter
 */
NodeParameter
convert_parameter_definition(const sop::SOPNode::ParameterDefinition &def) {
  using ParamType = sop::SOPNode::ParameterDefinition::Type;

  NodeParameter param(def.name, 0.0F); // Temporary, will be overwritten

  switch (def.type) {
  case ParamType::Float: {
    float default_val = std::get<float>(def.default_value);
    param = NodeParameter(def.name, default_val, def.label,
                          static_cast<float>(def.float_min),
                          static_cast<float>(def.float_max), def.category);
    break;
  }

  case ParamType::Int: {
    int default_val = std::get<int>(def.default_value);
    if (!def.options.empty()) {
      // Combo box
      param = NodeParameter(def.name, default_val, def.options, def.label,
                            def.category);
    } else {
      // Regular int
      param = NodeParameter(def.name, default_val, def.label, def.int_min,
                            def.int_max, def.category);
    }
    break;
  }

  case ParamType::Bool: {
    bool default_val = std::get<bool>(def.default_value);
    param = NodeParameter(def.name, default_val, def.label, def.category);
    break;
  }

  case ParamType::String: {
    std::string default_val = std::get<std::string>(def.default_value);
    param = NodeParameter(def.name, default_val, def.label, def.category);
    break;
  }

  case ParamType::Vector3: {
    auto vec3_eigen = std::get<Eigen::Vector3f>(def.default_value);
    std::array<float, 3> vec3_array = {vec3_eigen.x(), vec3_eigen.y(),
                                       vec3_eigen.z()};
    param = NodeParameter(def.name, vec3_array, def.label,
                          static_cast<float>(def.float_min),
                          static_cast<float>(def.float_max), def.category);
    break;
  }
  }

  // Copy visibility control metadata
  param.category_control_param = def.category_control_param;
  param.category_control_value = def.category_control_value;

  return param;
}

/**
 * @brief Initialize GraphNode parameters from SOPNode parameter definitions
 */
void initialize_node_parameters_from_sop(GraphNode &node) {
  // Create a temporary SOP instance to get its parameter definitions
  auto sop = sop::SOPFactory::create(node.get_type(), node.get_name());

  if (!sop) {
    // Node type doesn't have a SOP implementation (e.g., Switch)
    // Parameters will be set up by setup_pins_for_type() in the constructor
    return;
  }

  // Get parameter definitions from the SOP
  const auto &param_defs = sop->get_parameter_definitions();

  if (param_defs.empty()) {
    // No registered parameters, will use hardcoded ones from
    // setup_pins_for_type()
    return;
  }

  // Convert and add parameters to GraphNode
  for (const auto &def : param_defs) {
    NodeParameter param = convert_parameter_definition(def);
    node.add_parameter(param);
  }
}

} // anonymous namespace

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
    parameters_.emplace_back("radius", 1.0F, "Radius", 0.01F, 100.0F);
    parameters_.emplace_back("u_segments", 32, "U Segments", 3, 128);
    parameters_.emplace_back("v_segments", 16, "V Segments", 2, 64);
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;

  case NodeType::Box:
    parameters_.emplace_back("width", 1.0F, "Width", 0.01F, 100.0F);
    parameters_.emplace_back("height", 1.0F, "Height", 0.01F, 100.0F);
    parameters_.emplace_back("depth", 1.0F, "Depth", 0.01F, 100.0F);
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;

  case NodeType::File:
    parameters_.emplace_back("file_path", std::string(""), "File Path");
    parameters_.emplace_back("reload", false, "Reload");
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;

  case NodeType::Export:
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Mesh", 0});
    parameters_.emplace_back("file_path", std::string(""), "Export Path");
    parameters_.emplace_back("export_now", false, "Export");
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;

  case NodeType::Cylinder:
    parameters_.emplace_back("radius", 1.0F, "Radius", 0.01F, 100.0F);
    parameters_.emplace_back("height", 2.0F, "Height", 0.01F, 100.0F);
    parameters_.emplace_back("segments", 32, "Segments", 3, 128);
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;

  case NodeType::Extrude: {
    // Query parameter schema from SOP (single source of truth!)
    auto param_defs = sop::SOPFactory::get_parameter_schema(NodeType::Extrude);
    for (const auto &def : param_defs) {
      parameters_.push_back(convert_parameter_definition(def));
    }
    // Setup pins
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input", 0});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;
  }

  case NodeType::PolyExtrude:
    parameters_.emplace_back("distance", 1.0F, "Distance", 0.0F, 10.0F);
    parameters_.emplace_back("inset", 0.0F, "Inset", 0.0F, 10.0F);
    parameters_.emplace_back("individual_faces", true, "Individual Faces");
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input", 0});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;

  case NodeType::Smooth: {
    // Query parameter schema from SOP (single source of truth!)
    auto param_defs = sop::SOPFactory::get_parameter_schema(NodeType::Smooth);
    for (const auto &def : param_defs) {
      parameters_.push_back(convert_parameter_definition(def));
    }
    // Setup pins
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input", 0});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;
  }

  case NodeType::Mirror: {
    // Query parameter schema from SOP (single source of truth!)
    auto param_defs = sop::SOPFactory::get_parameter_schema(NodeType::Mirror);
    for (const auto &def : param_defs) {
      parameters_.push_back(convert_parameter_definition(def));
    }
    // Setup pins
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input", 0});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;
  }

  case NodeType::Subdivide: {
    // Query parameter schema from SOP (single source of truth!)
    auto param_defs =
        sop::SOPFactory::get_parameter_schema(NodeType::Subdivide);
    for (const auto &def : param_defs) {
      parameters_.push_back(convert_parameter_definition(def));
    }
    // Setup pins
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input", 0});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Mesh", 0});
    break;
  }

  case NodeType::Boolean:
    parameters_.emplace_back(
        "operation", 0,
        std::vector<std::string>{"Union", "Intersection", "Difference"},
        "Operation");
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "A", 0});
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "B", 1});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Result", 0});
    break;

  case NodeType::Transform: {
    // Query parameter schema from SOP (single source of truth!)
    auto param_defs =
        sop::SOPFactory::get_parameter_schema(NodeType::Transform);
    for (const auto &def : param_defs) {
      parameters_.push_back(convert_parameter_definition(def));
    }
    // Setup pins
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input", 0});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Output", 0});
    break;
  }

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
    // Array node - parameters defined in ArraySOP, loaded via
    // initialize_node_parameters_from_sop()
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input", 0});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Array", 0});
    break;

  case NodeType::Scatter:
    // Scatter points on mesh surface
    parameters_.emplace_back("point_count", 100);
    parameters_.emplace_back("seed", 42);
    parameters_.emplace_back("density", 1.0F);
    parameters_.emplace_back("use_face_area", 1); // 1=true, 0=false
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input", 0});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Points", 0});
    break;

  case NodeType::NoiseDisplacement:
    // Apply procedural noise displacement to mesh vertices
    parameters_.emplace_back("amplitude", 0.1F, "Amplitude", 0.0F, 10.0F);
    parameters_.emplace_back("frequency", 1.0F, "Frequency", 0.01F, 10.0F);
    parameters_.emplace_back("octaves", 4, "Octaves", 1, 8);
    parameters_.emplace_back("lacunarity", 2.0F, "Lacunarity", 1.0F, 4.0F);
    parameters_.emplace_back("persistence", 0.5F, "Persistence", 0.0F, 1.0F);
    parameters_.emplace_back("seed", 42, "Seed", 0, 10000);
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Input", 0});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Output", 0});
    break;

  case NodeType::CopyToPoints:
    // Copy template geometry to each point - parameters defined in
    // CopyToPointsSOP
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Points", 0});
    input_pins_.push_back(
        {NodePin::Type::Input, NodePin::DataType::Mesh, "Template", 1});
    output_pins_.push_back(
        {NodePin::Type::Output, NodePin::DataType::Mesh, "Copied", 0});
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

  // Initialize parameters from SOPNode for modern SOPs
  initialize_node_parameters_from_sop(*node);

  nodes_.push_back(std::move(node));

  notify_node_changed(node_id);
  return node_id;
}

int NodeGraph::add_node_with_id(int node_id, NodeType type,
                                const std::string &name) {
  // For undo/redo: add node with a specific ID
  const std::string node_name = name.empty() ? generate_node_name(type) : name;

  auto node = std::make_unique<GraphNode>(node_id, type, node_name);

  // Initialize parameters from SOPNode for modern SOPs
  initialize_node_parameters_from_sop(*node);

  nodes_.push_back(std::move(node));
  nodes_.push_back(std::move(node));

  // Update next_node_id if necessary to avoid ID conflicts
  if (node_id >= next_node_id_) {
    next_node_id_ = node_id + 1;
  }

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

std::vector<int> NodeGraph::get_upstream_dependencies(int node_id) const {
  // Verify node exists
  const auto *target_node = get_node(node_id);
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
  // Use switch statement to keep names in sync with enum
  // Compiler will warn if we miss a case with -Wswitch-enum
  switch (type) {
  // Generators
  case NodeType::Sphere:
    return "Sphere";
  case NodeType::Box:
    return "Box";
  case NodeType::Cylinder:
    return "Cylinder";
  case NodeType::Plane:
    return "Plane";
  case NodeType::Torus:
    return "Torus";
  case NodeType::Line:
    return "Line";

  // IO
  case NodeType::File:
    return "File";
  case NodeType::Export:
    return "Export";

  // Modifiers
  case NodeType::Extrude:
    return "Extrude";
  case NodeType::PolyExtrude:
    return "PolyExtrude";
  case NodeType::Smooth:
    return "Smooth";
  case NodeType::Subdivide:
    return "Subdivide";
  case NodeType::Transform:
    return "Transform";
  case NodeType::Array:
    return "Array";
  case NodeType::Mirror:
    return "Mirror";
  case NodeType::Resample:
    return "Resample";
  case NodeType::NoiseDisplacement:
    return "NoiseDisplacement";

  // Boolean Operations
  case NodeType::Boolean:
    return "Boolean";

  // Point Operations
  case NodeType::Scatter:
    return "Scatter";
  case NodeType::CopyToPoints:
    return "CopyToPoints";

  // Utilities
  case NodeType::Merge:
    return "Merge";
  case NodeType::Switch:
    return "Switch";
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
