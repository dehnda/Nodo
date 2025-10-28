#include "nodo/geometry/box_generator.hpp"
#include "nodo/sop/sop_utils.hpp"

namespace attrs = nodo::core::standard_attrs;

namespace nodo::geometry {

// Thread-local storage for error reporting
thread_local core::Error BoxGenerator::last_error_{
    core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"};

std::optional<core::GeometryContainer>
BoxGenerator::generate(double width, double height, double depth,
                       int width_segments, int height_segments,
                       int depth_segments) {

  if (width <= 0.0 || height <= 0.0 || depth <= 0.0) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidFormat,
                               "Box dimensions must be positive"});
    return std::nullopt;
  }

  if (width_segments < 1 || height_segments < 1 || depth_segments < 1) {
    set_last_error(core::Error{
        core::ErrorCategory::Validation, core::ErrorCode::InvalidFormat,
        "Box requires at least 1 segment in each dimension"});
    return std::nullopt;
  }

  const double half_width = width * 0.5;
  const double half_height = height * 0.5;
  const double half_depth = depth * 0.5;

  return generate_from_bounds(
      Eigen::Vector3d(-half_width, -half_height, -half_depth),
      Eigen::Vector3d(half_width, half_height, half_depth), width_segments,
      height_segments, depth_segments);
}

