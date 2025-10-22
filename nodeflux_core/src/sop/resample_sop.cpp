#include "nodeflux/sop/resample_sop.hpp"
#include <Eigen/Dense>
#include <cmath>
#include <iostream>
#include <vector>

namespace nodeflux::sop {

ResampleSOP::ResampleSOP(const std::string &name)
    : SOPNode(name, "Resample") {}

// Helper function for calculating curve length
static float calculate_curve_length(const core::Mesh &mesh) {
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

std::shared_ptr<GeometryData> ResampleSOP::execute() {
  // Get input geometry
  auto input_geo = get_input_data(0);
  if (!input_geo) {
    set_error("No input geometry connected");
    return nullptr;
  }

  auto input_mesh = input_geo->get_mesh();
  if (!input_mesh || input_mesh->vertex_count() < 2) {
    set_error("Input geometry does not contain a valid mesh");
    return nullptr;
  }

  const auto &input_vertices = input_mesh->vertices();
  const int input_count = input_vertices.rows();

  // Calculate total curve length
  const float total_length = calculate_curve_length(*input_mesh);
  if (total_length < 0.001F) {
    set_error("Curve too short to resample");
    return nullptr;
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

  std::cout << "ResampleSOP '" << get_name() << "': Resampling " << input_count
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

  auto result_mesh =
      std::make_shared<core::Mesh>(resampled_vertices, faces);
  return std::make_shared<GeometryData>(result_mesh);
}

} // namespace nodeflux::sop
