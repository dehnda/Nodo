#include "../../include/nodeflux/geometry/box_generator.hpp"

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::geometry {

// Thread-local storage for error reporting
thread_local core::Error BoxGenerator::last_error_{
    core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"};

std::optional<core::GeometryContainer> BoxGenerator::generate(double width, double height,
                                                 double depth,
                                                 int width_segments,
                                                 int height_segments,
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
  auto &topology = container.topology();
  topology.set_point_count(total_vertices);

  // Storage for positions and primitives
  std::vector<core::Vec3f> positions;
  positions.reserve(total_vertices);
  std::vector<std::vector<int>> primitive_vertices;
  primitive_vertices.reserve(total_faces);

  int vertex_index = 0;

  // Define 8 corners
  const Eigen::Vector3d corners[8] = {
      {min_corner.x(), min_corner.y(), min_corner.z()}, // 0: min corner
      {max_corner.x(), min_corner.y(), min_corner.z()}, // 1: +X
      {max_corner.x(), max_corner.y(), min_corner.z()}, // 2: +X+Y
      {min_corner.x(), max_corner.y(), min_corner.z()}, // 3: +Y
      {min_corner.x(), min_corner.y(), max_corner.z()}, // 4: +Z
      {max_corner.x(), min_corner.y(), max_corner.z()}, // 5: +X+Z
      {max_corner.x(), max_corner.y(), max_corner.z()}, // 6: max corner
      {min_corner.x(), max_corner.y(), max_corner.z()}  // 7: +Y+Z
  };

  // Generate 6 faces (flip_normal=true for outward-facing normals)
  // Front face (Z = min_corner.z) - faces toward -Z
  generate_face(positions, primitive_vertices, vertex_index, corners[0], corners[1],
                corners[2], corners[3], width_segments, height_segments, true);

  // Back face (Z = max_corner.z) - faces toward +Z
  generate_face(positions, primitive_vertices, vertex_index, corners[5], corners[4],
                corners[7], corners[6], width_segments, height_segments, true);

  // Left face (X = min_corner.x) - faces toward -X
  generate_face(positions, primitive_vertices, vertex_index, corners[4], corners[0],
                corners[3], corners[7], depth_segments, height_segments, true);

  // Right face (X = max_corner.x) - faces toward +X
  generate_face(positions, primitive_vertices, vertex_index, corners[1], corners[5],
                corners[6], corners[2], depth_segments, height_segments, true);

  // Bottom face (Y = min_corner.y) - faces toward -Y
  generate_face(positions, primitive_vertices, vertex_index, corners[4], corners[5],
                corners[1], corners[0], width_segments, depth_segments, true);

  // Top face (Y = max_corner.y) - faces toward +Y
  generate_face(positions, primitive_vertices, vertex_index, corners[3], corners[2],
                corners[6], corners[7], width_segments, depth_segments, true);

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

  // Calculate and add normals
  // For boxes, we can compute face normals from the corner arrangement
  container.add_vertex_attribute(attrs::N, core::AttributeType::VEC3F);
  auto *n_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::N);
  if (n_storage != nullptr) {
    auto n_span = n_storage->values_writable();

    // Compute normals by calculating face normals from cross products
    // Since this is a box with flat faces, we can compute per-face normals
    for (size_t prim_idx = 0; prim_idx < primitive_vertices.size(); ++prim_idx) {
      const auto &verts = primitive_vertices[prim_idx];
      if (verts.size() >= 3) {
        const auto &p0 = positions[verts[0]];
        const auto &p1 = positions[verts[1]];
        const auto &p2 = positions[verts[2]];

        // Compute face normal
        core::Vec3f edge1 = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
        core::Vec3f edge2 = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};

        // Cross product
        core::Vec3f normal = {
          edge1[1] * edge2[2] - edge1[2] * edge2[1],
          edge1[2] * edge2[0] - edge1[0] * edge2[2],
          edge1[0] * edge2[1] - edge1[1] * edge2[0]
        };

        // Normalize
        float length = std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
        if (length > 0.0F) {
          normal[0] /= length;
          normal[1] /= length;
          normal[2] /= length;
        }

        // Assign to all vertices of this primitive
        for (int vert_idx : verts) {
          n_span[vert_idx] = normal;
        }
      }
    }
  }

  return container;
}

void BoxGenerator::generate_face(std::vector<core::Vec3f>& positions,
                                 std::vector<std::vector<int>>& primitive_vertices,
                                 int &vertex_index,
                                 const Eigen::Vector3d &corner1,
                                 const Eigen::Vector3d &corner2,
                                 const Eigen::Vector3d &corner3,
                                 const Eigen::Vector3d &corner4, int u_segments,
                                 int v_segments, bool flip_normal) {

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
        // First triangle
        primitive_vertices.push_back({bottom_left, bottom_right, top_left});

        // Second triangle
        primitive_vertices.push_back({bottom_right, top_right, top_left});
      } else {
        // First triangle
        primitive_vertices.push_back({bottom_left, top_left, bottom_right});

        // Second triangle
        primitive_vertices.push_back({bottom_right, top_left, top_right});
      }
    }
  }
}

const core::Error &BoxGenerator::last_error() { return last_error_; }

void BoxGenerator::set_last_error(const core::Error &error) {
  last_error_ = error;
}

} // namespace nodeflux::geometry
