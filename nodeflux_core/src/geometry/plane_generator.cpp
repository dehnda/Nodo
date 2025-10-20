#include "../../include/nodeflux/geometry/plane_generator.hpp"

namespace nodeflux::geometry {

// Thread-local storage for error reporting
thread_local core::Error PlaneGenerator::last_error_{
    core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"};

std::optional<core::Mesh> PlaneGenerator::generate(double width, double height,
                                                   int width_segments,
                                                   int height_segments) {

  if (width <= 0.0) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidFormat,
                               "Plane width must be positive"});
    return std::nullopt;
  }

  if (height <= 0.0) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidFormat,
                               "Plane height must be positive"});
    return std::nullopt;
  }

  if (width_segments < 1) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidFormat,
                               "Plane requires at least 1 width segment"});
    return std::nullopt;
  }

  if (height_segments < 1) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidFormat,
                               "Plane requires at least 1 height segment"});
    return std::nullopt;
  }

  core::Mesh mesh;

  // Calculate vertices and faces
  const int vertices_per_row = width_segments + 1;
  const int vertices_per_col = height_segments + 1;
  const int total_vertices = vertices_per_row * vertices_per_col;
  const int total_faces = width_segments * height_segments * 2;

  mesh.vertices().resize(total_vertices, 3);
  mesh.faces().resize(total_faces, 3);

  // Generate vertices
  int vertex_index = 0;
  const double half_width = width * 0.5;
  const double half_height = height * 0.5;

  for (int row = 0; row < vertices_per_col; ++row) {
    const double coord_z =
        -half_height +
        (static_cast<double>(row) / static_cast<double>(height_segments)) *
            height;

    for (int col = 0; col < vertices_per_row; ++col) {
      const double coord_x =
          -half_width +
          (static_cast<double>(col) / static_cast<double>(width_segments)) *
              width;

      mesh.vertices()(vertex_index, 0) = coord_x;
      mesh.vertices()(vertex_index, 1) = 0.0; // Plane lies in XZ plane
      mesh.vertices()(vertex_index, 2) = coord_z;
      ++vertex_index;
    }
  }

  // Generate faces
  int face_index = 0;

  for (int row = 0; row < height_segments; ++row) {
    for (int col = 0; col < width_segments; ++col) {
      const int top_left = (row * vertices_per_row) + col;
      const int top_right = top_left + 1;
      const int bottom_left = ((row + 1) * vertices_per_row) + col;
      const int bottom_right = bottom_left + 1;

      // First triangle (top-left, bottom-left, top-right)
      mesh.faces()(face_index, 0) = top_left;
      mesh.faces()(face_index, 1) = bottom_left;
      mesh.faces()(face_index, 2) = top_right;
      ++face_index;

      // Second triangle (top-right, bottom-left, bottom-right)
      mesh.faces()(face_index, 0) = top_right;
      mesh.faces()(face_index, 1) = bottom_left;
      mesh.faces()(face_index, 2) = bottom_right;
      ++face_index;
    }
  }

  return mesh;
}

const core::Error &PlaneGenerator::last_error() { return last_error_; }

void PlaneGenerator::set_last_error(const core::Error &error) {
  last_error_ = error;
}

} // namespace nodeflux::geometry
