#pragma once

#include "nodo/graph/node_graph.hpp"
#include "nodo/sop/sop_factory.hpp"

#include <memory>

namespace nodo::sop {

class NodeRegistry {
public:
  static NodeRegistry& instance();

  void registerNode(graph::NodeType type, const NodeMetadata& metadata);
  [[nodiscard]] std::shared_ptr<SOPNode> create(graph::NodeType type, const std::string& name) const;
  [[nodiscard]] bool isRegistered(graph::NodeType type) const;
  [[nodiscard]] std::vector<NodeMetadata> getAllNodes() const;
  [[nodiscard]] std::vector<NodeMetadata> getNodesByCategory(const std::string& category) const;

private:
  std::unordered_map<graph::NodeType, NodeMetadata> registry_;
};
} // namespace nodo::sop
