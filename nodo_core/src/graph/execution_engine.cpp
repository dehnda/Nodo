/**
 * Nodo - Graph Execution Engine Implementation
 */

#include "nodo/graph/execution_engine.hpp"

#include "nodo/core/mesh.hpp"
#include "nodo/geometry/mesh_generator.hpp"
#include "nodo/geometry/plane_generator.hpp"
#include "nodo/geometry/sphere_generator.hpp"
#include "nodo/geometry/torus_generator.hpp"
#include "nodo/graph/parameter_expression_resolver.hpp"
#include "nodo/sop/boolean_sop.hpp"
#include "nodo/sop/copy_to_points_sop.hpp"
#include "nodo/sop/extrude_sop.hpp"
#include "nodo/sop/laplacian_sop.hpp"
#include "nodo/sop/line_sop.hpp"
#include "nodo/sop/mirror_sop.hpp"
#include "nodo/sop/noise_displacement_sop.hpp"
#include "nodo/sop/polyextrude_sop.hpp"
#include "nodo/sop/resample_sop.hpp"
#include "nodo/sop/scatter_sop.hpp"
#include "nodo/sop/sop_factory.hpp"
#include "nodo/sop/subdivisions_sop.hpp"
#include "nodo/sop/transform_sop.hpp"

#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <set>

// For trigonometry and pi constant
#include <cmath>
#include <numbers>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::graph {

// Temporary helper to convert GeometryContainer to core::Mesh
// TODO: Remove this once all execution pipeline migrated to GeometryContainer
static std::shared_ptr<core::Mesh> convert_container_to_mesh(const core::GeometryContainer& container) {
  // Get positions from container
  auto* positions = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (positions == nullptr) {
    return std::make_shared<core::Mesh>(); // Empty mesh
  }

  size_t point_count = container.topology().point_count();
  size_t prim_count = container.topology().primitive_count();

  // Create mesh
  core::Mesh::Vertices vertices(point_count, 3);
  core::Mesh::Faces faces(prim_count, 3);

  // Copy positions
  for (size_t i = 0; i < point_count; ++i) {
    const auto& pos = (*positions)[i];
    vertices(i, 0) = pos.x();
    vertices(i, 1) = pos.y();
    vertices(i, 2) = pos.z();
  }

  // Copy face indices
  for (size_t i = 0; i < prim_count; ++i) {
    const auto prim_verts = container.topology().get_primitive_vertices(i);
    if (prim_verts.size() >= 3) {
      faces(i, 0) = prim_verts[0];
      faces(i, 1) = prim_verts[1];
      faces(i, 2) = prim_verts[2];
    }
  }

  return std::make_shared<core::Mesh>(vertices, faces);
}

