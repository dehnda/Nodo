#pragma once

#include "node_port.hpp"
#include "nodo/core/attribute_group.hpp"
#include "nodo/core/geometry_container.hpp"
#include <Eigen/Dense>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace nodo {

// Forward declaration
namespace graph {
class ExecutionEngine;
}

namespace sop {

/**
 * @brief Base class for all Surface Operator (SOP) nodes
 *
 * SOPNode provides the foundation for the procedural mesh generation system,
 * implementing caching, dependency tracking, and execution management.
 */
class SOPNode {
  friend class graph::ExecutionEngine; // Allow bridge access

public:
  enum class ExecutionState {
    CLEAN,     // Node output is up-to-date
    DIRTY,     // Node needs to be recomputed
    COMPUTING, // Node is currently being computed
    ERROR      // Node computation failed
  };

  // Parameter types for node configuration
  using ParameterValue =
      std::variant<int, float, double, bool, std::string, Eigen::Vector3f>;
  using ParameterMap = std::unordered_map<std::string, ParameterValue>;

  /**
   * @brief Parameter definition with UI metadata (schema)
   */
  struct ParameterDefinition {
    enum class Type { Float, Int, Bool, String, Vector3 };

    std::string name;             // Internal identifier
    std::string label;            // UI display name
    std::string category;         // UI grouping (optional)
    std::string description;      // Tooltip/help text (optional)
    Type type;                    // Data type
    ParameterValue default_value; // Default value

    // UI hints
    double float_min = 0.0;
    double float_max = 100.0;
    int int_min = 0;
    int int_max = 100;
    std::vector<std::string> options; // For combo boxes (int type)

    // Category visibility control (optional)
    // If set, this parameter's category is only visible when the control
    // parameter has the matching value
    std::string category_control_param; // Name of the parameter that controls
                                        // visibility
    int category_control_value =
        -1; // Value that makes this category visible (-1 = always visible)

    ParameterDefinition(const std::string &n, Type t, ParameterValue def)
        : name(n), label(n), type(t), default_value(def) {}
  };

  /**
   * @brief Fluent builder for parameter definitions
   */
  class ParameterBuilder {
  public:
    ParameterBuilder(ParameterDefinition def) : def_(std::move(def)) {}

    ParameterBuilder &label(const std::string &lbl) {
      def_.label = lbl;
      return *this;
    }

    ParameterBuilder &category(const std::string &cat) {
      def_.category = cat;
      return *this;
    }

    ParameterBuilder &description(const std::string &desc) {
      def_.description = desc;
      return *this;
    }

    ParameterBuilder &range(double min, double max) {
      def_.float_min = min;
      def_.float_max = max;
      return *this;
    }

    ParameterBuilder &range(int min, int max) {
      def_.int_min = min;
      def_.int_max = max;
      return *this;
    }

    ParameterBuilder &options(const std::vector<std::string> &opts) {
      def_.options = opts;
      def_.int_min = 0;
      def_.int_max = static_cast<int>(opts.size()) - 1;
      return *this;
    }

    ParameterBuilder &visible_when(const std::string &control_param,
                                   int value) {
      def_.category_control_param = control_param;
      def_.category_control_value = value;
      return *this;
    }

    ParameterDefinition build() const { return def_; }

  private:
    ParameterDefinition def_;
  };

private:
  std::string node_name_;
  std::string node_type_;
  ExecutionState state_ = ExecutionState::DIRTY;

  // Error information
  std::string last_error_;

  // Timing information
  std::chrono::steady_clock::time_point last_cook_time_;
  std::chrono::milliseconds cook_duration_{0};

  // Node parameters
  ParameterMap parameters_;

  // Parameter schema (definitions with metadata)
  std::vector<ParameterDefinition> parameter_definitions_;

protected:
  // Port management
  PortCollection input_ports_;
  PortCollection output_ports_;

  // Main output port (most nodes have one primary output)
  NodePort *main_output_ = nullptr;

public:
  SOPNode(std::string node_name, std::string node_type)
      : node_name_(std::move(node_name)), node_type_(std::move(node_type)) {

    // Create default geometry output port
    main_output_ = output_ports_.add_port("geometry", NodePort::Type::OUTPUT,
                                          NodePort::DataType::GEOMETRY, this);

    // Add universal group parameter (all SOPs inherit this)
    register_parameter(
        define_string_parameter("group", "")
            .label("Group")
            .category("Group")
            .description("Name of group to operate on (empty = all elements)")
            .build());
  }

  virtual ~SOPNode() = default;

  // Non-copyable but movable
  SOPNode(const SOPNode &) = delete;
  SOPNode &operator=(const SOPNode &) = delete;
  SOPNode(SOPNode &&) = default;
  SOPNode &operator=(SOPNode &&) = default;

  /**
   * @brief Get node name
   */
  const std::string &get_name() const { return node_name_; }

  /**
   * @brief Get node type
   */
  const std::string &get_type() const { return node_type_; }

  /**
   * @brief Get current execution state
   */
  ExecutionState get_state() const { return state_; }

