#include "nodo/core/attribute_group.hpp"
#include "nodo/processing/subdivision.hpp"
#include "nodo/sop/subdivisions_sop.hpp"

namespace nodo::sop {

SubdivisionSOP::SubdivisionSOP(const std::string& name) : SOPNode(name, "Subdivide") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

  // Subdivision algorithm type
  register_parameter(define_int_parameter("subdivision_type", 0)
                         .label("Algorithm")
                         .options({"Catmull-Clark", "Loop", "Quad-Tri"})
                         .category("Subdivision")
                         .description("Catmull-Clark = quads, Loop = triangles, Quad-Tri = mixed")
                         .build());

  // Number of subdivision levels
  register_parameter(define_int_parameter("subdivision_levels", 1)
                         .label("Levels")
                         .range(1, 5)
                         .category("Subdivision")
                         .description("Number of subdivision iterations (each doubles face count)")
                         .build());
}

core::Result<std::shared_ptr<core::GeometryContainer>> SubdivisionSOP::execute() {
  // Apply group filter if specified (keeps only grouped primitives)
  auto filter_result = apply_group_filter(0, core::ElementClass::PRIMITIVE, false);
  if (!filter_result.is_success()) {
    return {filter_result.error().value()};
  }
  const auto& input = filter_result.get_value();
  // Get subdivision parameters
  int subdivision_type = get_parameter<int>("subdivision_type", 0);
  int subdivision_levels = get_parameter<int>("subdivision_levels", 1);

  // Get parameters
  processing::SubdivisionParams params;

  switch (subdivision_type) {
    case 0:
      params.type = processing::SubdivisionType::CATMULL_CLARK;
      break;
    case 1:
      params.type = processing::SubdivisionType::LOOP;
      break;
    case 2:
      params.type = processing::SubdivisionType::QUAD_TRI;
      break;
    default:
      params.type = processing::SubdivisionType::CATMULL_CLARK;
  }

  params.levels = subdivision_levels;

  // Perform subdivision
  std::string error;
  auto result = processing::Subdivision::subdivide(*input, params, &error);

  if (!result) {
    return {(std::string)error};
  }

  return std::make_shared<core::GeometryContainer>(std::move(*result));
}

} // namespace nodo::sop