bool ExecutionEngine::execute_graph(NodeGraph& graph) {
  // Get display node (if set, only execute its dependencies)
  int display_node_id = graph.get_display_node();

  std::vector<int> execution_order;

  if (display_node_id >= 0) {
    // Selective execution: only cook nodes needed for display node
    execution_order = graph.get_upstream_dependencies(display_node_id);
  } else {
    // No display node: cook everything (fallback to old behavior)
    execution_order = graph.get_execution_order();
  }

  // DON'T clear the cache completely - only clear nodes that need recomputation
  // This preserves expensive operations like UV unwrap
  // geometry_cache_.clear();  // OLD: cleared everything

  // Clear error flags from all nodes
  for (const auto& node_ptr : graph.get_nodes()) {
    node_ptr->set_error(false);
  }

  // Report start of execution
  int total_nodes = execution_order.size();
  int completed_nodes = 0;

  // Execute nodes in dependency order
  for (int node_id : execution_order) {
    // Report progress before executing each node
    notify_progress(completed_nodes, total_nodes);

    auto* node = graph.get_node(node_id);
    if (!node) {
      std::cout << "❌ Node " << node_id << " not found" << std::endl;
      return false;
    }

    // Check if this node is already cached and valid
    // Only use cache if node doesn't need update
    auto cached_it = geometry_cache_.find(node_id);
    bool has_valid_cache = (cached_it != geometry_cache_.end()) && !node->needs_update();

    // Skip execution if already cached and doesn't need update
    if (has_valid_cache) {
      continue;
    }

    // Start timing
    auto start_time = std::chrono::high_resolution_clock::now();

    // Unified SOP execution path
    std::shared_ptr<core::GeometryContainer> geometry_result = nullptr;

    // Create SOP instance via factory
    auto sop = sop::SOPFactory::create(node->get_type(), node->get_name());

    if (!sop) {
      // Unsupported node type (e.g., Switch)
      std::cout << "⚠️ Node type not supported: " << node->get_name() << std::endl;
      continue;
    }

    // Transfer parameters from GraphNode to SOP (with expression resolution)
    transfer_parameters(*node, *sop, graph);

    // Transfer pass-through flag from GraphNode to SOP
    sop->set_pass_through(node->is_pass_through());

    // Gather input geometries and set them on the SOP
    auto input_geometries = gather_input_geometries(graph, node_id);
    for (const auto& [port_index, geometry] : input_geometries) {
      sop->set_input_data(port_index, geometry);
    }

    // Execute the SOP (cook)
    geometry_result = sop->cook();

    // Sync parameters back from SOP to GraphNode
    // This handles dynamic parameters added during execution (e.g., Wrangle
    // ch() params)
    sync_parameters_from_sop(*sop, *node);

    // End timing and calculate duration
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    double cook_time_ms = duration.count() / 1000.0; // Convert to milliseconds

    // Store cook time in node
    node->set_cook_time(cook_time_ms);

    // Store the result or mark error
    if (geometry_result) {
      // Cache GeometryContainer
      geometry_cache_[node_id] = geometry_result;

      // Convert to Mesh for legacy API
      auto mesh_result = convert_container_to_mesh(*geometry_result);
      node->set_output_mesh(mesh_result);
      node->set_error(false); // Clear any previous error
      node->mark_updated();   // Mark as no longer needing update

      // Increment completed count after successful execution
      completed_nodes++;
    } else {
      // Get the actual error message from the SOPNode
      std::string error_msg = sop->get_last_error();
      if (error_msg.empty()) {
        error_msg = "Execution failed - no result generated";
      }

      std::cout << "❌ Node " << node_id << " (" << sop->get_type() << ") execution failed: " << error_msg << std::endl;
      node->set_error(true, error_msg);
      notify_error(error_msg, node_id);
      return false;
    }
  }

  // Report completion
  notify_progress(completed_nodes, total_nodes);

  return true;
}

std::shared_ptr<core::GeometryContainer> ExecutionEngine::get_node_geometry(int node_id) const {
  auto it = geometry_cache_.find(node_id);
  return it != geometry_cache_.end() ? it->second : nullptr;
}

void ExecutionEngine::clear_cache() {
  geometry_cache_.clear();
}

void ExecutionEngine::invalidate_node(NodeGraph& graph, int node_id) {
  // Remove this node from cache
  geometry_cache_.erase(node_id);

  // Find all downstream nodes (nodes that depend on this one)
  const auto& all_nodes = graph.get_nodes();
  for (const auto& node_ptr : all_nodes) {
    int other_id = node_ptr->get_id();
    if (other_id == node_id) {
      continue;
    }

    // Check if this node depends on the invalidated node
    auto dependencies = graph.get_upstream_dependencies(other_id);
    if (std::find(dependencies.begin(), dependencies.end(), node_id) != dependencies.end()) {
      // This node depends on the invalidated node, so invalidate it too
      geometry_cache_.erase(other_id);
    }
  }
}

