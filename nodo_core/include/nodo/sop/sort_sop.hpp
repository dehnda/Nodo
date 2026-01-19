#pragma once

#include "sop_node.hpp"

#include <memory>

namespace nodo::sop {

/**
 * @brief Reorder points or primitives
 *
 * TODO: Full implementation requires attribute type introspection system.
 * Current version is a simplified pass-through placeholder.
 *
 * Sorts geometry elements by various criteria:
 * - By X/Y/Z position
 * - By attribute value
 * - Random shuffle
 * - Reverse order
 */
class SortSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit SortSOP(const std::string& name = "sort") : SOPNode(name, "Sort") {
    input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

    // Element class to sort
    register_parameter(define_int_parameter("class", 0)
                           .label("Sort")
                           .options({"Points", "Primitives"})
                           .category("Sort")
                           .description("Type of elements to sort")
                           .build());

    // Sort key
    register_parameter(define_int_parameter("key", 0)
                           .label("Sort By")
                           .options({"X Position", "Y Position", "Z Position", "Reverse", "Random", "Attribute"})
                           .category("Sort")
                           .description("Criteria for sorting elements")
                           .build());

    // Attribute name for attribute-based sorting
    register_parameter(define_string_parameter("attribute", "")
                           .label("Attribute")
                           .category("Sort")
                           .visible_when("key", 5)
                           .description("Attribute name to sort by")
                           .build());

    // Sort order (visible for position and attribute modes)
    register_parameter(define_int_parameter("order", 0)
                           .label("Order")
                           .options({"Ascending", "Descending"})
                           .category("Sort")
                           .description("Sort direction (ascending or descending)")
                           .build());

    // Random seed
    register_parameter(define_int_parameter("seed", 0)
                           .label("Seed")
                           .range(0, 10000)
                           .category("Random")
                           .visible_when("key", 4)
                           .description("Random seed for shuffle mode")
                           .build());
  }

  ~SortSOP() override = default;

protected:
  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override {
    auto input = get_input_data(0);
    if (!input) {
      set_error("SortSOP requires input geometry");
      return {(std::string) "SortSOP requires input geometry"};
    }

    // TODO: Implement full sorting with proper attribute handling
    // Requires attribute type introspection system to properly rebuild geometry
    return std::make_shared<core::GeometryContainer>(input->clone());
  }
};

} // namespace nodo::sop
