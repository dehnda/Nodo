/**
 * NodeFlux Engine - Visual Node Graph Editor
 * Modern procedural modeling interface with ImNodes integration
 */

#pragma once

#include <imgui.h>
#include <imnodes.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace nodeflux::core {
class Mesh;
}

namespace nodeflux::ui {

enum class NodeType {
  Sphere,
  Box,
  Cylinder,
  Plane,
  Torus,
  Extrude,
  Smooth,
  Boolean,
  Transform,
  Array,
  Mirror
};

struct GraphNode {
  int id;
  NodeType type;
  std::string name;
  ImVec2 position;
  bool selected;

  // Node parameters (simplified for demo)
  float radius = 1.0f;
  int subdivisions = 3;  // Valid range for icosphere is 0-5
  float distance = 0.5f;
  int iterations = 3;

  // Node state
  std::shared_ptr<nodeflux::core::Mesh> output_mesh;
  bool needs_update = true;
};

struct NodeLink {
  int id;
  int start_pin_id;
  int end_pin_id;

  // Helper to extract node IDs
  int get_start_node_id() const { return start_pin_id / 100; }
  int get_end_node_id() const { return end_pin_id / 100; }
};

/**
 * @brief Visual Node Graph Editor using ImNodes
 *
 * Provides a complete visual interface for creating and editing
 * procedural mesh generation graphs with real-time parameter control.
 */
class NodeGraphEditor {
private:
  std::vector<GraphNode> nodes_;
  std::vector<NodeLink> links_;
  int next_node_id_;
  int next_link_id_;
  bool is_initialized_;

  // Node execution state
  std::unordered_map<int, std::shared_ptr<nodeflux::core::Mesh>> node_cache_;
  bool auto_execute_;

public:
  NodeGraphEditor();
  ~NodeGraphEditor();

  /**
   * @brief Initialize ImNodes context
   * Call this once before using the editor
   */
  void initialize();

  /**
   * @brief Shutdown ImNodes context
   * Call this when done with the editor
   */
  void shutdown();

  /**
   * @brief Render the complete node graph editor
   * This should be called inside an ImGui window
   */
  void render();

  /**
   * @brief Add a new node to the graph
   * @param type Type of node to create
   * @param position Position in the graph space
   * @return ID of the created node
   */
  int add_node(NodeType type, const ImVec2 &position);

  /**
   * @brief Remove a node from the graph
   * @param node_id ID of the node to remove
   */
  void remove_node(int node_id);

  /**
   * @brief Clear all nodes and links
   */
  void clear_graph();

  /**
   * @brief Execute the node graph
   * Process all nodes and generate output meshes
   */
  void execute_graph();

  /**
   * @brief Get the output mesh from a specific node
   * @param node_id ID of the node
   * @return Shared pointer to the output mesh, or nullptr if not available
   */
  std::shared_ptr<nodeflux::core::Mesh> get_node_output(int node_id) const;

  /**
   * @brief Get the first available mesh output from any node
   * @return Shared pointer to the first available mesh, or nullptr if none available
   */
  std::shared_ptr<nodeflux::core::Mesh> get_first_available_mesh() const;

  /**
   * @brief Enable/disable automatic graph execution on parameter changes
   * @param auto_exec Whether to automatically execute on changes
   */
  void set_auto_execute(bool auto_exec) { auto_execute_ = auto_exec; }

  /**
   * @brief Get current node count
   */
  size_t get_node_count() const { return nodes_.size(); }

  /**
   * @brief Get current link count
   */
  size_t get_link_count() const { return links_.size(); }

  // JSON Integration Methods
  /**
   * @brief Serialize current graph to JSON string
   * @return JSON string representation of the graph
   */
  std::string serialize_to_json() const;

  /**
   * @brief Load graph from JSON string
   * @param json_str JSON string to deserialize
   * @return true if successful, false otherwise
   */
  bool load_from_json(const std::string &json_str);

  /**
   * @brief Save current graph to JSON file
   * @param filename Path to save the JSON file
   * @return true if successful, false otherwise
   */
  bool save_to_file(const std::string &filename);

  /**
   * @brief Load graph from JSON file
   * @param filename Path to the JSON file to load
   * @return true if successful, false otherwise
   */
  bool load_from_file(const std::string &filename);

private:
  /**
   * @brief Render a single node
   */
  void render_node(GraphNode &node);

  /**
   * @brief Render node parameters panel
   */
  void render_node_parameters(GraphNode &node);

  /**
   * @brief Get node type name as string
   */
  std::string get_node_type_name(NodeType type) const;

  /**
   * @brief Generate input/output pin IDs for a node
   */
  int get_input_pin_id(int node_id) const { return node_id * 100 + 1; }
  int get_output_pin_id(int node_id) const { return node_id * 100 + 2; }

  /**
   * @brief Execute a specific node
   */
  void execute_node(GraphNode &node);

  /**
   * @brief Get input mesh for a node (from connected nodes)
   */
  std::shared_ptr<nodeflux::core::Mesh> get_input_mesh(int node_id);

  /**
   * @brief Handle node graph interactions
   */
  void handle_interactions();
};

} // namespace nodeflux::ui
