#include "nodeflux/sop/array_sop.hpp"
#include "nodeflux/core/math.hpp"
#include "nodeflux/core/types.hpp"

#include <cmath>
#include <iostream>

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::sop {

ArraySOP::ArraySOP(const std::string &name) : SOPNode(name, "Array") {
  // Add input port
  input_ports_.add_port("mesh", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_int_parameter("array_type", 0)
                         .label("Array Type")
                         .options({"Linear", "Radial", "Grid"})
                         .category("Array")
                         .build());

  register_parameter(define_int_parameter("count", 3)
                         .label("Count")
                         .range(1, 100)
                         .category("Array")
                         .build());

  // Linear array parameters
  register_parameter(define_float_parameter("linear_offset_x", 1.0F)
                         .label("Offset X")
                         .range(-100.0, 100.0)
                         .category("Linear")
                         .build());

  register_parameter(define_float_parameter("linear_offset_y", 0.0F)
                         .label("Offset Y")
                         .range(-100.0, 100.0)
                         .category("Linear")
                         .build());

  register_parameter(define_float_parameter("linear_offset_z", 0.0F)
                         .label("Offset Z")
                         .range(-100.0, 100.0)
                         .category("Linear")
                         .build());

  // Radial array parameters
  register_parameter(define_float_parameter("radial_center_x", 0.0F)
                         .label("Center X")
                         .range(-100.0, 100.0)
                         .category("Radial")
                         .build());

  register_parameter(define_float_parameter("radial_center_y", 0.0F)
                         .label("Center Y")
                         .range(-100.0, 100.0)
                         .category("Radial")
                         .build());

  register_parameter(define_float_parameter("radial_center_z", 0.0F)
                         .label("Center Z")
                         .range(-100.0, 100.0)
                         .category("Radial")
                         .build());

  register_parameter(define_float_parameter("radial_radius", 2.0F)
                         .label("Radius")
                         .range(0.0, 100.0)
                         .category("Radial")
                         .build());

  register_parameter(define_float_parameter("angle_step", 60.0F)
                         .label("Angle Step")
                         .range(0.0, 360.0)
                         .category("Radial")
                         .build());

  // Grid array parameters
  register_parameter(define_int_parameter("grid_width", 3)
                         .label("Grid Width")
                         .range(1, 50)
                         .category("Grid")
                         .build());

  register_parameter(define_int_parameter("grid_height", 3)
                         .label("Grid Height")
                         .range(1, 50)
                         .category("Grid")
                         .build());

  register_parameter(define_float_parameter("grid_spacing_x", 1.0F)
                         .label("Spacing X")
                         .range(0.01, 100.0)
                         .category("Grid")
                         .build());

  register_parameter(define_float_parameter("grid_spacing_y", 1.0F)
                         .label("Spacing Y")
                         .range(0.01, 100.0)
                         .category("Grid")
                         .build());
}

std::shared_ptr<core::GeometryContainer> ArraySOP::execute() {
  // Get input geometry (already GeometryContainer)
  auto input_geo = get_input_data("mesh");

  if (!input_geo) {
    set_error("No input geometry connected");
    return nullptr;
  }

  if (input_geo->topology().point_count() == 0) {
    set_error("Input geometry is empty");
    return nullptr;
  }

  // Read parameters
  const auto array_type =
      static_cast<ArrayType>(get_parameter<int>("array_type", 0));
  const int count = get_parameter<int>("count", 3);

  // Execute the appropriate array operation
  std::unique_ptr<core::GeometryContainer> result;

  switch (array_type) {
  case ArrayType::LINEAR:
    result = create_linear_array(*input_geo, count);
    break;
  case ArrayType::RADIAL:
    result = create_radial_array(*input_geo, count);
    break;
  case ArrayType::GRID: {
    const int grid_width = get_parameter<int>("grid_width", 3);
    const int grid_height = get_parameter<int>("grid_height", 3);
    result = create_grid_array(*input_geo, grid_width, grid_height);
    break;
  }
  default:
    set_error("Unknown array type");
    return nullptr;
  }

  // Check if operation succeeded
  if (!result) {
    set_error("Array operation failed");
    return nullptr;
  }

  return std::shared_ptr<core::GeometryContainer>(std::move(result));
}

std::unique_ptr<core::GeometryContainer>
ArraySOP::create_linear_array(const core::GeometryContainer &input_geo,
                              int count) {
  // Get input positions
  auto *input_positions =
      input_geo.get_point_attribute_typed<Eigen::Vector3f>("P");
  if (!input_positions) {
    return nullptr;
  }

  const size_t input_point_count = input_geo.topology().point_count();
  const size_t input_prim_count = input_geo.topology().primitive_count();

  // Read linear offset parameters
  const Eigen::Vector3f linear_offset(
      get_parameter<float>("linear_offset_x", 1.0F),
      get_parameter<float>("linear_offset_y", 0.0F),
      get_parameter<float>("linear_offset_z", 0.0F));

  // Create output container
  auto result = std::make_unique<core::GeometryContainer>();
  const size_t total_points = input_point_count * count;

  result->set_point_count(total_points);

  // Add position attribute
  result->add_point_attribute("P", core::AttributeType::VEC3F);
  auto *output_positions =
      result->get_point_attribute_typed<Eigen::Vector3f>("P");

  // Copy and transform points for each array element
  for (int i = 0; i < count; ++i) {
    Eigen::Vector3f offset = linear_offset * static_cast<float>(i);
    size_t point_offset = i * input_point_count;

    auto input_span = input_positions->values();
    auto output_span = output_positions->values_writable();

    for (size_t p = 0; p < input_point_count; ++p) {
      output_span[point_offset + p] = input_span[p] + offset;
    }
  }

  // Copy topology for each array element
  for (int i = 0; i < count; ++i) {
    int vertex_offset = i * static_cast<int>(input_point_count);

    for (size_t prim_idx = 0; prim_idx < input_prim_count; ++prim_idx) {
      const auto &prim_verts =
          input_geo.topology().get_primitive_vertices(prim_idx);
      std::vector<int> new_prim_verts;

      for (int point_idx : prim_verts) {
        new_prim_verts.push_back(point_idx + vertex_offset);
      }

      result->add_primitive(new_prim_verts);
    }
  }

  return result;
}

std::unique_ptr<core::GeometryContainer>
ArraySOP::create_radial_array(const core::GeometryContainer &input_geo,
                              int count) {
  // TODO: Implement radial array with GeometryContainer
  // For now, return a clone of input
  auto result = std::make_unique<core::GeometryContainer>(input_geo.clone());
  return result;
}

std::unique_ptr<core::GeometryContainer>
ArraySOP::create_grid_array(const core::GeometryContainer &input_geo,
                            int grid_width, int grid_height) {
  // TODO: Implement grid array with GeometryContainer
  // For now, return a clone of input
  auto result = std::make_unique<core::GeometryContainer>(input_geo.clone());
  return result;
}

} // namespace nodeflux::sop
