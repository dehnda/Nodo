#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

namespace nodeflux::sop {

/**
 * @brief Merge SOP node - combines multiple input geometries
 */
class MergeSOP : public SOPNode {
public:
  explicit MergeSOP(const std::string &node_name = "merge")
      : SOPNode(node_name, "MergeSOP") {
    // Merge can accept multiple inputs
    // The base SOPNode already supports this via input ports
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    // Collect all connected inputs
    std::vector<std::shared_ptr<core::GeometryContainer>> inputs;

    // Check each input port (try indices 0-9, typical max for merge)
    for (int i = 0; i < 10; ++i) {
      auto input = get_input_data(i);
      if (input) {
        inputs.push_back(input);
      }
    }

    if (inputs.empty()) {
      set_error("Merge requires at least one input");
      return nullptr;
    }

    // If only one input, just pass it through
    if (inputs.size() == 1) {
      return inputs[0];
    }

    try {
      // Create output container
      auto result = std::make_shared<core::GeometryContainer>();

      // Merge all inputs
      // TODO: This is a placeholder - actual merging needs to:
      // 1. Combine all points from all inputs
      // 2. Adjust vertex indices in primitives to account for point offset
      // 3. Combine all primitives
      // 4. Merge attributes (with type checking)

      // For now, just return the first input as a placeholder
      // This needs proper implementation
      set_error("Merge SOP not fully implemented yet");
      return inputs[0];

    } catch (const std::exception &exception) {
      set_error("Exception during merge: " + std::string(exception.what()));
      return nullptr;
    }
  }
};

} // namespace nodeflux::sop
