/**
 * NodeFlux Engine - Core Node Graph Data Model
 * Pure data representation with serialization support
 */

#pragma once

#include "nodeflux/core/mesh.hpp"
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace nodeflux::graph {

/**
 * @brief Node types supported by the system
 */
enum class NodeType {
  // Generators
  Sphere,
  Box,
  Cylinder,
  Plane,
  Torus,
  Line,

  // Modifiers
  Extrude,
  PolyExtrude,
  Smooth,
  Subdivide,
  Transform,
  Array,
  Mirror,
  Resample,

  // Boolean Operations
  Boolean,

  // Point Operations
  Scatter,
  CopyToPoints,

  // Utilities
  Merge,
  Switch
};

/**
 * @brief Parameter value that can hold different types
 */
struct NodeParameter {
  enum class Type { Float, Int, Bool, Vector3, String };

  Type type;
  std::string name;

  // Storage for different types
  union {
    float float_value;
    int int_value;
    bool bool_value;
  };
  std::string string_value;
  std::array<float, 3> vector3_value;

  // Constructors for different types
  explicit NodeParameter(const std::string &name, float value)
      : type(Type::Float), name(name), float_value(value) {}
  explicit NodeParameter(const std::string &name, int value)
      : type(Type::Int), name(name), int_value(value) {}
  explicit NodeParameter(const std::string &name, bool value)
      : type(Type::Bool), name(name), bool_value(value) {}
  explicit NodeParameter(const std::string &name, const std::string &value)
      : type(Type::String), name(name), string_value(value) {}
  explicit NodeParameter(const std::string &name,
                         const std::array<float, 3> &value)
      : type(Type::Vector3), name(name), vector3_value(value) {}
};

/**
 * @brief Connection between two node pins
 */
struct NodeConnection {
  int id;
  int source_node_id;
  int source_pin_index;
  int target_node_id;
  int target_pin_index;

  bool operator==(const NodeConnection &other) const { return id == other.id; }
};

/**
 * @brief Pin definition for a node
 */
struct NodePin {
  enum class Type { Input, Output };
  enum class DataType { Mesh, Float, Int, Bool, Vector3 };

  Type type;
  DataType data_type;
  std::string name;
  int index;
  bool required = true;
};

/**
 * @brief Node in the graph - pure data, no UI coupling
 */
class GraphNode {
public:
  GraphNode(int id, NodeType type, const std::string &name);

  // Basic properties
  int get_id() const { return id_; }
  NodeType get_type() const { return type_; }
  const std::string &get_name() const { return name_; }
  void set_name(const std::string &name) { name_ = name; }

  // Position (for UI layout)
  std::pair<float, float> get_position() const { return {x_, y_}; }
  void set_position(float x, float y) {
    x_ = x;
    y_ = y;
  }

  // Parameters
  void add_parameter(const NodeParameter &param);
  std::optional<NodeParameter> get_parameter(const std::string &name) const;
  void set_parameter(const std::string &name, const NodeParameter &param);
  const std::vector<NodeParameter> &get_parameters() const {
    return parameters_;
  }

  // Pins
  const std::vector<NodePin> &get_input_pins() const { return input_pins_; }
  const std::vector<NodePin> &get_output_pins() const { return output_pins_; }

  // State
  bool needs_update() const { return needs_update_; }
  void mark_for_update() { needs_update_ = true; }
  void mark_updated() { needs_update_ = false; }

  // Flags (Houdini-style)
  bool has_display_flag() const { return display_flag_; }
  void set_display_flag(bool flag) { display_flag_ = flag; }

  bool has_render_flag() const { return render_flag_; }
  void set_render_flag(bool flag) { render_flag_ = flag; }

  bool is_bypassed() const { return bypass_flag_; }
  void set_bypass(bool bypass) { bypass_flag_ = bypass; }

  // Error state
  bool has_error() const { return has_error_; }
  void set_error(bool error, const std::string &message = "") {
    has_error_ = error;
    error_message_ = message;
  }
  const std::string &get_error_message() const { return error_message_; }

  // Result cache
  void set_output_mesh(std::shared_ptr<core::Mesh> mesh) {
    output_mesh_ = mesh;
  }
  std::shared_ptr<core::Mesh> get_output_mesh() const { return output_mesh_; }

private:
  int id_;
  NodeType type_;
  std::string name_;
  float x_ = 0.0F;
  float y_ = 0.0F;

  std::vector<NodeParameter> parameters_;
  std::vector<NodePin> input_pins_;
  std::vector<NodePin> output_pins_;

  bool needs_update_ = true;
  std::shared_ptr<core::Mesh> output_mesh_;

  // Houdini-style flags
  bool display_flag_ = false;
  bool render_flag_ = false;
  bool bypass_flag_ = false;

  // Error state
  bool has_error_ = false;
  std::string error_message_;

  void setup_pins_for_type();
};

/**
 * @brief Main node graph data structure
 */
class NodeGraph {
public:
  NodeGraph() = default;
  ~NodeGraph() = default;

  // Non-copyable but movable
  NodeGraph(const NodeGraph &) = delete;
  NodeGraph &operator=(const NodeGraph &) = delete;
  NodeGraph(NodeGraph &&) = default;
  NodeGraph &operator=(NodeGraph &&) = default;

  // Node management
  int add_node(NodeType type, const std::string &name = "");
  bool remove_node(int node_id);
  GraphNode *get_node(int node_id);
  const GraphNode *get_node(int node_id) const;
  const std::vector<std::unique_ptr<GraphNode>> &get_nodes() const {
    return nodes_;
  }

  // Connection management
  int add_connection(int source_node_id, int source_pin, int target_node_id,
                     int target_pin);
  bool remove_connection(int connection_id);
  bool remove_connections_to_node(int node_id);
  const std::vector<NodeConnection> &get_connections() const {
    return connections_;
  }

  // Graph queries
  std::vector<int> get_input_nodes(int node_id) const;
  std::vector<int> get_output_nodes(int node_id) const;
  std::vector<int> get_execution_order() const; // Topological sort

  // Graph operations
  void clear();
  bool is_valid() const;
  bool has_cycles() const;

  // Display flag management (only one node can have display flag)
  void set_display_node(int node_id);
  int get_display_node() const;

  // Events
  using NodeChangedCallback = std::function<void(int node_id)>;
  using ConnectionChangedCallback = std::function<void(int connection_id)>;

  void set_node_changed_callback(NodeChangedCallback callback) {
    node_changed_callback_ = callback;
  }
  void set_connection_changed_callback(ConnectionChangedCallback callback) {
    connection_changed_callback_ = callback;
  }

private:
  std::vector<std::unique_ptr<GraphNode>> nodes_;
  std::vector<NodeConnection> connections_;
  int next_node_id_ = 1;
  int next_connection_id_ = 1;

  // Event callbacks
  NodeChangedCallback node_changed_callback_;
  ConnectionChangedCallback connection_changed_callback_;

  // Helper methods
  void notify_node_changed(int node_id);
  void notify_connection_changed(int connection_id);
  std::string generate_node_name(NodeType type) const;
};

} // namespace nodeflux::graph
