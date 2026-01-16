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

std::shared_ptr<core::GeometryContainer> SubdivisionSOP::execute() {
  // Get input geometry
  auto input = get_input_data(0);

  if (!input) {
    set_error("No input geometry");
    return nullptr;
  }

  // Get parameters
  processing::SubdivisionParams params;

  int type_index = get_parameter<int>("subdivision_type", 0);
  switch (type_index) {
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

  params.levels = get_parameter<int>("subdivision_levels", 1);

  // Perform subdivision
  std::string error;
  auto result = processing::Subdivision::subdivide(*input, params, &error);

  if (!result) {
    set_error(error);
    return nullptr;
  }

  return std::make_shared<core::GeometryContainer>(std::move(*result));
}

} // namespace nodo::sop
