#pragma once

#include "nodo/core/attribute_group.hpp"
#include "nodo/core/geometry_container.hpp"
#include "nodo/core/geometry_handle.hpp"
#include "nodo/core/result.hpp"

#include "node_port.hpp"

#include <Eigen/Dense>

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

namespace nodo {

// Forward declaration
namespace graph {
class ExecutionEngine;
} // namespace graph

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
  using ParameterValue = std::variant<int, float, double, bool, std::string, Eigen::Vector3f>;
  using ParameterMap = std::unordered_map<std::string, ParameterValue>;

  /**
   * @brief Parameter definition with UI metadata (schema)
   */
  struct ParameterDefinition {
    enum class Type {
      Float,
      Int,
      Bool,
      String,
      Vector3,
      Code,
      GroupSelector
    };

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
    std::string ui_hint;              // UI widget hint (e.g., "multiline", "filepath")

    // Category visibility control (optional)
    // If set, this parameter's category is only visible when the control
    // parameter has the matching value
    std::string category_control_param; // Name of the parameter that controls
                                        // visibility
    int category_control_value = -1;    // Value that makes this category visible (-1 = always visible)

    ParameterDefinition(const std::string& n, Type t, ParameterValue def)
        : name(n), label(n), type(t), default_value(def) {}
  };

  /**
   * @brief Input configuration type - defines how a node handles inputs
   */
  enum class InputType : std::uint8_t {
    NONE,          // Generator nodes (Box, Sphere) - no inputs
    SINGLE,        // Standard modifiers (Transform, Subdivide) - 1 input
    DUAL,          // Dual-input operations (Boolean) - exactly 2 inputs
    MULTI_DYNAMIC, // Dynamic multi-input (Merge) - unlimited inputs
    MULTI_FIXED    // Fixed multi-input (Switch) - up to N inputs
  };

  /**
   * @brief Input configuration structure
   */
  struct InputConfig {
    InputType type;   // Type of input handling
    int min_count;    // Minimum required inputs
    int max_count;    // Maximum allowed inputs (-1 = unlimited)
    int initial_pins; // How many pins to show initially in UI

    InputConfig(InputType input_type, int min, int max, int initial)
        : type(input_type), min_count(min), max_count(max), initial_pins(initial) {}
  };

  /**
   * @brief Fluent builder for parameter definitions
   */
  class ParameterBuilder {
  public:
    ParameterBuilder(ParameterDefinition def) : def_(std::move(def)) {}

    ParameterBuilder& label(const std::string& lbl) {
      def_.label = lbl;
      return *this;
    }

    ParameterBuilder& category(const std::string& cat) {
      def_.category = cat;
      return *this;
    }

    ParameterBuilder& description(const std::string& desc) {
      def_.description = desc;
      return *this;
    }

    ParameterBuilder& range(double min, double max) {
      def_.float_min = min;
      def_.float_max = max;
      return *this;
    }

    ParameterBuilder& range(int min, int max) {
      def_.int_min = min;
      def_.int_max = max;
      return *this;
    }

    ParameterBuilder& options(const std::vector<std::string>& opts) {
      def_.options = opts;
      def_.int_min = 0;
      def_.int_max = static_cast<int>(opts.size()) - 1;
      return *this;
    }

    ParameterBuilder& visible_when(const std::string& control_param, int value) {
      def_.category_control_param = control_param;
      def_.category_control_value = value;
      return *this;
    }

