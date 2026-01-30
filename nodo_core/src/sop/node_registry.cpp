#include <iostream>

#include <nodo/sop/node_registry.hpp>

namespace nodo::sop {

NodeRegistry& NodeRegistry::instance() {
  static NodeRegistry instance;
  return instance;
}

void NodeRegistry::registerNode(graph::NodeType type, const NodeMetadata& metadata) {
  registry_[type] = metadata;
}

std::shared_ptr<SOPNode> NodeRegistry::create(graph::NodeType type, const std::string& name) const {
  auto iter = registry_.find(type);
  if (iter != registry_.end()) {
    return iter->second.factory(name);
  }
  return nullptr;
}

bool NodeRegistry::isRegistered(graph::NodeType type) const {
  return registry_.contains(type);
}

std::vector<NodeMetadata> NodeRegistry::getAllNodes() const {
  std::vector<NodeMetadata> nodes;
  nodes.reserve(registry_.size());
  for (const auto& [type, metadata] : registry_) {
    nodes.push_back(metadata);
  }
  return nodes;
}

std::vector<NodeMetadata> NodeRegistry::getNodesByCategory(const std::string& category) const {
  std::vector<NodeMetadata> nodes;
  for (const auto& [type, metadata] : registry_) {
    if (metadata.category == category) {
      nodes.push_back(metadata);
    }
  }
  return nodes;
}

} // namespace nodo::sop
