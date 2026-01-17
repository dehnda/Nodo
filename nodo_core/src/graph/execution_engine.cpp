/**
 * Nodo - Graph Execution Engine Implementation
 */

#include "nodo/graph/execution_engine.hpp"

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

    // Get persistent SOP instance
    auto* sop = node->get_sop();

    if (!sop) {
      // Unsupported node type (e.g., Switch)
      std::cout << "⚠️ Node type not supported: " << node->get_name() << std::endl;
      continue;
    }

    // TODO: Implement expression resolution for SOPNode directly
    // For now, SOPs use their default parameter values

    // Transfer pass-through flag from GraphNode to SOP
    sop->set_pass_through(node->is_pass_through());

    // Gather input geometries and set them on the SOP
    auto input_geometries = gather_input_geometries(graph, node_id);
    for (const auto& [port_index, geometry] : input_geometries) {
      sop->set_input_data(port_index, geometry);
    }

    // Execute the SOP (cook)
    geometry_result = sop->cook();

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
