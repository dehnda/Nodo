#include "nodo/sop/sop_factory.hpp"

#include "nodo/sop/node_registry.hpp"

namespace nodo::sop {
// Forward declare the registration function
void registerAllNodes();

// Ensure nodes are registered exactly once
static void ensureNodesRegistered() {
  static bool initialized = false;
  if (!initialized) {
    registerAllNodes();
    initialized = true;
  }
}

} // namespace nodo::sop

namespace nodo::sop {

using namespace graph;

std::shared_ptr<SOPNode> SOPFactory::create(NodeType type, const std::string& name) {
  ensureNodesRegistered();
  return NodeRegistry::instance().create(type, name);
}

std::vector<SOPNode::ParameterDefinition> SOPFactory::get_parameter_schema(NodeType type) {
  ensureNodesRegistered();
  // Create temporary instance to query schema
  auto sop = create(type, "temp");
  if (sop) {
    return sop->get_parameter_definitions();
  }

  // Fallback: return empty for unsupported types
  return {};
}

bool SOPFactory::is_sop_supported(NodeType type) {
  ensureNodesRegistered();
  return create(type, "test") != nullptr;
}

std::vector<NodeMetadata> SOPFactory::get_all_available_nodes() {
  ensureNodesRegistered(); // CRITICAL: Register before returning list!
  return NodeRegistry::instance().getAllNodes();
}

int SOPFactory::get_min_inputs(graph::NodeType type) {
  // Create a temporary instance to query its input requirements
  auto sop = create(type);
  if (sop) {
    return sop->get_input_config().min_count;
  }
  return 1; // Default fallback
}

int SOPFactory::get_max_inputs(graph::NodeType type) {
  // Create a temporary instance to query its input requirements
  auto sop = create(type);
  if (sop) {
    return sop->get_input_config().max_count;
  }
  return 1; // Default fallback
}

SOPNode::InputConfig SOPFactory::get_input_config(graph::NodeType type) {
  // Create a temporary instance to query its input configuration
  auto sop = create(type);
  if (sop) {
    return sop->get_input_config();
  }
  // Default fallback: single input modifier
  return SOPNode::InputConfig(SOPNode::InputType::SINGLE, 1, 1, 1);
}

std::string SOPFactory::get_display_name(graph::NodeType type) {
  // Get display name directly from registry metadata
  // This avoids creating a temporary SOP with an empty name
  ensureNodesRegistered();
  const auto& registry = NodeRegistry::instance();
  if (registry.isRegistered(type)) {
    const auto& all_nodes = registry.getAllNodes();
    for (const auto& metadata : all_nodes) {
      if (metadata.type == type) {
        return metadata.name;
      }
    }
  }

  // Fallback for non-SOP nodes
  return "Unknown";
}

} // namespace nodo::sop
