#include "nodo/sop/boolean_sop.hpp"

#include "nodo/core/geometry_container.hpp"
#include "nodo/geometry/boolean_ops.hpp"

#include <iostream>

namespace nodo::sop {

BooleanSOP::BooleanSOP(const std::string& node_name) : SOPNode(node_name, "Boolean") {
  // Add input ports (using numeric names for execution engine compatibility)
  input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);
  input_ports_.add_port("1", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_int_parameter("operation", 0)
                         .label("Operation")
                         .options({"Union", "Intersection", "Difference"})
                         .category("Boolean")
                         .description("Boolean operation: Union (A+B), "
                                      "Intersection (A∩B), or Difference (A-B)")
                         .build());
}

core::Result<std::shared_ptr<core::GeometryContainer>> BooleanSOP::execute() {
  // Get input geometries (using numeric port names)
  auto input_a = get_input_data(0); // Port "0"
  auto input_b = get_input_data(1); // Port "1"

  if (!input_a) {
    set_error("Missing input geometry A (port 0 not connected)");
    return {(std::string) "Missing input geometry A (port 0 not connected)"};
  }

  if (!input_b) {
    set_error("Missing input geometry B (port 1 not connected)");
    return {(std::string) "Missing input geometry B (port 1 not connected)"};
  }

  // Get operation type parameter
  const int operation = get_parameter<int>("operation", 0);

  // Perform the boolean operation using new GeometryContainer API
  std::optional<core::GeometryContainer> result;

  switch (operation) {
    case 0: // UNION
      result = geometry::BooleanOps::union_geometries(*input_a, *input_b);
      break;
    case 1: // INTERSECTION
      result = geometry::BooleanOps::intersect_geometries(*input_a, *input_b);
      break;
    case 2: // DIFFERENCE
      result = geometry::BooleanOps::difference_geometries(*input_a, *input_b);
      break;
    default:
      set_error("Invalid operation type: " + std::to_string(operation));
      return {(std::string) "Invalid operation type: " + std::to_string(operation)};
  }

  // Check if operation succeeded
  if (!result.has_value()) {
    const auto& error = geometry::BooleanOps::last_error();
    set_error("Boolean operation failed: " + error.message);
    return {(std::string) "Boolean operation failed: " + error.message};
  }

  return std::make_shared<core::GeometryContainer>(std::move(*result));
}
} // namespace nodo::sop
