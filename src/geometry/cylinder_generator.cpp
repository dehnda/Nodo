#include "../../include/nodeflux/geometry/cylinder_generator.hpp"
#include <cmath>
#include <numbers>

namespace nodeflux::geometry {

// Thread-local storage for error reporting
thread_local core::Error CylinderGenerator::last_error_{
    core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"};

std::optional<core::Mesh>
CylinderGenerator::generate(double radius, double height, int radial_segments,
                            int height_segments, bool top_cap,
                            bool bottom_cap) {

  if (radius <= 0.0) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidFormat,
                               "Cylinder radius must be positive"});
    return std::nullopt;
  }

  if (height <= 0.0) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidFormat,
                               "Cylinder height must be positive"});
    return std::nullopt;
  }

  if (radial_segments < 3) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidFormat,
                               "Cylinder requires at least 3 radial segments"});
    return std::nullopt;
  }

  if (height_segments < 1) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidFormat,
                               "Cylinder requires at least 1 height segment"});
    return std::nullopt;
  }

  core::Mesh mesh;

  // Calculate vertices and faces count
  const int ring_vertices = (height_segments + 1) * radial_segments;
  const int cap_vertices = (top_cap ? 1 : 0) + (bottom_cap ? 1 : 0);
  const int total_vertices = ring_vertices + cap_vertices;

  const int side_faces = height_segments * radial_segments * 2;
  const int cap_faces =
      (top_cap ? radial_segments : 0) + (bottom_cap ? radial_segments : 0);
  const int total_faces = side_faces + cap_faces;

  mesh.vertices().resize(total_vertices, 3);
  mesh.faces().resize(total_faces, 3);

  // Generate vertices
  int vertex_index = 0;

  // Generate ring vertices
  for (int height_ring = 0; height_ring <= height_segments; ++height_ring) {
    const double coord_y = (static_cast<double>(height_ring) /
                                static_cast<double>(height_segments) -
                            0.5) *
                           height;

    for (int segment = 0; segment < radial_segments; ++segment) {
      const double angle = 2.0 * std::numbers::pi *
                           static_cast<double>(segment) /
                           static_cast<double>(radial_segments);
      const double coord_x = radius * std::cos(angle);
      const double coord_z = radius * std::sin(angle);

      mesh.vertices()(vertex_index, 0) = coord_x;
      mesh.vertices()(vertex_index, 1) = coord_y;
      mesh.vertices()(vertex_index, 2) = coord_z;
      ++vertex_index;
    }
  }

  // Cap centers
  int top_center = -1;
  int bottom_center = -1;

  if (top_cap) {
    top_center = vertex_index;
    mesh.vertices()(vertex_index, 0) = 0.0;
    mesh.vertices()(vertex_index, 1) = height * 0.5;
    mesh.vertices()(vertex_index, 2) = 0.0;
    ++vertex_index;
  }

  if (bottom_cap) {
    bottom_center = vertex_index;
    mesh.vertices()(vertex_index, 0) = 0.0;
    mesh.vertices()(vertex_index, 1) = -height * 0.5;
    mesh.vertices()(vertex_index, 2) = 0.0;
    ++vertex_index;
  }

  // Generate faces
  int face_index = 0;

  // Side faces
  for (int height_ring = 0; height_ring < height_segments; ++height_ring) {
    for (int segment = 0; segment < radial_segments; ++segment) {
      const int next_segment = (segment + 1) % radial_segments;

      const int current_ring_base = height_ring * radial_segments;
      const int next_ring_base = (height_ring + 1) * radial_segments;

      const int current_vertex = current_ring_base + segment;
      const int next_vertex = current_ring_base + next_segment;
      const int current_vertex_next_ring = next_ring_base + segment;
      const int next_vertex_next_ring = next_ring_base + next_segment;

      // First triangle
      mesh.faces()(face_index, 0) = current_vertex;
      mesh.faces()(face_index, 1) = next_vertex_next_ring;
      mesh.faces()(face_index, 2) = next_vertex;
      ++face_index;

      // Second triangle
      mesh.faces()(face_index, 0) = current_vertex;
      mesh.faces()(face_index, 1) = current_vertex_next_ring;
      mesh.faces()(face_index, 2) = next_vertex_next_ring;
      ++face_index;
    }
  }

  // Top cap faces
  if (top_cap) {
    const int top_ring_base = height_segments * radial_segments;
    for (int segment = 0; segment < radial_segments; ++segment) {
      const int next_segment = (segment + 1) % radial_segments;

      mesh.faces()(face_index, 0) = top_center;
      mesh.faces()(face_index, 1) = top_ring_base + segment;
      mesh.faces()(face_index, 2) = top_ring_base + next_segment;
      ++face_index;
    }
  }

  // Bottom cap faces
  if (bottom_cap) {
    const int bottom_ring_base = 0; // First ring
    for (int segment = 0; segment < radial_segments; ++segment) {
      const int next_segment = (segment + 1) % radial_segments;

      mesh.faces()(face_index, 0) = bottom_center;
      mesh.faces()(face_index, 1) = bottom_ring_base + next_segment;
      mesh.faces()(face_index, 2) = bottom_ring_base + segment;
      ++face_index;
    }
  }

  return mesh;
}

const core::Error &CylinderGenerator::last_error() { return last_error_; }

void CylinderGenerator::set_last_error(const core::Error &error) {
  last_error_ = error;
}

} // namespace nodeflux::geometry
