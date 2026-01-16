#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief Output SOP node - marks geometry as a named output
 *
 * The Output node serves as a semantic marker to identify important
 * outputs in the node network. It doesn't modify geometry, but provides
 * metadata that can be queried by:
 * - Render systems (which nodes to render?)
 * - Display systems (which nodes to show in viewport?)
 * - Export systems (which nodes to include in batch export?)
 *
 * Think of it like naming a layer or marking a node as "final output"
 */
class OutputSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit OutputSOP(const std::string& node_name = "output") : SOPNode(node_name, "Output") {
    // Single geometry input
    input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

    // Output name
    register_parameter(define_string_parameter("output_name", "output1")
                           .label("Output Name")
                           .category("Output")
                           .description("Name identifier for this output")
                           .build());

    // Render flag
    register_parameter(define_int_parameter("render", 1)
                           .label("Render")
                           .options({"Off", "On"})
                           .category("Output")
                           .description("Include this output in rendering")
                           .build());

    // Display flag
    register_parameter(define_int_parameter("display", 1)
                           .label("Display")
                           .options({"Off", "On"})
                           .category("Output")
                           .description("Show this output in the viewport")
                           .build());
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    // Simply pass through the input unchanged
    auto input = get_input_data(0);
    if (!input) {
      set_error("Output node requires input geometry");
      return nullptr;
    }

    // In the future, we could store metadata on the geometry container
    // to mark it as a named output, but for now just pass through
    return input;
  }
};

} // namespace nodo::sop
