#include "nodo/sop/line_sop.hpp"
#include "nodo/core/geometry_container.hpp"
#include "nodo/core/math.hpp"
#include <Eigen/Dense>
#include <memory>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::sop {

LineSOP::LineSOP(const std::string &name) : SOPNode(name, "Line") {
  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_float_parameter("start_x", 0.0F)
                         .label("Start X")
                         .range(-100.0, 100.0)
                         .category("Start Point")
                         .description("X coordinate of the line start point")
                         .build());

  register_parameter(define_float_parameter("start_y", 0.0F)
                         .label("Start Y")
                         .range(-100.0, 100.0)
                         .category("Start Point")
                         .description("Y coordinate of the line start point")
                         .build());

  register_parameter(define_float_parameter("start_z", 0.0F)
                         .label("Start Z")
                         .range(-100.0, 100.0)
                         .category("Start Point")
                         .description("Z coordinate of the line start point")
                         .build());

  register_parameter(define_float_parameter("end_x", 5.0F)
                         .label("End X")
                         .range(-100.0, 100.0)
                         .category("End Point")
                         .description("X coordinate of the line end point")
                         .build());

  register_parameter(define_float_parameter("end_y", 0.0F)
                         .label("End Y")
                         .range(-100.0, 100.0)
                         .category("End Point")
                         .description("Y coordinate of the line end point")
                         .build());

  register_parameter(define_float_parameter("end_z", 0.0F)
                         .label("End Z")
                         .range(-100.0, 100.0)
                         .category("End Point")
                         .description("Z coordinate of the line end point")
                         .build());

  register_parameter(
      define_int_parameter("segments", 10)
          .label("Segments")
          .range(1, 1000)
          .category("Line")
          .description("Number of segments (points = segments + 1)")
          .build());
}

std::shared_ptr<core::GeometryContainer> LineSOP::execute() {
  // Read parameters from parameter system
  const float start_x = get_parameter<float>("start_x", 0.0F);
  const float start_y = get_parameter<float>("start_y", 0.0F);
  const float start_z = get_parameter<float>("start_z", 0.0F);
  const float end_x = get_parameter<float>("end_x", 1.0F);
  const float end_y = get_parameter<float>("end_y", 0.0F);
  const float end_z = get_parameter<float>("end_z", 0.0F);
  const int segments = get_parameter<int>("segments", 10);

  if (segments < 1) {
    set_error("Segments must be at least 1");
    return nullptr;
  }

  const int num_points = segments + 1;

  // Create GeometryContainer
  core::GeometryContainer container;

  // Set point and vertex count (1:1 mapping for lines)
  container.set_point_count(num_points);
  container.set_vertex_count(num_points);

  auto &topology = container.topology();

  // Set up 1:1 vertex-to-point mapping
  for (int i = 0; i < num_points; ++i) {
    topology.set_vertex_point(i, i);
  }

  // Create line segment primitives (2 vertices per edge)
  for (int i = 0; i < segments; ++i) {
    topology.add_primitive({i, i + 1});
  }

  // Calculate positions along the line
  std::vector<core::Vec3f> positions;
  positions.reserve(num_points);

  for (int i = 0; i < num_points; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(segments);
    const float x = start_x + t * (end_x - start_x);
    const float y = start_y + t * (end_y - start_y);
    const float z = start_z + t * (end_z - start_z);
    positions.push_back({x, y, z});
  }

  // Add P (position) attribute
  container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage != nullptr) {
    auto p_span = p_storage->values_writable();
    std::copy(positions.begin(), positions.end(), p_span.begin());
  }

  return std::make_shared<core::GeometryContainer>(std::move(container));
}

} // namespace nodo::sop
