#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief Switch SOP node - selects one of multiple inputs based on index
 */
class SwitchSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit SwitchSOP(const std::string& node_name = "switch")
      : SOPNode(node_name, "Switch") {
    // Multiple geometry inputs (up to 10)
    for (int i = 0; i < 10; ++i) {
      input_ports_.add_port(std::to_string(i), NodePort::Type::INPUT,
                            NodePort::DataType::GEOMETRY, this);
    }

    // Index parameter
    register_parameter(define_int_parameter("index", 0)
                           .label("Select Input")
                           .range(0, 9)
                           .category("Switch")
                           .description("Index of input to pass through (0-9)")
                           .build());
  }

  // Multi-input node - requires at least 1 input, accepts up to 10
  InputConfig get_input_config() const override {
    return InputConfig(InputType::MULTI_FIXED, 1, 10, 2);
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    const int index = get_parameter<int>("index", 0);

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