std::optional<core::GeometryContainer> BoxGenerator::generate_from_bounds(
    const Eigen::Vector3d &min_corner, const Eigen::Vector3d &max_corner,
    int width_segments, int height_segments, int depth_segments) {

  if ((max_corner.array() <= min_corner.array()).any()) {
    set_last_error(core::Error{
        core::ErrorCategory::Validation, core::ErrorCode::InvalidFormat,
        "Max corner must be greater than min corner in all dimensions"});
    return std::nullopt;
  }

  if (width_segments < 1 || height_segments < 1 || depth_segments < 1) {
    set_last_error(core::Error{
        core::ErrorCategory::Validation, core::ErrorCode::InvalidFormat,
        "Box requires at least 1 segment in each dimension"});
    return std::nullopt;
  }

  // Calculate total vertices and faces
  auto vertices_per_face = [](int u_seg, int v_seg) {
    return (u_seg + 1) * (v_seg + 1);
  };

  const int total_vertices =
      2 * vertices_per_face(width_segments, height_segments) + // front + back
      2 * vertices_per_face(depth_segments, height_segments) + // left + right
      2 * vertices_per_face(width_segments, depth_segments);   // top + bottom

  auto faces_per_face = [](int u_seg, int v_seg) { return u_seg * v_seg * 2; };

  const int total_faces =
      2 * faces_per_face(width_segments, height_segments) + // front + back
      2 * faces_per_face(depth_segments, height_segments) + // left + right
      2 * faces_per_face(width_segments, depth_segments);   // top + bottom

  // Create GeometryContainer
  core::GeometryContainer container;

  // Build a 3D grid of points (shared at edges/corners)
  const int num_x = width_segments + 1;
  const int num_y = height_segments + 1;
  const int num_z = depth_segments + 1;
  std::vector<core::Vec3f> positions(num_x * num_y * num_z);
  auto lerp = [](double start_val, double end_val, double t_val) {
    return start_val + (t_val * (end_val - start_val));
  };
  for (int z_idx = 0; z_idx < num_z; ++z_idx) {
    double z_pos =
        lerp(min_corner.z(), max_corner.z(), double(z_idx) / double(num_z - 1));
    for (int y_idx = 0; y_idx < num_y; ++y_idx) {
      double y_pos = lerp(min_corner.y(), max_corner.y(),
                          double(y_idx) / double(num_y - 1));
      for (int x_idx = 0; x_idx < num_x; ++x_idx) {
        double x_pos = lerp(min_corner.x(), max_corner.x(),
                            double(x_idx) / double(num_x - 1));
        positions[(z_idx * num_y * num_x) + (y_idx * num_x) + x_idx] =
            core::Vec3f(float(x_pos), float(y_pos), float(z_pos));
      }
    }
  }

  // Build quad faces for each of the 6 sides
  std::vector<std::vector<int>> primitive_vertices;
  // -Z face
  for (int y_idx = 0; y_idx < num_y - 1; ++y_idx) {
    for (int x_idx = 0; x_idx < num_x - 1; ++x_idx) {
      int v00 = (0 * num_y * num_x) + (y_idx * num_x) + x_idx; // bottom-left
      int v10 =
          (0 * num_y * num_x) + (y_idx * num_x) + (x_idx + 1); // bottom-right
      int v11 = (0 * num_y * num_x) + ((y_idx + 1) * num_x) +
                (x_idx + 1); // top-right
      int v01 = (0 * num_y * num_x) + ((y_idx + 1) * num_x) + x_idx; // top-left
      primitive_vertices.push_back({v00, v01, v11, v10});
    }
  }
  // +Z face
  for (int y_idx = 0; y_idx < num_y - 1; ++y_idx) {
    for (int x_idx = 0; x_idx < num_x - 1; ++x_idx) {
      int v00 = ((num_z - 1) * num_y * num_x) + (y_idx * num_x) + x_idx;
      int v01 = ((num_z - 1) * num_y * num_x) + ((y_idx + 1) * num_x) + x_idx;
      int v11 =
          ((num_z - 1) * num_y * num_x) + ((y_idx + 1) * num_x) + (x_idx + 1);
      int v10 = ((num_z - 1) * num_y * num_x) + (y_idx * num_x) + (x_idx + 1);
      primitive_vertices.push_back({v00, v10, v11, v01});
    }
  }
  // -Y face
  for (int z_idx = 0; z_idx < num_z - 1; ++z_idx) {
    for (int x_idx = 0; x_idx < num_x - 1; ++x_idx) {
      int v00 = (z_idx * num_y * num_x) + (0 * num_x) + x_idx;
      int v10 = (z_idx * num_y * num_x) + (0 * num_x) + (x_idx + 1);
      int v11 = ((z_idx + 1) * num_y * num_x) + (0 * num_x) + (x_idx + 1);
      int v01 = ((z_idx + 1) * num_y * num_x) + (0 * num_x) + x_idx;
      primitive_vertices.push_back({v00, v10, v11, v01});
    }
  }
  // +Y face (CORRECT - keep this winding)
  for (int z_idx = 0; z_idx < num_z - 1; ++z_idx) {
    for (int x_idx = 0; x_idx < num_x - 1; ++x_idx) {
      int v00 = (z_idx * num_y * num_x) + ((num_y - 1) * num_x) + x_idx;
      int v10 = (z_idx * num_y * num_x) + ((num_y - 1) * num_x) + (x_idx + 1);
      int v11 =
          ((z_idx + 1) * num_y * num_x) + ((num_y - 1) * num_x) + (x_idx + 1);
      int v01 = ((z_idx + 1) * num_y * num_x) + ((num_y - 1) * num_x) + x_idx;
      primitive_vertices.push_back({v00, v01, v11, v10});
    }
  }
  // -X face
  for (int z_idx = 0; z_idx < num_z - 1; ++z_idx) {
    for (int y_idx = 0; y_idx < num_y - 1; ++y_idx) {
      int v00 = (z_idx * num_y * num_x) + (y_idx * num_x) + 0;
      int v10 = (z_idx * num_y * num_x) + ((y_idx + 1) * num_x) + 0;
      int v11 = ((z_idx + 1) * num_y * num_x) + ((y_idx + 1) * num_x) + 0;
      int v01 = ((z_idx + 1) * num_y * num_x) + (y_idx * num_x) + 0;
      primitive_vertices.push_back({v00, v01, v11, v10});
    }
  }
  // +X face
  for (int z_idx = 0; z_idx < num_z - 1; ++z_idx) {
    for (int y_idx = 0; y_idx < num_y - 1; ++y_idx) {
      int v00 = (z_idx * num_y * num_x) + (y_idx * num_x) + (num_x - 1);
      int v01 = ((z_idx + 1) * num_y * num_x) + (y_idx * num_x) + (num_x - 1);
      int v11 =
          ((z_idx + 1) * num_y * num_x) + ((y_idx + 1) * num_x) + (num_x - 1);
      int v10 = (z_idx * num_y * num_x) + ((y_idx + 1) * num_x) + (num_x - 1);
      primitive_vertices.push_back({v00, v10, v11, v01});
    }
  }

  // Update point and vertex counts to actual size
  const size_t actual_point_count = positions.size();
  container.set_point_count(actual_point_count);  // Use container method!
  container.set_vertex_count(actual_point_count); // 1:1 mapping

  // Get topology reference for adding primitives
  auto &topology = container.topology();

  // Set up 1:1 vertex→point mapping
  for (size_t i = 0; i < actual_point_count; ++i) {
    topology.set_vertex_point(i, static_cast<int>(i));
  }

  // Add primitives to topology
  for (const auto &prim_verts : primitive_vertices) {
    topology.add_primitive(prim_verts);
  }

  // Add P (position) attribute
  container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage != nullptr) {
    auto p_span = p_storage->values_writable();
    std::copy(positions.begin(), positions.end(), p_span.begin());
  }

  // Use hard edge normals for box (creates crisp edges)
  nodo::sop::utils::compute_hard_edge_normals(container);

  return container;
}

