#include "nodo/sop/array_sop.hpp"
#include "nodo/core/math.hpp"
#include "nodo/core/types.hpp"

#include <cmath>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::sop {

ArraySOP::ArraySOP(const std::string &name) : SOPNode(name, "Array") {
  // Add input port with name "mesh" (matches test expectations)
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_int_parameter("array_type", 0)
                         .label("Array Type")
                         .options({"Linear", "Radial", "Grid"})
                         .category("Array")
                         .description("Array pattern (linear, radial, or grid)")
                         .build());

  register_parameter(define_int_parameter("count", 3)
                         .label("Count")
                         .range(1, 100)
                         .category("Array")
                         .description("Number of copies to create")
                         .build());

  // Linear array parameters (visible when array_type == 0)
  register_parameter(define_float_parameter("linear_offset_x", 1.0F)
                         .label("Offset X")
                         .range(-100.0, 100.0)
                         .category("Linear")
                         .visible_when("array_type", 0)
                         .description("Offset between copies along X axis")
                         .build());

  register_parameter(define_float_parameter("linear_offset_y", 0.0F)
                         .label("Offset Y")
                         .range(-100.0, 100.0)
                         .category("Linear")
                         .visible_when("array_type", 0)
                         .description("Offset between copies along Y axis")
                         .build());

  register_parameter(define_float_parameter("linear_offset_z", 0.0F)
                         .label("Offset Z")
                         .range(-100.0, 100.0)
                         .category("Linear")
                         .visible_when("array_type", 0)
                         .description("Offset between copies along Z axis")
                         .build());

  // Radial array parameters (visible when array_type == 1)
  register_parameter(define_float_parameter("radial_center_x", 0.0F)
                         .label("Center X")
                         .range(-100.0, 100.0)
                         .category("Radial")
                         .visible_when("array_type", 1)
                         .description("X coordinate of radial array center")
                         .build());

  register_parameter(define_float_parameter("radial_center_y", 0.0F)
                         .label("Center Y")
                         .range(-100.0, 100.0)
                         .category("Radial")
                         .visible_when("array_type", 1)
                         .description("Y coordinate of radial array center")
                         .build());

  register_parameter(define_float_parameter("radial_center_z", 0.0F)
                         .label("Center Z")
                         .range(-100.0, 100.0)
                         .category("Radial")
                         .visible_when("array_type", 1)
                         .description("Z coordinate of radial array center")
                         .build());

  register_parameter(define_float_parameter("radial_radius", 2.0F)
                         .label("Radius")
                         .range(0.0, 100.0)
                         .category("Radial")
                         .visible_when("array_type", 1)
                         .description("Radius of the circular array")
                         .build());

  register_parameter(
      define_float_parameter("angle_step", 60.0F)
          .label("Angle Step")
          .range(0.0, 360.0)
          .category("Radial")
          .visible_when("array_type", 1)
          .description("Angular spacing between copies in degrees")
          .build());

  // Grid array parameters (visible when array_type == 2)
  register_parameter(define_int_parameter("grid_width", 3)
                         .label("Grid Width")
                         .range(1, 50)
                         .category("Grid")
                         .visible_when("array_type", 2)
                         .description("Number of copies along X axis in grid")
                         .build());

  register_parameter(define_int_parameter("grid_height", 3)
                         .label("Grid Height")
                         .range(1, 50)
                         .category("Grid")
                         .visible_when("array_type", 2)
                         .description("Number of copies along Y axis in grid")
                         .build());

  register_parameter(
      define_float_parameter("grid_spacing_x", 1.0F)
          .label("Spacing X")
          .range(0.01, 100.0)
          .category("Grid")
          .visible_when("array_type", 2)
          .description("Spacing between grid copies along X axis")
          .build());

  register_parameter(
      define_float_parameter("grid_spacing_y", 1.0F)
          .label("Spacing Y")
          .range(0.01, 100.0)
          .category("Grid")
          .visible_when("array_type", 2)
          .description("Spacing between grid copies along Y axis")
          .build());
}

