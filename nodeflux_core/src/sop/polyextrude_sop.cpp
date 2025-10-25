#include "nodeflux/sop/polyextrude_sop.hpp"
#include "nodeflux/core/math.hpp"
#include <Eigen/Dense>
#include <iostream>
#include <map>

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::sop {

PolyExtrudeSOP::PolyExtrudeSOP(const std::string &name)
    : SOPNode(name, "PolyExtrude") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_float_parameter("distance", 1.0F)
                         .label("Distance")
                         .range(0.0, 10.0)
                         .category("Extrusion")
                         .build());

  register_parameter(define_float_parameter("inset", 0.0F)
                         .label("Inset")
                         .range(0.0, 1.0)
                         .category("Extrusion")
                         .build());

  register_parameter(define_int_parameter("individual_faces", 1)
                         .label("Individual Faces")
                         .range(0, 1)
                         .category("Extrusion")
                         .build());
}

std::shared_ptr<core::GeometryContainer> PolyExtrudeSOP::execute() {
  // TODO: Implement poly extrude with GeometryContainer
  auto input = get_input_data(0);
  if (!input) {
    return nullptr;
  }
  return std::make_shared<core::GeometryContainer>(input->clone());
}
// Get input geometry
} // namespace nodeflux::sop