    ParameterBuilder& hint(const std::string& ui_hint) {
      def_.ui_hint = ui_hint;
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
  bool pass_through_ = false; // When true, passes first input unchanged

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
  NodePort* main_output_ = nullptr;

public:
  SOPNode(std::string node_name, std::string node_type)
      : node_name_(std::move(node_name)), node_type_(std::move(node_type)) {
    // Create default geometry output port
    main_output_ = output_ports_.add_port("geometry", NodePort::Type::OUTPUT, NodePort::DataType::GEOMETRY, this);

    // Add universal group parameter (all SOPs inherit this)
    register_parameter(define_group_selector_parameter("input_group", "")
                           .label("Group")
                           .category("Universal")
                           .description("Name of group to operate on (empty = all elements)")
                           .build());
  }

  virtual ~SOPNode() = default;

  // Non-copyable but movable
  SOPNode(const SOPNode&) = delete;
  SOPNode& operator=(const SOPNode&) = delete;
  SOPNode(SOPNode&&) = default;
  SOPNode& operator=(SOPNode&&) = default;

  /**
   * @brief Get node name
   */
  const std::string& get_name() const { return node_name_; }

  /**
   * @brief Get node type
   */
  const std::string& get_type() const { return node_type_; }

  /**
   * @brief Get current execution state
   */
  ExecutionState get_state() const { return state_; }

  /**
   * @brief Set/get pass-through flag
   * When true, the node passes its first input through without processing
   */
  void set_pass_through(bool pass_through) { pass_through_ = pass_through; }
  bool is_pass_through() const { return pass_through_; }

  /**
   * @brief Get last error message
   */
  const std::string& get_last_error() const { return last_error_; }

  /**
   * @brief Get last cook duration
   */
  std::chrono::milliseconds get_cook_duration() const { return cook_duration_; }

  /**
   * @brief Get input ports
   */
  PortCollection& get_input_ports() { return input_ports_; }
  const PortCollection& get_input_ports() const { return input_ports_; }

  /**
   * @brief Get output ports
   */
  PortCollection& get_output_ports() { return output_ports_; }
  const PortCollection& get_output_ports() const { return output_ports_; }

  /**
   * @brief Get main output port
   */
  NodePort* get_main_output() const { return main_output_; }

  /**
   * @brief Set parameter value
   */
  template <typename T>
  void set_parameter(const std::string& name, T value) {
    parameters_[name] = value;
    mark_dirty();
  }

  /**
   * @brief Get parameter value
   */
  template <typename T>
  T get_parameter(const std::string& name, T default_value = T{}) const {
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
  bool has_parameter(const std::string& name) const { return parameters_.find(name) != parameters_.end(); }

  /**
   * @brief Get all parameter definitions (schema)
   */
  const std::vector<ParameterDefinition>& get_parameter_definitions() const { return parameter_definitions_; }

  /**
   * @brief Get parameter map (current values)
   */
  const ParameterMap& get_parameters() const { return parameters_; }

  /**
   * @brief Mark node as dirty (needs recomputation)
   */
  void mark_dirty() { state_ = ExecutionState::DIRTY; }

  auto isNodeClean() const -> bool { return state_ == ExecutionState::CLEAN; }

  auto getCachedOutput() const -> std::shared_ptr<core::GeometryContainer> {
    if (main_output_->is_cache_valid()) {
      return main_output_->get_data();
    }
    return nullptr;
  }

  auto setComputingState() -> void { state_ = ExecutionState::COMPUTING; }

  auto preCookInputs() -> void {
    // Prevent recursive cooking
    if (state_ == ExecutionState::COMPUTING) {
      last_error_ = "Circular dependency detected in node: " + node_name_;
      state_ = ExecutionState::ERROR;
      // return nullptr;
    }

    auto cook_start = std::chrono::steady_clock::now();
    setComputingState();
    last_error_.clear();

    try {
      // Cook input dependencies first
      cookInputs();
    } catch (const std::exception& exception) {
      last_error_ = std::string("Exception in node ") + node_name_ + ": " + exception.what();
      state_ = ExecutionState::ERROR;
      cook_duration_ =
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - cook_start);
      // return nullptr;
    }
  }

  auto getPassthroughResult() const -> std::shared_ptr<core::GeometryContainer> {
    auto input = get_input_data(0);
    if (!input || is_pass_through()) {
      return nullptr;
    }

    return input;
  }

  auto executeAndHandleErrors() -> core::Result<std::shared_ptr<core::GeometryContainer>> {
    try {
      auto result = execute();
      return result;
    } catch (const std::exception& exception) {
      return {(std::string("Exception in node ") + node_name_ + ": " + exception.what())};
    }
  }

  auto updateCacheAndState(std::shared_ptr<core::GeometryContainer> result) -> void {
    // Update output port cache
    main_output_->set_data(std::move(result));
    state_ = ExecutionState::CLEAN;
  }

  /**
   * @brief Cook (execute) this node
   *
   * This is the main execution entry point. It handles caching,
   * dependency resolution, and error handling.
   */
  std::shared_ptr<core::GeometryContainer> cook() {
    // Return cached result if clean
    // TODO check the whole process on how we mark dirty and clean
    // if (isNodeClean()) {
    //   return getCachedOutput();
    // }

    auto cook_start = std::chrono::steady_clock::now();
    setComputingState();

    try {
      // Cook input dependencies first
      preCookInputs();

      if (is_pass_through()) {
        auto result = getPassthroughResult();
        // Calulate duration for pass-through case
        cook_duration_ =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - cook_start);
        return result;
      }

      // Execute the node-specific computation
      auto result = executeAndHandleErrors();

      if (result.is_error()) {
        set_error(*result.error());
        cook_duration_ =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - cook_start);
        return nullptr;
      }

      updateCacheAndState(*result.value());
      cook_duration_ =
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - cook_start);
      return *result.value();
    } catch (const std::exception& exception) {
      last_error_ = std::string("Exception in node: ") + node_name_ + ": " + exception.what();
      state_ = ExecutionState::ERROR;
      cook_duration_ =
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - cook_start);
      return nullptr;
    }
  }

  /**
   * @brief Get input configuration for this node type
   * Override in derived classes to specify input handling behavior
   * @return InputConfig with type, min/max counts, and initial pin count
   */
  virtual InputConfig get_input_config() const {
    // Default: single input modifier (Transform, Subdivide, etc.)
    return InputConfig(InputType::SINGLE, 1, 1, 1);
  }