  /**
   * @brief Get last error message
   */
  const std::string &get_last_error() const { return last_error_; }

  /**
   * @brief Get last cook duration
   */
  std::chrono::milliseconds get_cook_duration() const { return cook_duration_; }

  /**
   * @brief Get input ports
   */
  PortCollection &get_input_ports() { return input_ports_; }
  const PortCollection &get_input_ports() const { return input_ports_; }

  /**
   * @brief Get output ports
   */
  PortCollection &get_output_ports() { return output_ports_; }
  const PortCollection &get_output_ports() const { return output_ports_; }

  /**
   * @brief Get main output port
   */
  NodePort *get_main_output() const { return main_output_; }

  /**
   * @brief Set parameter value
   */
  template <typename T> void set_parameter(const std::string &name, T value) {
    parameters_[name] = value;
    mark_dirty();
  }

  /**
   * @brief Get parameter value
   */
  template <typename T>
  T get_parameter(const std::string &name, T default_value = T{}) const {
    auto param_it = parameters_.find(name);
    if (param_it != parameters_.end()) {
      if (std::holds_alternative<T>(param_it->second)) {
        return std::get<T>(param_it->second);
      }
    }
    return default_value;
  }

  /**
   * @brief Check if parameter exists
   */
  bool has_parameter(const std::string &name) const {
    return parameters_.find(name) != parameters_.end();
  }

  /**
   * @brief Get all parameter definitions (schema)
   */
  const std::vector<ParameterDefinition> &get_parameter_definitions() const {
    return parameter_definitions_;
  }

  /**
   * @brief Get parameter map (current values)
   */
  const ParameterMap &get_parameters() const { return parameters_; }

  /**
   * @brief Mark node as dirty (needs recomputation)
   */
  void mark_dirty() {
    if (state_ == ExecutionState::CLEAN) {
      state_ = ExecutionState::DIRTY;

      // Invalidate output port caches
      for (const auto &port : output_ports_.get_all_ports()) {
        port->invalidate_cache();
      }
    }
  }

  /**
   * @brief Cook (execute) this node
   *
   * This is the main execution entry point. It handles caching,
   * dependency resolution, and error handling.
   */
  std::shared_ptr<core::GeometryContainer> cook() {
    // Return cached result if clean
    if (state_ == ExecutionState::CLEAN && main_output_->is_cache_valid()) {
      return main_output_->get_data();
    }

    // Prevent recursive cooking
    if (state_ == ExecutionState::COMPUTING) {
      last_error_ = "Circular dependency detected in node: " + node_name_;
      state_ = ExecutionState::ERROR;
      return nullptr;
    }

    auto cook_start = std::chrono::steady_clock::now();
    state_ = ExecutionState::COMPUTING;
    last_error_.clear();

    try {
      // Cook input dependencies first
      cook_inputs();

      // Execute the node-specific computation
      auto result = execute();

      // Update timing and state
      cook_duration_ = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - cook_start);
      last_cook_time_ = cook_start;

      if (result) {
        main_output_->set_data(result);
        state_ = ExecutionState::CLEAN;
      } else {
        state_ = ExecutionState::ERROR;
        if (last_error_.empty()) {
          last_error_ = "Node execution returned null result";
        }
      }

      return result;

    } catch (const std::exception &exception) {
      last_error_ = std::string("Exception in node ") + node_name_ + ": " +
                    exception.what();
      state_ = ExecutionState::ERROR;
      cook_duration_ = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - cook_start);
      return nullptr;
    }
  }

