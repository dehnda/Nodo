#include "nodeflux/sop/line_sop.hpp"
#include <Eigen/Dense>

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

  // Create vertices along the line
  Eigen::MatrixXd vertices(num_points, 3);

  for (int i = 0; i < num_points; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(segments);
    vertices(i, 0) = start_x + t * (end_x - start_x);
    vertices(i, 1) = start_y + t * (end_y - start_y);
    vertices(i, 2) = start_z + t * (end_z - start_z);
  }

  // Create edges as line segments (store as degenerate triangles for
  // compatibility) Face format: [i, i+1, i+1] indicates this is a line edge
  // from i to i+1
  Eigen::MatrixXi faces(segments, 3);

  for (int i = 0; i < segments; ++i) {
    faces(i, 0) = i;
    faces(i, 1) = i + 1;
    faces(i, 2) = i + 1; // Degenerate triangle marker for line edge
  }

  auto result_mesh =
      std::make_shared<core::Mesh>(std::move(vertices), std::move(faces));
  return std::make_shared<GeometryData>(result_mesh);
}

} // namespace nodeflux::sop
