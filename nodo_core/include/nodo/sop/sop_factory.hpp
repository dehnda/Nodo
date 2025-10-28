#pragma once

#include "nodo/graph/node_graph.hpp"
#include "nodo/sop/sop_node.hpp"
#include <memory>
#include <string>

namespace nodo::sop {

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
};

} // namespace nodo::sop
