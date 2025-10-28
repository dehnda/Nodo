#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief Switch SOP node - selects one of multiple inputs based on index
 */
class SwitchSOP : public SOPNode {
private:
  static constexpr int DEFAULT_INDEX = 0;

public:
  explicit SwitchSOP(const std::string &node_name = "switch")
      : SOPNode(node_name, "SwitchSOP") {
    set_parameter("index", DEFAULT_INDEX);
  }

  void set_index(int index) { set_parameter("index", index); }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    const int index = get_parameter<int>("index", DEFAULT_INDEX);

    // Collect all connected inputs
    std::vector<std::shared_ptr<core::GeometryContainer>> inputs;

    // Check each input port (try indices 0-9, typical max for switch)
    for (int i = 0; i < 10; ++i) {
      auto input = get_input_data(i);
      if (input) {
        inputs.push_back(input);
      }
    }

    if (inputs.empty()) {
      set_error("Switch has no inputs");
      return nullptr;
    }

    // Check if index is valid
    if (index < 0 || index >= static_cast<int>(inputs.size())) {
      set_error("Switch index " + std::to_string(index) + " out of range [0, " +
                std::to_string(inputs.size() - 1) + "]");
      return nullptr;
    }

    // Return the selected input
    return inputs[index];
  }
};

} // namespace nodo::sop
