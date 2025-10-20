#include "nodeflux/sop/resample_sop.hpp"
#include <Eigen/Dense>
#include <cmath>
#include <iostream>
#include <vector>

namespace nodeflux::sop {

ResampleSOP::ResampleSOP(std::string name) : name_(std::move(name)) {}

void ResampleSOP::set_mode(Mode mode) {
  if (mode_ != mode) {
    mode_ = mode;
    mark_dirty();
  }
}

void ResampleSOP::set_point_count(int count) {
  const int min_points = 2;
  if (count < min_points) {
    count = min_points;
  }
  if (point_count_ != count) {
    point_count_ = count;
    mark_dirty();
  }
}

void ResampleSOP::set_segment_length(float length) {
  const float min_length = 0.001F;
  if (length < min_length) {
    length = min_length;
  }
  if (segment_length_ != length) {
    segment_length_ = length;
    mark_dirty();
  }
}

void ResampleSOP::set_input_mesh(std::shared_ptr<core::Mesh> mesh) {
  if (input_mesh_ != mesh) {
    input_mesh_ = std::move(mesh);
    mark_dirty();
  }
}

float ResampleSOP::calculate_curve_length(const core::Mesh &mesh) const {
  const auto &vertices = mesh.vertices();
  if (vertices.rows() < 2) {
    return 0.0F;
  }

  float total_length = 0.0F;
  for (int i = 0; i < vertices.rows() - 1; ++i) {
    Eigen::Vector3d diff = vertices.row(i + 1) - vertices.row(i);
    total_length += static_cast<float>(diff.norm());
  }

  return total_length;
}

std::optional<core::Mesh> ResampleSOP::execute() {
  if (!input_mesh_ || input_mesh_->vertex_count() < 2) {
    std::cout << "ResampleSOP '" << name_ << "': Invalid input mesh\n";
    return std::nullopt;
  }

  const auto &input_vertices = input_mesh_->vertices();
  const int input_count = input_vertices.rows();

  // Calculate total curve length
  const float total_length = calculate_curve_length(*input_mesh_);
  if (total_length < 0.001F) {
    std::cout << "ResampleSOP '" << name_ << "': Curve too short to resample\n";
    return std::nullopt;
  }

  // Determine target point count
  int target_count = point_count_;
  if (mode_ == Mode::BY_LENGTH) {
    target_count =
        static_cast<int>(std::ceil(total_length / segment_length_)) + 1;
    if (target_count < 2) {
      target_count = 2;
    }
  }

  std::cout << "ResampleSOP '" << name_ << "': Resampling " << input_count
            << " points to " << target_count
            << " points (length: " << total_length << ")\n";

  // Build cumulative length array for parametric sampling
  std::vector<float> cumulative_lengths(input_count);
  cumulative_lengths[0] = 0.0F;

  for (int i = 1; i < input_count; ++i) {
    Eigen::Vector3d diff = input_vertices.row(i) - input_vertices.row(i - 1);
    cumulative_lengths[i] =
        cumulative_lengths[i - 1] + static_cast<float>(diff.norm());
  }

  // Resample points uniformly along the curve
  Eigen::MatrixXd resampled_vertices(target_count, 3);

  for (int i = 0; i < target_count; ++i) {
    // Calculate target distance along curve
    const float target_dist = (total_length * static_cast<float>(i)) /
                              static_cast<float>(target_count - 1);

    // Find segment containing this distance
    int seg_idx = 0;
    for (int j = 1; j < input_count; ++j) {
      if (cumulative_lengths[j] >= target_dist) {
        seg_idx = j - 1;
        break;
      }
    }

    // Handle edge case for last point
    if (i == target_count - 1) {
      resampled_vertices.row(i) = input_vertices.row(input_count - 1);
      continue;
    }

    // Interpolate within segment
    const float seg_start_dist = cumulative_lengths[seg_idx];
    const float seg_end_dist = cumulative_lengths[seg_idx + 1];
    const float seg_length = seg_end_dist - seg_start_dist;

    float interpolation_factor = 0.0F;
    if (seg_length > 0.0001F) {
      interpolation_factor = (target_dist - seg_start_dist) / seg_length;
    }

    resampled_vertices.row(i) =
        input_vertices.row(seg_idx) +
        interpolation_factor *
            (input_vertices.row(seg_idx + 1) - input_vertices.row(seg_idx));
  }

  // Create edges as degenerate triangles
  const int num_segments = target_count - 1;
  Eigen::MatrixXi faces(num_segments, 3);

  for (int i = 0; i < num_segments; ++i) {
    faces(i, 0) = i;
    faces(i, 1) = i + 1;
    faces(i, 2) = i + 1; // Degenerate triangle
  }

  core::Mesh result(resampled_vertices, faces);

  std::cout << "ResampleSOP '" << name_ << "': Resampling completed\n";

  return result;
}

std::shared_ptr<core::Mesh> ResampleSOP::cook() {
  if (!is_dirty_ && cached_result_) {
    std::cout << "ResampleSOP '" << name_ << "': Using cached result\n";
    return cached_result_;
  }

  std::cout << "ResampleSOP '" << name_ << "': Computing resample...\n";

  auto result = execute();
  if (!result) {
    std::cerr << "ResampleSOP '" << name_ << "': Resampling failed\n";
    return nullptr;
  }

  cached_result_ = std::make_shared<core::Mesh>(std::move(*result));
  is_dirty_ = false;

  std::cout << "ResampleSOP '" << name_ << "': Resampled to "
            << cached_result_->vertex_count() << " vertices\n";

  return cached_result_;
}

} // namespace nodeflux::sop
