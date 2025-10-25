#include "../../include/nodeflux/sop/copy_to_points_sop.hpp"
#include "../../include/nodeflux/core/math.hpp"

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::sop {

// Constants for copy operations
constexpr float DEFAULT_SCALE_MULTIPLIER = 0.1F;
constexpr double UP_VECTOR_Y = 1.0;

CopyToPointsSOP::CopyToPointsSOP(const std::string &node_name)
    : SOPNode(node_name, "CopyToPoints") {
  // Add input ports
  input_ports_.add_port("points", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);
  input_ports_.add_port("template", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_int_parameter("use_point_normals", 1)
                         .label("Use Point Normals")
                         .range(0, 1)
                         .category("Copy")
                         .build());

  register_parameter(define_int_parameter("use_point_scale", 1)
                         .label("Use Point Scale")
                         .range(0, 1)
                         .category("Copy")
                         .build());

  register_parameter(define_float_parameter("uniform_scale", 1.0F)
                         .label("Uniform Scale")
                         .range(0.01, 10.0)
                         .category("Copy")
                         .build());

  register_parameter(define_string_parameter("scale_attribute", "point_index")
                         .label("Scale Attribute")
                         .category("Copy")
                         .build());

  register_parameter(define_string_parameter("rotation_attribute", "")
                         .label("Rotation Attribute")
                         .category("Copy")
                         .build());
}

std::shared_ptr<core::GeometryContainer> CopyToPointsSOP::execute() {
  // TODO: Implement copy to points with GeometryContainer
  // For now, return empty container as stub
  auto input = get_input_data("template");
  if (!input) {
    return std::make_shared<core::GeometryContainer>();
  }
  return std::make_shared<core::GeometryContainer>(input->clone());
}

} // namespace nodeflux::sop