void ExecutionEngine::transfer_parameters(const GraphNode& graph_node, sop::SOPNode& sop_node, const NodeGraph& graph) {
  const auto& node_params = graph_node.get_parameters();
  ParameterExpressionResolver resolver(graph, &node_params, graph_node.get_id());

  // Iterate over all parameters in GraphNode and transfer to SOPNode
  for (const auto& param : node_params) {
    switch (param.type) {
      case NodeParameter::Type::Float: {
        if (param.has_expression()) {
          // Evaluate expression
          auto result = resolver.resolve_float(param.get_expression());
          if (result.has_value()) {
            sop_node.set_parameter(param.name, result.value());
          } else {
            // Fallback to literal value if expression evaluation fails
            sop_node.set_parameter(param.name, param.float_value);
          }
        } else {
          // Use literal value
          sop_node.set_parameter(param.name, param.float_value);
        }
        break;
      }

      case NodeParameter::Type::Int: {
        if (param.has_expression()) {
          // Evaluate expression
          auto result = resolver.resolve_int(param.get_expression());
          if (result.has_value()) {
            sop_node.set_parameter(param.name, result.value());
          } else {
            // Fallback to literal value if expression evaluation fails
            sop_node.set_parameter(param.name, param.int_value);
          }
        } else {
          // Use literal value
          sop_node.set_parameter(param.name, param.int_value);
        }
        break;
      }

      case NodeParameter::Type::Bool:
        sop_node.set_parameter(param.name, param.bool_value);
        break;

      case NodeParameter::Type::String:
      case NodeParameter::Type::Code:
      case NodeParameter::Type::GroupSelector: {
        // For Code type, pass through directly without resolution
        // (@ symbols are VEX attribute syntax, not parameter references)
        // For GroupSelector, treat similarly to String
        if (param.type == NodeParameter::Type::Code) {
          sop_node.set_parameter(param.name, param.string_value);
        } else {
          // For String and GroupSelector types, resolve any graph parameter
          // references
          if (resolver.has_references(param.string_value)) {
            std::string resolved = resolver.resolve(param.string_value);
            sop_node.set_parameter(param.name, resolved);
          } else {
            sop_node.set_parameter(param.name, param.string_value);
          }
        }
        break;
      }

      case NodeParameter::Type::Vector3: {
        Eigen::Vector3f vec3;

        if (param.has_expression()) {
          // For Vector3, expression could be:
          // 1. Three comma-separated expressions: "1.0, 2.0, 3.0"
          // 2. Single expression for all components: "$offset"
          // 3. Math per component: "$x + 1, $y * 2, $z"

          const std::string& expr = param.get_expression();

          // Try to parse as comma-separated values
          size_t comma1 = expr.find(',');
          if (comma1 != std::string::npos) {
            size_t comma2 = expr.find(',', comma1 + 1);
            if (comma2 != std::string::npos) {
              // Three components
              std::string x_expr = expr.substr(0, comma1);
              std::string y_expr = expr.substr(comma1 + 1, comma2 - comma1 - 1);
              std::string z_expr = expr.substr(comma2 + 1);

              // Trim whitespace
              auto trim = [](std::string& s) {
                s.erase(0, s.find_first_not_of(" \t"));
                s.erase(s.find_last_not_of(" \t") + 1);
              };
              trim(x_expr);
              trim(y_expr);
              trim(z_expr);

              // Evaluate each component
              auto x_val = resolver.resolve_float(x_expr);
              auto y_val = resolver.resolve_float(y_expr);
              auto z_val = resolver.resolve_float(z_expr);

              vec3[0] = x_val.value_or(param.vector3_value[0]);
              vec3[1] = y_val.value_or(param.vector3_value[1]);
              vec3[2] = z_val.value_or(param.vector3_value[2]);
            } else {
              // Fallback to literal
              vec3 = Eigen::Vector3f(param.vector3_value[0], param.vector3_value[1], param.vector3_value[2]);
            }
          } else {
            // Single expression - use for all components
            auto result = resolver.resolve_float(expr);
            float val = result.value_or(0.0f);
            vec3 = Eigen::Vector3f(val, val, val);
          }
        } else {
          // Use literal value
          vec3 = Eigen::Vector3f(param.vector3_value[0], param.vector3_value[1], param.vector3_value[2]);
        }

        sop_node.set_parameter(param.name, vec3);
        break;
      }
    }
  }
}

