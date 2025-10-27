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

  // IO
  File,
  Export,

  // Modifiers
  Extrude,
  PolyExtrude,
  Smooth,
  Subdivide,
  Transform,
  Array,
  Mirror,
  Resample,
  NoiseDisplacement,

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
  std::string label;    // Display name for UI
  std::string category; // UI grouping/filtering (optional)

  // Storage for different types
  union {
    float float_value;
    int int_value;
    bool bool_value;
  };
  std::string string_value;
  std::array<float, 3> vector3_value;

  // UI metadata
  struct {
    float float_min = 0.0F;
    float float_max = 100.0F;
    int int_min = 0;
    int int_max = 100;
  } ui_range;

  std::vector<std::string> string_options; // For combo boxes (int type)

  // Category visibility control (optional)
  std::string
      category_control_param; // Name of the parameter that controls visibility
  int category_control_value =
      -1; // Value that makes this parameter visible (-1 = always visible)

  // Constructors for different types
  explicit NodeParameter(const std::string &name, float value,
                         const std::string &label = "", float min = 0.01F,
                         float max = 100.0F, const std::string &cat = "")
      : type(Type::Float), name(name), label(label.empty() ? name : label),
        category(cat), float_value(value) {
    ui_range.float_min = min;
    ui_range.float_max = max;
  }

  explicit NodeParameter(const std::string &name, int value,
                         const std::string &label = "", int min = 0,
                         int max = 100, const std::string &cat = "")
      : type(Type::Int), name(name), label(label.empty() ? name : label),
        category(cat), int_value(value) {
    ui_range.int_min = min;
    ui_range.int_max = max;
  }

  explicit NodeParameter(const std::string &name, bool value,
                         const std::string &label = "",
                         const std::string &cat = "")
      : type(Type::Bool), name(name), label(label.empty() ? name : label),
        category(cat), bool_value(value) {}

  explicit NodeParameter(const std::string &name, const std::string &value,
                         const std::string &label = "",
                         const std::string &cat = "")
      : type(Type::String), name(name), label(label.empty() ? name : label),
        category(cat), string_value(value) {}

  explicit NodeParameter(const std::string &name,
                         const std::array<float, 3> &value,
                         const std::string &label = "", float min = -100.0F,
                         float max = 100.0F, const std::string &cat = "")
      : type(Type::Vector3), name(name), label(label.empty() ? name : label),
        category(cat), vector3_value(value) {
    ui_range.float_min = min;
    ui_range.float_max = max;
  }

  // Constructor for combo box (int with string options)
  explicit NodeParameter(const std::string &name, int value,
                         const std::vector<std::string> &options,
                         const std::string &label = "",
                         const std::string &cat = "")
      : type(Type::Int), name(name), label(label.empty() ? name : label),
        category(cat), int_value(value), string_options(options) {
    ui_range.int_min = 0;
    ui_range.int_max = static_cast<int>(options.size()) - 1;
  }
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

  // Cook time (execution duration in milliseconds)
  double get_cook_time() const { return cook_time_ms_; }
  void set_cook_time(double time_ms) { cook_time_ms_ = time_ms; }

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

  // Performance tracking
  double cook_time_ms_ = 0.0;

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
  int add_node_with_id(int node_id, NodeType type,
                       const std::string &name = ""); // For undo/redo
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

  /**
   * @brief Get all upstream dependencies of a node (backward trace)
   *
   * Returns all nodes that need to be executed before the given node,
   * in topological order (dependencies first, then dependents).
   * This enables selective execution - only cook what's needed!
   *
   * @param node_id The target node
   * @return Vector of node IDs in execution order (inputs first)
   */
  std::vector<int> get_upstream_dependencies(int node_id) const;

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
