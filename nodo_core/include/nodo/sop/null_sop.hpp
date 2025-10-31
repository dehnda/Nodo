#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief Null SOP node - pass-through node for organization
 *
 * The Null node simply passes through its input geometry unchanged.
 * Useful for:
 * - Organizing node networks
 * - Creating named reference points
 * - Merging multiple branches without modification
 */
class NullSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit NullSOP(const std::string &node_name = "null")
      : SOPNode(node_name, "Null") {
    // Single geometry input
    input_ports_.add_port("0", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    // Simply return the input unchanged
    auto input = get_input_data(0);
    if (!input) {
      set_error("Null node requires input geometry");
      return nullptr;
    }

    // Return input directly (no copy needed)
    return input;
  }
};

} // namespace nodo::sop