void ExecutionEngine::sync_parameters_from_sop(const sop::SOPNode& sop_node, GraphNode& graph_node) {
  // Completely rebuild GraphNode parameters from SOP definitions
  // This handles both adding new parameters and removing obsolete ones

  const auto& sop_params = sop_node.get_parameter_definitions();

  // Build a set of current SOP parameter names
  std::set<std::string> sop_param_names;
  for (const auto& sop_param : sop_params) {
    sop_param_names.insert(sop_param.name);
  }

  // Get existing GraphNode parameters to preserve their values
  const auto& existing_params = graph_node.get_parameters();

  // Remove obsolete parameters (that exist in GraphNode but not in SOP)
  for (const auto& existing : existing_params) {
    if (sop_param_names.find(existing.name) == sop_param_names.end()) {
      graph_node.remove_parameter(existing.name);
    }
  }

  // Check which parameters are new or need updates
  for (const auto& sop_param : sop_params) {
    // Check if parameter already exists in GraphNode
    auto existing_param_opt = graph_node.get_parameter(sop_param.name);

    if (existing_param_opt.has_value()) {
      // Parameter exists - update its value from SOP if changed
      auto& existing_param = existing_param_opt.value();

      // Get current value from SOP's parameter map
      auto sop_value = sop_node.get_parameters();
      auto sop_value_it = sop_value.find(sop_param.name);

      if (sop_value_it != sop_value.end()) {
        // Update the value based on type
        if (std::holds_alternative<int>(sop_value_it->second)) {
          int new_value = std::get<int>(sop_value_it->second);
          if (existing_param.int_value != new_value) {
            existing_param.int_value = new_value;
            graph_node.set_parameter(sop_param.name, existing_param);
          }
        } else if (std::holds_alternative<float>(sop_value_it->second)) {
          float new_value = std::get<float>(sop_value_it->second);
          if (existing_param.float_value != new_value) {
            existing_param.float_value = new_value;
            graph_node.set_parameter(sop_param.name, existing_param);
          }
        } else if (std::holds_alternative<bool>(sop_value_it->second)) {
          bool new_value = std::get<bool>(sop_value_it->second);
          if (existing_param.bool_value != new_value) {
            existing_param.bool_value = new_value;
            graph_node.set_parameter(sop_param.name, existing_param);
          }
        } else if (std::holds_alternative<std::string>(sop_value_it->second)) {
          std::string new_value = std::get<std::string>(sop_value_it->second);
          if (existing_param.string_value != new_value) {
            existing_param.string_value = new_value;
            graph_node.set_parameter(sop_param.name, existing_param);
          }
        }
      }
      continue;
    }

    // New parameter - add it
    NodeParameter new_param = NodeParameter(sop_param.name, 0.0F, sop_param.label, sop_param.float_min,
                                            sop_param.float_max, sop_param.category);

    // Update with actual type and default value
    if (std::holds_alternative<float>(sop_param.default_value)) {
      new_param.float_value = std::get<float>(sop_param.default_value);
    }

    // Copy visibility control
    new_param.category_control_param = sop_param.category_control_param;
    new_param.category_control_value = sop_param.category_control_value;

    graph_node.add_parameter(new_param);
  }
}

std::unordered_map<int, std::shared_ptr<core::GeometryContainer>>
ExecutionEngine::gather_input_geometries(const NodeGraph& graph, int node_id) {
  std::unordered_map<int, std::shared_ptr<core::GeometryContainer>> input_geometries;

  // Get all connections that target this node
  const auto& connections = graph.get_connections();

  // Collect connections to this node, grouped by target pin
  std::unordered_map<int, std::vector<NodeConnection>> connections_by_pin;
  for (const auto& connection : connections) {
    if (connection.target_node_id == node_id) {
      connections_by_pin[connection.target_pin_index].push_back(connection);
    }
  }

  // Process each target pin
  int input_index = 0;
  for (auto& [pin_index, pin_connections] : connections_by_pin) {
    // For pins with multiple connections (MULTI_DYNAMIC nodes),
    // assign them to sequential input indices in order of connection ID
    std::sort(pin_connections.begin(), pin_connections.end(),
              [](const NodeConnection& a, const NodeConnection& b) { return a.id < b.id; });

    for (const auto& connection : pin_connections) {
      // Find the geometry from the source node
      auto geometry_iterator = geometry_cache_.find(connection.source_node_id);
      if (geometry_iterator != geometry_cache_.end() && geometry_iterator->second != nullptr) {
        // For single connection to pin: use pin index directly
        // For multiple connections to same pin: use sequential indices
        int mapped_index = (pin_connections.size() > 1) ? input_index++ : pin_index;
        input_geometries[mapped_index] = geometry_iterator->second;
      }
    }
  }

  return input_geometries;
}

void ExecutionEngine::notify_progress(int completed, int total) {
  // Call legacy progress callback
  if (progress_callback_) {
    progress_callback_(completed, total);
  }

  // Call host interface progress reporting (M2.1)
  if (host_interface_ != nullptr) {
    std::string message = "Executing node " + std::to_string(completed) + " of " + std::to_string(total);
    bool continue_execution = host_interface_->report_progress(completed, total, message);
    // Note: cancellation check would need to propagate up to execute_graph
    // For now, just report progress without cancellation support
    (void)continue_execution; // Suppress unused variable warning
  }
}

void ExecutionEngine::notify_error(const std::string& error, int node_id) {
  // Call legacy error callback
  if (error_callback_) {
    error_callback_(error, node_id);
  }

  // Log error to host interface (M2.1)
  if (host_interface_ != nullptr) {
    std::string error_msg = "Node " + std::to_string(node_id) + ": " + error;
    host_interface_->log("error", error_msg);
  }
}

} // namespace nodo::graph
