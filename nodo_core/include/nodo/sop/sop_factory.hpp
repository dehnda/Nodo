#pragma once

#include "nodo/graph/node_graph.hpp"
#include "nodo/sop/sop_node.hpp"

#include <memory>
#include <string>
#include <vector>

namespace nodo::sop {

/**
 * @brief Metadata for a node type
 *
 * Contains display information for UI systems
 */
struct NodeMetadata {
  graph::NodeType type;    // Backend node type enum
  std::string name;        // Display name (e.g., "Sphere", "Boolean")
  std::string category;    // Category (e.g., "Generator", "Modifier", "Array")
  std::string description; // Short description for tooltips
};

/**
 * @brief Factory for creating SOP nodes by type
 *
 * This factory allows GraphNode to query parameter schemas
 * from SOPs without executing them.
 */
class SOPFactory {
public:
  /**
   * @brief Create a SOP instance by NodeType
   * @param type The node type to create
   * @param name Node name
   * @return Shared pointer to created SOP, or nullptr if type not supported
   */
  static std::shared_ptr<SOPNode> create(graph::NodeType type,
                                         const std::string& name = "node");

  /**
   * @brief Get parameter definitions for a node type without instantiating
   * @param type The node type to query
   * @return Vector of parameter definitions
   */
  static std::vector<SOPNode::ParameterDefinition>
  get_parameter_schema(graph::NodeType type);

  /**
   * @brief Check if a node type is supported by SOP system
   * @param type The node type to check
   * @return True if type has a corresponding SOP class
   */
  static bool is_sop_supported(graph::NodeType type);

  /**
   * @brief Get metadata for all available nodes
   * @return Vector of node metadata for UI display
   *
   * Returns a complete list of all nodes that can be created,
   * with their display names, categories, and descriptions.
   * This is the single source of truth for node discovery.
   */
  static std::vector<NodeMetadata> get_all_available_nodes();

  /**
   * @brief Get the number of required inputs for a node type
   * @param type The node type to query
   * @return Number of required inputs (0 for generators)
   * @deprecated Use get_input_config() instead
   */
  static int get_min_inputs(graph::NodeType type);

  /**
   * @brief Get the maximum number of inputs for a node type
   * @param type The node type to query
   * @return Maximum number of inputs (-1 for unlimited)
   * @deprecated Use get_input_config() instead
   */
  static int get_max_inputs(graph::NodeType type);

  /**
   * @brief Get input configuration for a node type
   * @param type The node type to query
   * @return InputConfig with input handling details
   */
  static SOPNode::InputConfig get_input_config(graph::NodeType type);

  /**
   * @brief Get display name for a node type
   * @param type The node type to query
   * @return Display name (e.g., "Sphere", "Copy to Points")
   */
  static std::string get_display_name(graph::NodeType type);
};

} // namespace nodo::sop
