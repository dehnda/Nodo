#include "nodeflux/sop/line_sop.hpp"
#include "nodeflux/core/math.hpp"
#include <Eigen/Dense>

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::sop {

LineSOP::LineSOP(const std::string &name) : SOPNode(name, "Line") {
  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_float_parameter("start_x", 0.0F)
                         .label("Start X")
                         .range(-100.0, 100.0)
                         .category("Start Point")
                         .build());

  register_parameter(define_float_parameter("start_y", 0.0F)
                         .label("Start Y")
                         .range(-100.0, 100.0)
                         .category("Start Point")
                         .build());

  register_parameter(define_float_parameter("start_z", 0.0F)
                         .label("Start Z")
                         .range(-100.0, 100.0)
                         .category("Start Point")
                         .build());

  register_parameter(define_float_parameter("end_x", 1.0F)
                         .label("End X")
                         .range(-100.0, 100.0)
                         .category("End Point")
                         .build());

  register_parameter(define_float_parameter("end_y", 0.0F)
                         .label("End Y")
                         .range(-100.0, 100.0)
                         .category("End Point")
                         .build());

  register_parameter(define_float_parameter("end_z", 0.0F)
                         .label("End Z")
                         .range(-100.0, 100.0)
                         .category("End Point")
                         .build());

  register_parameter(define_int_parameter("segments", 10)
                         .label("Segments")
                         .range(1, 1000)
                         .category("Line")
                         .build());
}

// Helper to convert GeometryContainer to GeometryData (bridge for migration)
static std::shared_ptr<GeometryData>
convert_from_container(const core::GeometryContainer &container) {
  const auto &topology = container.topology();

  // Extract positions
  auto *p_storage =
      container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (!p_storage)
    return std::make_shared<GeometryData>(std::make_shared<core::Mesh>());

  Eigen::MatrixXd vertices(topology.point_count(), 3);
  auto p_span = p_storage->values();
  for (size_t i = 0; i < p_span.size(); ++i) {
    vertices.row(i) = p_span[i].cast<double>();
  }

  // Extract primitives (line segments represented as degenerate triangles)
  Eigen::MatrixXi faces(topology.primitive_count(), 3);
  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &verts = topology.get_primitive_vertices(prim_idx);
    if (verts.size() >= 2) {
      // Line segment: [i, i+1, i+1] (degenerate triangle marker)
      faces(prim_idx, 0) = verts[0];
      faces(prim_idx, 1) = verts[1];
      faces(prim_idx, 2) = verts[1]; // Duplicate last vertex to mark as line
    }
  }

  auto mesh = std::make_shared<core::Mesh>(vertices, faces);
  return std::make_shared<GeometryData>(mesh);
}

std::shared_ptr<GeometryData> LineSOP::execute() {
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
  auto &topology = container.topology();

  // Set point count
  topology.set_point_count(num_points);

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

  // Convert back to GeometryData for compatibility
  return convert_from_container(container);
}

} // namespace nodeflux::sop