std::shared_ptr<core::GeometryContainer> ArraySOP::execute() {
  // Get input geometry from "mesh" port
  auto input_geo = get_input_data("0");

  if (!input_geo) {
    set_error("No input geometry connected");
    return nullptr;
  }

  const size_t point_count = input_geo->topology().point_count();

  if (point_count == 0) {
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
  const size_t total_vertices = input_geo.topology().vertex_count() * count;

  result->set_point_count(total_points);
  result->set_vertex_count(total_vertices);

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
    int point_offset = i * static_cast<int>(input_point_count);

    for (size_t prim_idx = 0; prim_idx < input_prim_count; ++prim_idx) {
      const auto &prim_verts =
          input_geo.topology().get_primitive_vertices(prim_idx);
      std::vector<int> new_prim_verts;
      new_prim_verts.reserve(prim_verts.size());

      for (int vert_idx : prim_verts) {
        // Get the point that this vertex references
        int input_point = input_geo.topology().get_vertex_point(vert_idx);
        // Add offset to get new point index
        int new_point = input_point + point_offset;

        // Create new vertex for this point
        int new_vert_idx =
            (i * static_cast<int>(input_geo.topology().vertex_count())) +
            vert_idx;
        result->topology().set_vertex_point(new_vert_idx, new_point);
        new_prim_verts.push_back(new_vert_idx);
      }

      result->add_primitive(new_prim_verts);
    }
  }

  return result;
}

std::unique_ptr<core::GeometryContainer>
ArraySOP::create_radial_array(const core::GeometryContainer &input_geo,
                              int count) {
  // Get input positions
  auto *input_positions =
      input_geo.get_point_attribute_typed<Eigen::Vector3f>("P");
  if (input_positions == nullptr) {
    return nullptr;
  }

  const size_t input_point_count = input_geo.topology().point_count();
  const size_t input_prim_count = input_geo.topology().primitive_count();

  // Read radial parameters
  const Eigen::Vector3f center(get_parameter<float>("radial_center_x", 0.0F),
                               get_parameter<float>("radial_center_y", 0.0F),
                               get_parameter<float>("radial_center_z", 0.0F));
  const float radius = get_parameter<float>("radial_radius", 2.0F);
  const float angle_step = get_parameter<float>("angle_step", 60.0F);

  // Create output container
  auto result = std::make_unique<core::GeometryContainer>();
  const size_t total_points = input_point_count * count;
  const size_t total_vertices = input_geo.topology().vertex_count() * count;

  result->set_point_count(total_points);
  result->set_vertex_count(total_vertices);

  // Add position attribute
  result->add_point_attribute("P", core::AttributeType::VEC3F);
  auto *output_positions =
      result->get_point_attribute_typed<Eigen::Vector3f>("P");

  // Copy and rotate points for each array element
  auto input_span = input_positions->values();
  auto output_span = output_positions->values_writable();

  for (int i = 0; i < count; ++i) {
    const float angle_deg = angle_step * static_cast<float>(i);
    const float angle_rad =
        angle_deg * static_cast<float>(nodo::core::math::PI) / 180.0F;
    const float cos_angle = std::cos(angle_rad);
    const float sin_angle = std::sin(angle_rad);

    // Rotation matrix around Y-axis
    Eigen::Matrix3f rotation;
    rotation << cos_angle, 0.0F, sin_angle, 0.0F, 1.0F, 0.0F, -sin_angle, 0.0F,
        cos_angle;

    // Offset vector for radial placement (on XZ plane)
    const Eigen::Vector3f radial_offset(radius * sin_angle, 0.0F,
                                        radius * cos_angle);

    const size_t point_offset = i * input_point_count;

    for (size_t p = 0; p < input_point_count; ++p) {
      // Rotate around origin, then apply radial offset and center
      Eigen::Vector3f pos = rotation * input_span[p];
      pos = pos + radial_offset + center;
      output_span[point_offset + p] = pos;
    }
  }

  // Copy topology for each array element
  for (int i = 0; i < count; ++i) {
    const int point_offset = i * static_cast<int>(input_point_count);
    int vertex_offset =
        i * static_cast<int>(input_geo.topology().vertex_count());

    for (size_t prim_idx = 0; prim_idx < input_prim_count; ++prim_idx) {
      const auto &prim_verts =
          input_geo.topology().get_primitive_vertices(prim_idx);
      std::vector<int> new_prim_verts;
      new_prim_verts.reserve(prim_verts.size());

      for (int vertex_idx : prim_verts) {
        int input_point_idx = input_geo.topology().get_vertex_point(vertex_idx);
        int new_point_idx = input_point_idx + point_offset;
        int new_vertex_idx = vertex_offset++;

        result->topology().set_vertex_point(new_vertex_idx, new_point_idx);
        new_prim_verts.push_back(new_vertex_idx);
      }

      result->add_primitive(new_prim_verts);
    }
  }

  return result;
}

