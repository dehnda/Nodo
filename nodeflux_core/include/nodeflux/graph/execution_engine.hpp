/**
 * NodeFlux Engine - Graph Execution Engine
 * Executes node graphs in correct dependency order
 */

#pragma once

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/graph/node_graph.hpp"
#include <functional>
#include <memory>
#include <unordered_map>

// Forward declarations
namespace nodeflux::sop {
class SOPNode;
}

namespace nodeflux::graph {

/**
 * @brief Execution context for a single node
 */
struct ExecutionContext {
  GraphNode *node;
  std::vector<std::shared_ptr<core::Mesh>> input_meshes;
  std::shared_ptr<core::Mesh> output_mesh;
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
   * @brief Execute single node
   * @param node The node to execute
   * @param input_meshes Input meshes for the node
   * @return Output mesh or nullptr if failed
   */
  std::shared_ptr<core::Mesh>
  execute_node(GraphNode &node,
               const std::vector<std::shared_ptr<core::Mesh>> &input_meshes);

  /**
   * @brief Get cached result for a node
   * @param node_id Node ID to get result for
   * @return Cached mesh or nullptr if not available
   */
  std::shared_ptr<core::Mesh> get_node_result(int node_id) const;

  /**
   * @brief Get all cached results
   * @return Map of node_id -> mesh for all cached results
   */
  std::unordered_map<int, std::shared_ptr<core::Mesh>> get_all_results() const;

  /**
   * @brief Clear execution cache
   */
  void clear_cache();

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
  // Result cache: node_id -> mesh
  std::unordered_map<int, std::shared_ptr<core::Mesh>> result_cache_;

  // Callbacks
  ProgressCallback progress_callback_;
  ErrorCallback error_callback_;

  // Node execution methods
  std::shared_ptr<core::Mesh> execute_sphere_node(const GraphNode &node);
  std::shared_ptr<core::Mesh> execute_box_node(const GraphNode &node);
  std::shared_ptr<core::Mesh> execute_cylinder_node(const GraphNode &node);
  std::shared_ptr<core::Mesh> execute_plane_node(const GraphNode &node);
  std::shared_ptr<core::Mesh> execute_torus_node(const GraphNode &node);
  std::shared_ptr<core::Mesh> execute_line_node(const GraphNode &node);
  std::shared_ptr<core::Mesh> execute_transform_node(
      const GraphNode &node,
      const std::vector<std::shared_ptr<core::Mesh>> &inputs);
  std::shared_ptr<core::Mesh>
  execute_extrude_node(const GraphNode &node,
                       const std::vector<std::shared_ptr<core::Mesh>> &inputs);
  std::shared_ptr<core::Mesh> execute_polyextrude_node(
      const GraphNode &node,
      const std::vector<std::shared_ptr<core::Mesh>> &inputs);
  std::shared_ptr<core::Mesh>
  execute_smooth_node(const GraphNode &node,
                      const std::vector<std::shared_ptr<core::Mesh>> &inputs);
  std::shared_ptr<core::Mesh> execute_subdivide_node(
      const GraphNode &node,
      const std::vector<std::shared_ptr<core::Mesh>> &inputs);
  std::shared_ptr<core::Mesh>
  execute_mirror_node(const GraphNode &node,
                      const std::vector<std::shared_ptr<core::Mesh>> &inputs);
  std::shared_ptr<core::Mesh>
  execute_boolean_node(const GraphNode &node,
                       const std::vector<std::shared_ptr<core::Mesh>> &inputs);
  std::shared_ptr<core::Mesh>
  execute_merge_node(const GraphNode &node,
                     const std::vector<std::shared_ptr<core::Mesh>> &inputs);
  std::shared_ptr<core::Mesh>
  execute_array_node(const GraphNode &node,
                     const std::vector<std::shared_ptr<core::Mesh>> &inputs);
  std::shared_ptr<core::Mesh>
  execute_resample_node(const GraphNode &node,
                        const std::vector<std::shared_ptr<core::Mesh>> &inputs);
  std::shared_ptr<core::Mesh>
  execute_scatter_node(const GraphNode &node,
                       const std::vector<std::shared_ptr<core::Mesh>> &inputs);
  std::shared_ptr<core::Mesh> execute_copy_to_points_node(
      const GraphNode &node,
      const std::vector<std::shared_ptr<core::Mesh>> &inputs);
  std::shared_ptr<core::Mesh> execute_noise_displacement_node(
      const GraphNode &node,
      const std::vector<std::shared_ptr<core::Mesh>> &inputs);

  // Helper methods
  std::vector<std::shared_ptr<core::Mesh>>
  gather_input_meshes(const NodeGraph &graph, int node_id);
  void notify_progress(int completed, int total);
  void notify_error(const std::string &error, int node_id);

  /**
   * @brief Transfer parameters from GraphNode to SOPNode
   * Automatically converts parameter types and values
   */
  void transfer_parameters(const GraphNode &graph_node, sop::SOPNode &sop_node);

  /**
   * @brief Generic SOP node execution (for refactored nodes)
   * Creates SOP via factory, transfers parameters, and executes
   */
  std::shared_ptr<core::Mesh>
  execute_sop_node(const GraphNode &node,
                   const std::vector<std::shared_ptr<core::Mesh>> &inputs);
};

} // namespace nodeflux::graph