protected:
  /**
   * @brief Pure virtual function for node-specific computation
   *
   * Derived classes must implement this to define their behavior.
   * @return GeometryContainer with topology and attributes
   */
  virtual core::Result<std::shared_ptr<core::GeometryContainer>> execute() = 0;

  /**
   * @brief Set error message
   */
  void set_error(const std::string& error_message) {
    last_error_ = error_message;
    state_ = ExecutionState::ERROR;
  }

  /**
   * @brief Define a float parameter with fluent builder API
   */
  ParameterBuilder define_float_parameter(const std::string& name, float default_value) {
    ParameterDefinition def(name, ParameterDefinition::Type::Float, default_value);
    return ParameterBuilder(def);
  }

  /**
   * @brief Define an int parameter with fluent builder API
   */
  ParameterBuilder define_int_parameter(const std::string& name, int default_value) {
    ParameterDefinition def(name, ParameterDefinition::Type::Int, default_value);
    return ParameterBuilder(def);
  }

  /**
   * @brief Define a bool parameter with fluent builder API
   */
  ParameterBuilder define_bool_parameter(const std::string& name, bool default_value) {
    ParameterDefinition def(name, ParameterDefinition::Type::Bool, default_value);
    return ParameterBuilder(def);
  }

  /**
   * @brief Define a string parameter with fluent builder API
   */
  ParameterBuilder define_string_parameter(const std::string& name, const std::string& default_value) {
    ParameterDefinition def(name, ParameterDefinition::Type::String, default_value);
    return ParameterBuilder(def);
  }

  /**
   * @brief Define a code/expression parameter with fluent builder API
   * Multi-line text editor for code, expressions, scripts
   */
  ParameterBuilder define_code_parameter(const std::string& name, const std::string& default_value) {
    ParameterDefinition def(name, ParameterDefinition::Type::Code, default_value);
    return ParameterBuilder(def);
  }

  /**
   * @brief Define a vector3 parameter with fluent builder API
   */
  ParameterBuilder define_vector3_parameter(const std::string& name, const Eigen::Vector3f& default_value) {
    ParameterDefinition def(name, ParameterDefinition::Type::Vector3, default_value);
    return ParameterBuilder(def);
  }

  auto define_group_selector_parameter(const std::string& name, const std::string& default_value = "")
      -> ParameterBuilder {
    ParameterDefinition def(name, ParameterDefinition::Type::GroupSelector, default_value);
    return {def};
  }

  /**
   * @brief Register a parameter definition and initialize its value
   */
  void register_parameter(const ParameterDefinition& def) {
    parameter_definitions_.push_back(def);
    parameters_[def.name] = def.default_value;
  }

  /**
   * @brief Add universal 'class' parameter for attribute operations
   *
   * Helper for attribute nodes (AttributeCreate, AttributeDelete, Color, etc.)
   * to register a standard element class parameter.
   */
  void add_class_parameter(const std::string& name = "class", const std::string& label = "Class",
                           const std::string& category = "Attribute") {
    register_parameter(define_int_parameter(name, 0)
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
  void add_group_type_parameter(const std::string& name = "element_class", const std::string& label = "Group Type",
                                const std::string& category = "Group") {
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
  std::shared_ptr<core::GeometryContainer> get_input_data(const std::string& port_name) const {
    auto* port = input_ports_.get_port(port_name);
    return (port != nullptr) ? port->get_data() : nullptr;
  }

  /**
   * @brief Get input data from a specific input port by index
   */
  std::shared_ptr<core::GeometryContainer> get_input_data(int port_index) const {
    return get_input_data(std::to_string(port_index));
  }

  /**
   * @brief Get input as GeometryHandle (COW-aware, zero-copy for reads)
   *
   * Returns a handle that shares the input geometry. Multiple handles can
   * share the same data until one needs to modify it (copy-on-write).
   *
   * @param port_index Port index (0-based)
   * @return GeometryHandle wrapping input data (may be invalid if no input)
   */
  core::GeometryHandle get_input_handle(int port_index) const {
    auto data = get_input_data(port_index);
    if (data) {
      return core::GeometryHandle(std::move(data));
    }
    return core::GeometryHandle(); // Empty handle
  }

  /**
   * @brief Get writable input handle (triggers COW if shared)
   *
   * Convenience method that gets an input handle and immediately ensures
   * it's unique (copyable). Use this when you know you'll modify the input.
   *
   * @param port_index Port index (0-based)
   * @return GeometryHandle with exclusive ownership
   */
  core::GeometryHandle get_input_writable(int port_index) const {
    auto handle = get_input_handle(port_index);
    handle.make_unique(); // Force COW if shared
    return handle;
  }

public:
  /**
   * @brief Manually set input data (for testing/bridge purposes only)
   * @param port_index Port index (0-based)
   * @param data Geometry data to set
   */
  void set_input_data(int port_index, std::shared_ptr<core::GeometryContainer> data) {
    auto* port = input_ports_.get_port(std::to_string(port_index));
    if (port != nullptr) {
      port->set_data(std::move(data));
    }
  }

protected:
  /**
   * @brief Check if the node has an active group filter
   * @return True if a group name is specified
   */
  bool has_group_filter() const {
    std::string group_name = get_parameter<std::string>("input_group", "");
    return !group_name.empty();
  }

  /**
   * @brief Get the active group name
   * @return Group name, or empty string if no group filter
   */
  std::string get_group_name() const { return get_parameter<std::string>("input_group", ""); }

  /**
   * @brief Check if an element is in the active group
   * @param geometry Geometry to check
   * @param element_class Element class (POINT, PRIMITIVE, etc.)
   * @param element_index Index of the element
   * @return True if no group filter is active, or if element is in the group
   */
  bool is_in_active_group(const core::GeometryContainer* geometry, core::ElementClass element_class,
                          size_t element_index) const {
    std::string group_name = get_group_name();
    if (group_name.empty()) {
      return true; // No filter - all elements pass
    }
    return core::is_in_group(*geometry, group_name, element_class, element_index);
  }

  /**
   * @brief Helper to iterate only over points in the active group
   * @param geometry Geometry to iterate
   * @param func Function to call for each point index: void(size_t point_idx)
   */
  template <typename Func>
  void for_each_point_in_group(const core::GeometryContainer* geometry, Func&& func) const {
    std::string group_name = get_group_name();
    bool use_group = !group_name.empty();

    for (size_t i = 0; i < geometry->point_count(); ++i) {
      if (!use_group || core::is_in_group(*geometry, group_name, core::ElementClass::POINT, i)) {
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
  void for_each_primitive_in_group(const core::GeometryContainer* geometry, Func&& func) const {
    std::string group_name = get_group_name();
    bool use_group = !group_name.empty();

    for (size_t i = 0; i < geometry->primitive_count(); ++i) {
      if (!use_group || core::is_in_group(*geometry, group_name, core::ElementClass::PRIMITIVE, i)) {
        func(i);
      }
    }
  }

  /**
   * @brief Get indices of points in the active group
   * @param geometry Geometry to check
   * @return Vector of point indices (all points if no group filter)
   */
  std::vector<size_t> get_grouped_point_indices(const core::GeometryContainer* geometry) const {
    std::vector<size_t> indices;
    std::string group_name = get_group_name();

    if (group_name.empty()) {
      // No filter - return all point indices
      indices.reserve(geometry->point_count());
      for (size_t i = 0; i < geometry->point_count(); ++i) {
        indices.push_back(i);
      }
    } else {
      // Return only points in the group
      indices = core::get_group_elements(*geometry, group_name, core::ElementClass::POINT);
    }
    return indices;
  }

  /**
   * @brief Get indices of primitives in the active group
   * @param geometry Geometry to check
   * @return Vector of primitive indices (all primitives if no group filter)
   */
  std::vector<size_t> get_grouped_primitive_indices(const core::GeometryContainer* geometry) const {
    std::vector<size_t> indices;
    std::string group_name = get_group_name();

    if (group_name.empty()) {
      // No filter - return all primitive indices
      indices.reserve(geometry->primitive_count());
      for (size_t i = 0; i < geometry->primitive_count(); ++i) {
        indices.push_back(i);
      }
    } else {
      // Return only primitives in the group
      indices = core::get_group_elements(*geometry, group_name, core::ElementClass::PRIMITIVE);
    }
    return indices;
  }

  /**
   * @brief Check if a specific primitive should be processed (is in active group)
   * @param geometry Geometry to check
   * @param prim_idx Primitive index
   * @return True if primitive should be processed
   */
  bool should_process_primitive(const core::GeometryContainer* geometry, size_t prim_idx) const {
    return is_in_active_group(geometry, core::ElementClass::PRIMITIVE, prim_idx);
  }

  /**
   * @brief Check if a specific point should be processed (is in active group)
   * @param geometry Geometry to check
   * @param point_idx Point index
   * @return True if point should be processed
   */
  bool should_process_point(const core::GeometryContainer* geometry, size_t point_idx) const {
    return is_in_active_group(geometry, core::ElementClass::POINT, point_idx);
  }

  /**
   * @brief Check if element is in the active group (works with attr_class
   * parameter)
   * @param geo Geometry container
   * @param attr_class Element class as integer (0=Point, 1=Vertex, 2=Primitive)
   * @param index Element index
   * @return True if element passes group filter (or no filter active)
   */
  bool is_in_group(const std::shared_ptr<core::GeometryContainer>& geo, int attr_class, size_t index) const {
    const std::string group_name = get_group_name();

    // Empty group name means no filtering (all elements pass)
    if (group_name.empty()) {
      return true;
    }

    // Build the attribute name: "group_<name>"
    const std::string attr_name = "group_" + group_name;

    // Check based on element class
    switch (attr_class) {
      case 0: { // Point
        if (!geo->has_point_attribute(attr_name)) {
          return false;
        }
        auto* group_attr = geo->get_point_attribute_typed<int>(attr_name);
        if (group_attr && index < group_attr->size()) {
          return (*group_attr)[index] > 0;
        }
        break;
      }
      case 1: { // Vertex
        if (!geo->has_vertex_attribute(attr_name)) {
          return false;
        }
        auto* group_attr = geo->get_vertex_attribute_typed<int>(attr_name);
        if (group_attr && index < group_attr->size()) {
          return (*group_attr)[index] > 0;
        }
        break;
      }
      case 2: { // Primitive
        if (!geo->has_primitive_attribute(attr_name)) {
          return false;
        }
        auto* group_attr = geo->get_primitive_attribute_typed<int>(attr_name);
        if (group_attr && index < group_attr->size()) {
          return (*group_attr)[index] > 0;
        }
        break;
      }
    }

    return false;
  }

  /**
   * @brief Apply group filter to input geometry (keep only grouped elements)
   *
   * Helper method that filters input geometry based on the input_group parameter.
   * Call this at the start of execute() to work only on grouped elements.
   * Returns a geometry containing ONLY the elements in the specified group.
   *
   * @param port_index Input port index (default 0)
   * @param element_class Element class to filter (POINT or PRIMITIVE)
   * @param delete_orphaned_points If true, remove points not used by remaining primitives (only for PRIMITIVE
   * filtering)
   * @return Filtered geometry, or original if no group specified, or error if group doesn't exist
   *
   * Example usage:
   * @code
   * auto filtered = apply_group_filter(0, core::ElementClass::PRIMITIVE);
   * if (!filtered) return filtered.error();
   * auto input = filtered.value();
   * // Now process input which only contains grouped elements
   * @endcode
   */
  core::Result<std::shared_ptr<core::GeometryContainer>>
  apply_group_filter(int port_index = 0, core::ElementClass element_class = core::ElementClass::PRIMITIVE,
                     bool delete_orphaned_points = false) const {
    auto input = get_input_data(port_index);
    if (!input) {
      return {"No input geometry"};
    }

    std::string group_name = get_group_name();

    // No filter - return input directly (nodes must handle COW via handle.write())
    if (group_name.empty()) {
      return input;
    }

    // Check if group exists
    if (!core::has_group(*input, group_name, element_class)) {
      return {"Group '" + group_name + "' does not exist on input geometry"};
    }

    // Check if group is empty
    auto elements = core::get_group_elements(*input, group_name, element_class);
    if (elements.empty()) {
      return {"Group '" + group_name + "' is empty"};
    }

    // Create inverted group (elements NOT in the group)
    auto result_copy = std::make_shared<core::GeometryContainer>(input->clone());
    std::string inverted_group = "__inverted_" + group_name;

    // Create inverted group
    core::create_group(*result_copy, inverted_group, element_class);
    size_t elem_count =
        (element_class == core::ElementClass::POINT) ? result_copy->point_count() : result_copy->primitive_count();

    // Add all non-grouped elements to inverted group
    for (size_t i = 0; i < elem_count; ++i) {
      if (!core::is_in_group(*input, group_name, element_class, i)) {
        core::add_to_group(*result_copy, inverted_group, element_class, i);
      }
    }

    // Delete the inverted group (keeping only original group)
    auto filtered = result_copy->delete_elements(inverted_group, element_class, delete_orphaned_points);
    if (!filtered) {
      return {"Failed to filter by group '" + group_name + "'"};
    }

    return filtered;
  }

private:
  /**
   * @brief Cook all input dependencies
   */
  void cookInputs() {
    for (const auto& port : input_ports_.get_all_ports()) {
      if (port->is_connected()) {
        auto* output_port = port->get_connected_output();
        if ((output_port != nullptr) && (output_port->get_owner_node() != nullptr)) {
          output_port->get_owner_node()->cook();
        }
      }
    }
  }
};

} // namespace sop
} // namespace nodo