protected:
  /**
   * @brief Pure virtual function for node-specific computation
   *
   * Derived classes must implement this to define their behavior.
   * @return GeometryContainer with topology and attributes
   */
  virtual std::shared_ptr<core::GeometryContainer> execute() = 0;

  /**
   * @brief Set error message
   */
  void set_error(const std::string &error_message) {
    last_error_ = error_message;
    state_ = ExecutionState::ERROR;
  }

  /**
   * @brief Define a float parameter with fluent builder API
   */
  ParameterBuilder define_float_parameter(const std::string &name,
                                          float default_value) {
    ParameterDefinition def(name, ParameterDefinition::Type::Float,
                            default_value);
    return ParameterBuilder(def);
  }

  /**
   * @brief Define an int parameter with fluent builder API
   */
  ParameterBuilder define_int_parameter(const std::string &name,
                                        int default_value) {
    ParameterDefinition def(name, ParameterDefinition::Type::Int,
                            default_value);
    return ParameterBuilder(def);
  }

  /**
   * @brief Define a bool parameter with fluent builder API
   */
  ParameterBuilder define_bool_parameter(const std::string &name,
                                         bool default_value) {
    ParameterDefinition def(name, ParameterDefinition::Type::Bool,
                            default_value);
    return ParameterBuilder(def);
  }

  /**
   * @brief Define a string parameter with fluent builder API
   */
  ParameterBuilder define_string_parameter(const std::string &name,
                                           const std::string &default_value) {
    ParameterDefinition def(name, ParameterDefinition::Type::String,
                            default_value);
    return ParameterBuilder(def);
  }

  /**
   * @brief Register a parameter definition and initialize its value
   */
  void register_parameter(const ParameterDefinition &def) {
    parameter_definitions_.push_back(def);
    parameters_[def.name] = def.default_value;
  }

  /**
   * @brief Add universal 'class' parameter for attribute operations
   *
   * Helper for attribute nodes (AttributeCreate, AttributeDelete, Color, etc.)
   * to register a standard element class parameter.
   */
  void add_class_parameter(const std::string &name = "class",
                           const std::string &label = "Class",
                           const std::string &category = "Attribute") {
    register_parameter(
        define_int_parameter(name, 0)
            .label(label)
            .options({"Point", "Primitive", "Vertex", "Detail", "All"})
            .category(category)
            .description("Geometry element class to operate on")
            .build());
  }

  /**
   * @brief Add universal 'element_class' parameter for group operations
   *
   * Helper for group nodes (Group, GroupDelete, GroupCombine, etc.)
   * to register a standard group type parameter.
   */
  void add_group_type_parameter(const std::string &name = "element_class",
                                const std::string &label = "Group Type",
                                const std::string &category = "Group") {
    register_parameter(define_int_parameter(name, 0)
                           .label(label)
                           .options({"Points", "Primitives"})
                           .category(category)
                           .description("Type of geometry elements to group")
                           .build());
  }

  /**
   * @brief Get input data from a specific input port
   */
  std::shared_ptr<core::GeometryContainer>
  get_input_data(const std::string &port_name) const {
    auto *port = input_ports_.get_port(port_name);
    return (port != nullptr) ? port->get_data() : nullptr;
  }

  /**
   * @brief Get input data from a specific input port by index
   */
  std::shared_ptr<core::GeometryContainer>
  get_input_data(int port_index) const {
    return get_input_data(std::to_string(port_index));
  }

  /**
   * @brief Manually set input data (for testing/bridge purposes only)
   * @param port_index Port index (0-based)
   * @param data Geometry data to set
   */
  void set_input_data(int port_index,
                      std::shared_ptr<core::GeometryContainer> data) {
    auto *port = input_ports_.get_port(std::to_string(port_index));
    if (port != nullptr) {
      port->set_data(std::move(data));
    }
  }

  /**
   * @brief Check if the node has an active group filter
   * @return True if a group name is specified
   */
  bool has_group_filter() const {
    std::string group_name = get_parameter<std::string>("group", "");
    return !group_name.empty();
  }

  /**
   * @brief Get the active group name
   * @return Group name, or empty string if no group filter
   */
  std::string get_group_name() const {
    return get_parameter<std::string>("group", "");
  }

  /**
   * @brief Check if an element is in the active group
   * @param geometry Geometry to check
   * @param element_class Element class (POINT, PRIMITIVE, etc.)
   * @param element_index Index of the element
   * @return True if no group filter is active, or if element is in the group
   */
  bool is_in_active_group(const core::GeometryContainer *geometry,
                          core::ElementClass element_class,
                          size_t element_index) const {
    std::string group_name = get_group_name();
    if (group_name.empty()) {
      return true; // No filter - all elements pass
    }
    return core::is_in_group(*geometry, group_name, element_class,
                             element_index);
  }

  /**
   * @brief Helper to iterate only over points in the active group
   * @param geometry Geometry to iterate
   * @param func Function to call for each point index: void(size_t point_idx)
   */
  template <typename Func>
  void for_each_point_in_group(const core::GeometryContainer *geometry,
                               Func &&func) const {
    std::string group_name = get_group_name();
    bool use_group = !group_name.empty();

    for (size_t i = 0; i < geometry->point_count(); ++i) {
      if (!use_group || core::is_in_group(*geometry, group_name,
                                          core::ElementClass::POINT, i)) {
        func(i);
      }
    }
  }

  /**
   * @brief Helper to iterate only over primitives in the active group
   * @param geometry Geometry to iterate
   * @param func Function to call for each primitive index: void(size_t
   * prim_idx)
   */
  template <typename Func>
  void for_each_primitive_in_group(const core::GeometryContainer *geometry,
                                   Func &&func) const {
    std::string group_name = get_group_name();
    bool use_group = !group_name.empty();

    for (size_t i = 0; i < geometry->primitive_count(); ++i) {
      if (!use_group || core::is_in_group(*geometry, group_name,
                                          core::ElementClass::PRIMITIVE, i)) {
        func(i);
      }
    }
  }

private:
  /**
   * @brief Cook all input dependencies
   */
  void cook_inputs() {
    for (const auto &port : input_ports_.get_all_ports()) {
      if (port->is_connected()) {
        auto *output_port = port->get_connected_output();
        if ((output_port != nullptr) &&
            (output_port->get_owner_node() != nullptr)) {
          output_port->get_owner_node()->cook();
        }
      }
    }
  }
};

} // namespace sop
} // namespace nodo