std::unique_ptr<core::GeometryContainer>
ArraySOP::create_grid_array(const core::GeometryContainer &input_geo,
                            int grid_width, int grid_height) {
  // Get input positions
  auto *input_positions =
      input_geo.get_point_attribute_typed<Eigen::Vector3f>("P");
  if (input_positions == nullptr) {
    return nullptr;
  }

  const size_t input_point_count = input_geo.topology().point_count();
  const size_t input_prim_count = input_geo.topology().primitive_count();

  // Read grid spacing parameters
  const float spacing_x = get_parameter<float>("grid_spacing_x", 1.0F);
  const float spacing_y = get_parameter<float>("grid_spacing_y", 1.0F);

  // Create output container
  auto result = std::make_unique<core::GeometryContainer>();
  const int total_copies = grid_width * grid_height;
  const size_t total_points = input_point_count * total_copies;
  const size_t total_vertices =
      input_geo.topology().vertex_count() * total_copies;

  result->set_point_count(total_points);
  result->set_vertex_count(total_vertices);

  // Add position attribute
  result->add_point_attribute("P", core::AttributeType::VEC3F);
  auto *output_positions =
      result->get_point_attribute_typed<Eigen::Vector3f>("P");

  // Copy and translate points for each grid cell
  auto input_span = input_positions->values();
  auto output_span = output_positions->values_writable();

  int copy_idx = 0;
  for (int row = 0; row < grid_height; ++row) {
    for (int col = 0; col < grid_width; ++col) {
      const Eigen::Vector3f offset(static_cast<float>(col) * spacing_x,
                                   static_cast<float>(row) * spacing_y, 0.0F);

      const size_t point_offset = copy_idx * input_point_count;

      for (size_t pt = 0; pt < input_point_count; ++pt) {
        output_span[point_offset + pt] = input_span[pt] + offset;
      }

      ++copy_idx;
    }
  }

  // Copy topology for each grid cell
  for (int i = 0; i < total_copies; ++i) {
    const int point_offset = i * static_cast<int>(input_point_count);
    int vertex_offset =
        i * static_cast<int>(input_geo.topology().vertex_count());

    for (size_t prim_idx = 0; prim_idx < input_prim_count; ++prim_idx) {
      const auto &prim_verts =
          input_geo.topology().get_primitive_vertices(prim_idx);
      std::vector<int> new_prim_verts;
      new_prim_verts.reserve(prim_verts.size());

      for (int vertex_idx : prim_verts) {
        int input_point_idx = input_geo.topology().get_vertex_point(vertex_idx);
        int new_point_idx = input_point_idx + point_offset;
        int new_vertex_idx = vertex_offset++;

        result->topology().set_vertex_point(new_vertex_idx, new_point_idx);
        new_prim_verts.push_back(new_vertex_idx);
      }

      result->add_primitive(new_prim_verts);
    }
  }

  return result;
}

} // namespace nodo::sop
