#include "nodeflux/sop/line_sop.hpp"
#include <Eigen/Dense>

namespace nodeflux::sop {

LineSOP::LineSOP(const std::string &name) : SOPNode(name, "Line") {}

std::shared_ptr<GeometryData> LineSOP::execute() {
  if (segments_ < 1) {
    set_error("Segments must be at least 1");
    return nullptr;
  }

  const int num_points = segments_ + 1;

  // Create vertices along the line
  Eigen::MatrixXd vertices(num_points, 3);

  for (int i = 0; i < num_points; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(segments_);
    vertices(i, 0) = start_point_[0] + t * (end_point_[0] - start_point_[0]);
    vertices(i, 1) = start_point_[1] + t * (end_point_[1] - start_point_[1]);
    vertices(i, 2) = start_point_[2] + t * (end_point_[2] - start_point_[2]);
  }

  // Create edges as line segments (store as degenerate triangles for
  // compatibility) Face format: [i, i+1, i+1] indicates this is a line edge
  // from i to i+1
  Eigen::MatrixXi faces(segments_, 3);

  for (int i = 0; i < segments_; ++i) {
    faces(i, 0) = i;
    faces(i, 1) = i + 1;
    faces(i, 2) = i + 1; // Degenerate triangle marker for line edge
  }

  auto result_mesh =
      std::make_shared<core::Mesh>(std::move(vertices), std::move(faces));
  return std::make_shared<GeometryData>(result_mesh);
}

} // namespace nodeflux::sop
