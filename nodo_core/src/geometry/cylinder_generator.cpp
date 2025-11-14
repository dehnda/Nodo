#include "nodo/geometry/cylinder_generator.hpp"

#include <cmath>
#include <numbers>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::geometry {

// Thread-local storage for error reporting
thread_local core::Error CylinderGenerator::last_error_{
    core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"};

std::optional<core::GeometryContainer>
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

  // Calculate vertices and faces count
  const int ring_vertices = (height_segments + 1) * radial_segments;
  const int cap_vertices = (top_cap ? 1 : 0) + (bottom_cap ? 1 : 0);
  const int total_vertices = ring_vertices + cap_vertices;

  const int side_faces = height_segments * radial_segments * 2;
  const int cap_faces =
      (top_cap ? radial_segments : 0) + (bottom_cap ? radial_segments : 0);
  const int total_faces = side_faces + cap_faces;

  // Create GeometryContainer
  core::GeometryContainer container;
  container.set_point_count(total_vertices);
  container.set_vertex_count(total_vertices); // 1:1 mapping for cylinder

  auto& topology = container.topology();

  // Storage for positions
  std::vector<core::Vec3f> positions;
  positions.reserve(total_vertices);
  std::vector<std::vector<int>> primitive_vertices;
  primitive_vertices.reserve(total_faces);

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

      positions.push_back({static_cast<float>(coord_x),
                           static_cast<float>(coord_y),
                           static_cast<float>(coord_z)});
      ++vertex_index;
    }
  }

  // Cap centers
  int top_center = -1;
  int bottom_center = -1;

  if (top_cap) {
    top_center = vertex_index;
    positions.push_back({0.0F, static_cast<float>(height * 0.5), 0.0F});
    ++vertex_index;
  }

  if (bottom_cap) {
    bottom_center = vertex_index;
    positions.push_back({0.0F, static_cast<float>(-height * 0.5), 0.0F});
    ++vertex_index;
  }

  // Generate faces
  // Side faces (quads)
  for (int height_ring = 0; height_ring < height_segments; ++height_ring) {
    for (int segment = 0; segment < radial_segments; ++segment) {
      const int next_segment = (segment + 1) % radial_segments;

      const int current_ring_base = height_ring * radial_segments;
      const int next_ring_base = (height_ring + 1) * radial_segments;

      const int current_vertex = current_ring_base + segment;
      const int next_vertex = current_ring_base + next_segment;
      const int current_vertex_next_ring = next_ring_base + segment;
      const int next_vertex_next_ring = next_ring_base + next_segment;

      // Single quad (counter-clockwise winding)
      primitive_vertices.push_back({current_vertex, next_vertex,
                                    next_vertex_next_ring,
                                    current_vertex_next_ring});
    }
  }

  // Top cap faces
  if (top_cap) {
    const int top_ring_base = height_segments * radial_segments;
    for (int segment = 0; segment < radial_segments; ++segment) {
      const int next_segment = (segment + 1) % radial_segments;

      // Reversed winding: center, next, current (CCW from top view)
      primitive_vertices.push_back(
          {top_center, top_ring_base + next_segment, top_ring_base + segment});
    }
  }

  // Bottom cap faces
  if (bottom_cap) {
    const int bottom_ring_base = 0; // First ring
    for (int segment = 0; segment < radial_segments; ++segment) {
      const int next_segment = (segment + 1) % radial_segments;

      // Reversed winding: center, current, next (CCW from bottom view)
      primitive_vertices.push_back({bottom_center, bottom_ring_base + segment,
                                    bottom_ring_base + next_segment});
    }
  }

  // Set up 1:1 vertex→point mapping
  for (size_t i = 0; i < static_cast<size_t>(total_vertices); ++i) {
    topology.set_vertex_point(i, static_cast<int>(i));
  }

  // Add primitives to topology
  for (const auto& prim_verts : primitive_vertices) {
    topology.add_primitive(prim_verts);
  }

  // Add P (position) attribute
  container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto* p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage != nullptr) {
    auto p_span = p_storage->values_writable();
    std::copy(positions.begin(), positions.end(), p_span.begin());
  }

  // Calculate and add normals
  container.add_point_attribute(attrs::N, core::AttributeType::VEC3F);
  auto* n_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::N);
  if (n_storage != nullptr) {
    auto n_span = n_storage->values_writable();

    // For cylinders, side normals point outward from the axis
    // Ring vertices (excluding cap centers)
    int idx = 0;
    for (int height_ring = 0; height_ring <= height_segments; ++height_ring) {
      for (int segment = 0; segment < radial_segments; ++segment) {
        const double angle = 2.0 * std::numbers::pi *
                             static_cast<double>(segment) /
                             static_cast<double>(radial_segments);
        // Normal points radially outward (Y component is 0 for straight
        // cylinder)
        core::Vec3f normal = {static_cast<float>(std::cos(angle)), 0.0F,
                              static_cast<float>(std::sin(angle))};
        n_span[idx++] = normal;
      }
    }

    // Top cap center normal (pointing up)
    if (top_cap) {
      n_span[idx++] = {0.0F, 1.0F, 0.0F};
    }

    // Bottom cap center normal (pointing down)
    if (bottom_cap) {
      n_span[idx++] = {0.0F, -1.0F, 0.0F};
    }
  }

  return container;
}

const core::Error& CylinderGenerator::last_error() {
  return last_error_;
}

void CylinderGenerator::set_last_error(const core::Error& error) {
  last_error_ = error;
}

} // namespace nodo::geometry
