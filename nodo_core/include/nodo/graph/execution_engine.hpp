/**
 * NodeFlux Engine - Graph Execution Engine
 * Executes node graphs in correct dependency order
 */

#pragma once

#include "nodo/core/geometry_container.hpp"
#include "nodo/graph/node_graph.hpp"
#include <functional>
#include <memory>
#include <unordered_map>

// Forward declarations
namespace nodo::sop {
class SOPNode;
}

namespace nodo::graph {

/**
 * @brief Execution context for a single node
 */
struct ExecutionContext {
  GraphNode *node;
  std::vector<std::shared_ptr<core::GeometryContainer>> input_geometries;
  std::shared_ptr<core::GeometryContainer> output_geometry;
  bool success = false;
  std::string error_message;
};

/**
 * @brief Engine for executing node graphs
 */
class ExecutionEngine {
public:
  using ProgressCallback =
      std::function<void(int completed_nodes, int total_nodes)>;
  using ErrorCallback =
      std::function<void(const std::string &error, int node_id)>;

  ExecutionEngine() = default;
  ~ExecutionEngine() = default;

  /**
   * @brief Execute entire graph
   * @param graph The graph to execute
   * @return True if execution completed successfully
   */
  bool execute_graph(NodeGraph &graph);

  /**
   * @brief Get cached geometry for a node
   * @param node_id Node ID to get result for
   * @return Cached geometry or nullptr if not available
   */
  std::shared_ptr<core::GeometryContainer> get_node_geometry(int node_id) const;

  /**
   * @brief Clear execution cache
   */
  void clear_cache();

  /**
   * @brief Invalidate cache for a specific node and all downstream nodes
   * Call this when a node's parameters change
   * @param graph The node graph
   * @param node_id Node ID that was modified
   */
  void invalidate_node(NodeGraph &graph, int node_id);

  /**
   * @brief Set progress callback
   */
  void set_progress_callback(ProgressCallback callback) {
    progress_callback_ = std::move(callback);
  }

  /**
   * @brief Set error callback
   */
  void set_error_callback(ErrorCallback callback) {
    error_callback_ = std::move(callback);
  }

private:
  // Result cache: node_id -> geometry_container (new system)
  std::unordered_map<int, std::shared_ptr<core::GeometryContainer>>
      geometry_cache_;

  // Callbacks
  ProgressCallback progress_callback_;
  ErrorCallback error_callback_;

  // Helper methods
  std::vector<std::shared_ptr<core::GeometryContainer>>
  gather_input_geometries(const NodeGraph &graph, int node_id);
  void notify_progress(int completed, int total);
  void notify_error(const std::string &error, int node_id);

  /**
   * @brief Transfer parameters from GraphNode to SOPNode
   * Automatically converts parameter types and values
   */
  void transfer_parameters(const GraphNode &graph_node, sop::SOPNode &sop_node);
};

} // namespace nodo::graph
