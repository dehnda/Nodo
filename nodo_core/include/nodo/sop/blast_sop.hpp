#pragma once

#include "sop_node.hpp"
#include <memory>

namespace nodo::sop {

/**
 * @brief Delete geometry elements by group membership
 *
 * Removes points or primitives that are in a specified group.
 * Similar to Delete node but group-focused with simpler interface.
 */
class BlastSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit BlastSOP(const std::string &name = "blast")
      : SOPNode(name, "Blast") {
    input_ports_.add_port("0", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);

    // Group name to delete
    register_parameter(
        define_string_parameter("group", "")
            .label("Group")
            .category("Group")
            .description("Name of group to delete (leave empty for all)")
            .build());

    // Element class
    register_parameter(define_int_parameter("class", 0)
                           .label("Delete")
                           .options({"Points", "Primitives"})
                           .category("Group")
                           .description("Type of elements to delete")
                           .build());

    // Delete or keep group
    register_parameter(
        define_int_parameter("negate", 0)
            .label("Delete Non-Selected")
            .category("Options")
            .description("Delete elements NOT in the group instead")
            .build());
  }

  ~BlastSOP() override = default;

protected:
  std::shared_ptr<core::GeometryContainer> execute() override;
};

} // namespace nodo::sop
