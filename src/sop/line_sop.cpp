#include "nodeflux/sop/line_sop.hpp"
#include <Eigen/Dense>
#include <iostream>

namespace nodeflux::sop {

LineSOP::LineSOP(std::string name) : name_(std::move(name)) {}

void LineSOP::set_start_point(float x_coord, float y_coord, float z_coord) {
  start_point_ = {x_coord, y_coord, z_coord};
  mark_dirty();
}

void LineSOP::set_start_point(const std::array<float, 3> &point) {
  start_point_ = point;
  mark_dirty();
}

void LineSOP::set_end_point(float x_coord, float y_coord, float z_coord) {
  end_point_ = {x_coord, y_coord, z_coord};
  mark_dirty();
}

void LineSOP::set_end_point(const std::array<float, 3> &point) {
  end_point_ = point;
  mark_dirty();
}

void LineSOP::set_segments(int segments) {
  if (segments < 1) {
    segments = 1;
  }
  if (segments_ != segments) {
    segments_ = segments;
    mark_dirty();
  }
}

std::optional<core::Mesh> LineSOP::execute() {
  const int num_points = segments_ + 1;

  std::cout << "LineSOP '" << name_ << "': Generating line with " << num_points
            << " points (" << segments_ << " segments)\n";

  // Create vertices along the line
  Eigen::MatrixXd vertices(num_points, 3);

  for (int i = 0; i < num_points; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(segments_);
    vertices(i, 0) = start_point_[0] + t * (end_point_[0] - start_point_[0]);
    vertices(i, 1) = start_point_[1] + t * (end_point_[1] - start_point_[1]);
    vertices(i, 2) = start_point_[2] + t * (end_point_[2] - start_point_[2]);
  }

  // Create edges as degenerate triangles (line segments)
  // Each segment is represented as a face with indices [i, i+1, i+1]
  Eigen::MatrixXi faces(segments_, 3);

  for (int i = 0; i < segments_; ++i) {
    faces(i, 0) = i;
    faces(i, 1) = i + 1;
    faces(i, 2) = i + 1; // Degenerate triangle to represent line
  }

  core::Mesh result(std::move(vertices), std::move(faces));

  std::cout << "LineSOP '" << name_ << "': Line generated successfully\n";

  return result;
}

std::shared_ptr<core::Mesh> LineSOP::cook() {
  if (!is_dirty_ && cached_result_) {
    std::cout << "LineSOP '" << name_ << "': Using cached result\n";
    return cached_result_;
  }

  std::cout << "LineSOP '" << name_ << "': Computing line...\n";

  auto result = execute();
  if (!result) {
    std::cerr << "LineSOP '" << name_ << "': Line generation failed\n";
    return nullptr;
  }

  cached_result_ = std::make_shared<core::Mesh>(std::move(*result));
  is_dirty_ = false;

  std::cout << "LineSOP '" << name_ << "': Line generated with "
            << cached_result_->vertex_count() << " vertices\n";

  return cached_result_;
}

} // namespace nodeflux::sop
