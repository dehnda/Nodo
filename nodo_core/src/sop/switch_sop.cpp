#include "nodo/sop/switch_sop.hpp"

namespace nodo::sop {

SwitchSOP::SwitchSOP(const std::string& node_name) : SOPNode(node_name, "Switch") {
  // Multiple geometry inputs (up to MAX_INPUTS)
  for (int i = 0; i < MAX_INPUTS; ++i) {
    input_ports_.add_port(std::to_string(i), NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);
  }

  // Index parameter
  register_parameter(define_int_parameter("index", 0)
                         .label("Select Input")
                         .range(0, MAX_INPUTS - 1)
                         .category("Switch")
                         .description("Index of input to pass through (0-9)")
                         .build());
}

core::Result<std::shared_ptr<core::GeometryContainer>> SwitchSOP::execute() {
  const int index = get_parameter<int>("index", 0);

  // Collect all connected inputs
  std::vector<std::shared_ptr<core::GeometryContainer>> inputs;

  // Check each input port (try indices 0 to MAX_INPUTS-1)
  for (int i = 0; i < MAX_INPUTS; ++i) {
    auto input = get_input_data(i);
    if (input) {
      inputs.push_back(input);
    }
  }

  if (inputs.empty()) {
    return {"Switch has no inputs"};
  }

  // Check if index is valid
  if (index < 0 || index >= static_cast<int>(inputs.size())) {
    return {"Switch index " + std::to_string(index) + " out of range [0, " + std::to_string(inputs.size() - 1) + "]"};
  }

  // Return the selected input
  return inputs[index];
}

} // namespace nodo::sop
