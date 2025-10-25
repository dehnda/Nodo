#include "nodeflux/sop/extrude_sop.hpp"
#include "nodeflux/core/math.hpp"
#include <iostream>

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::sop {

ExtrudeSOP::ExtrudeSOP(const std::string &name) : SOPNode(name, "Extrude") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_float_parameter("distance", 1.0F)
                         .label("Distance")
                         .range(0.0, 10.0)
                         .category("Extrusion")
                         .build());

  register_parameter(
      define_int_parameter("mode", 0)
          .label("Mode")
          .options({"Face Normals", "Uniform Direction", "Region Normals"})
          .category("Extrusion")
          .build());

  register_parameter(define_float_parameter("inset", 0.0F)
                         .label("Inset")
                         .range(0.0, 5.0)
                         .category("Extrusion")
                         .build());

  // Direction vector (for uniform mode)
  register_parameter(define_float_parameter("direction_x", 0.0F)
                         .label("Direction X")
                         .range(-1.0, 1.0)
                         .category("Direction")
                         .build());

  register_parameter(define_float_parameter("direction_y", 0.0F)
                         .label("Direction Y")
                         .range(-1.0, 1.0)
                         .category("Direction")
                         .build());

  register_parameter(define_float_parameter("direction_z", 1.0F)
                         .label("Direction Z")
                         .range(-1.0, 1.0)
                         .category("Direction")
                         .build());
}

std::shared_ptr<core::GeometryContainer> ExtrudeSOP::execute() {
  // TODO: Implement extrude with GeometryContainer
  auto input = get_input_data(0);
  if (!input) {
    return nullptr;
  }
  return std::make_shared<core::GeometryContainer>(input->clone());
}
// Sync member variables from parameter system
} // namespace nodeflux::sop
