#pragma once

#include "node_port.hpp"
#include "nodeflux/core/geometry_container.hpp"
#include <Eigen/Dense>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace nodeflux {

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
    Type type;                    // Data type
    ParameterValue default_value; // Default value

    // UI hints
    double float_min = 0.0;
    double float_max = 100.0;
    int int_min = 0;
    int int_max = 100;
    std::vector<std::string> options; // For combo boxes (int type)

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
} // namespace nodeflux