void BoxGenerator::generate_face(
    std::vector<core::Vec3f> &positions,
    std::vector<std::vector<int>> &primitive_vertices, int &vertex_index,
    const Eigen::Vector3d &corner1, const Eigen::Vector3d &corner2,
    const Eigen::Vector3d &corner3, const Eigen::Vector3d &corner4,
    int u_segments, int v_segments, bool flip_normal) {

  const int start_vertex = vertex_index;

  // Generate vertices for this face
  for (int v_idx = 0; v_idx <= v_segments; ++v_idx) {
    const double v_ratio =
        static_cast<double>(v_idx) / static_cast<double>(v_segments);

    for (int u_idx = 0; u_idx <= u_segments; ++u_idx) {
      const double u_ratio =
          static_cast<double>(u_idx) / static_cast<double>(u_segments);

      // Bilinear interpolation
      const Eigen::Vector3d bottom_edge =
          corner1 + u_ratio * (corner2 - corner1);
      const Eigen::Vector3d top_edge = corner4 + u_ratio * (corner3 - corner4);
      const Eigen::Vector3d vertex_pos =
          bottom_edge + v_ratio * (top_edge - bottom_edge);

      positions.push_back({static_cast<float>(vertex_pos.x()),
                           static_cast<float>(vertex_pos.y()),
                           static_cast<float>(vertex_pos.z())});
      ++vertex_index;
    }
  }

  // Generate faces for this face
  const int vertices_per_row = u_segments + 1;

  for (int v_idx = 0; v_idx < v_segments; ++v_idx) {
    for (int u_idx = 0; u_idx < u_segments; ++u_idx) {
      const int bottom_left = start_vertex + (v_idx * vertices_per_row) + u_idx;
      const int bottom_right = bottom_left + 1;
      const int top_left = bottom_left + vertices_per_row;
      const int top_right = top_left + 1;

      if (flip_normal) {
        primitive_vertices.push_back(
            {bottom_right, bottom_left, top_left, top_right});
      } else {
        primitive_vertices.push_back(
            {bottom_left, bottom_right, top_right, top_left});
      }
    }
  }
}

const core::Error &BoxGenerator::last_error() { return last_error_; }

void BoxGenerator::set_last_error(const core::Error &error) {
  last_error_ = error;
}

} // namespace nodo::geometry
