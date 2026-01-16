#pragma once

#include "nodo/core/geometry_container.hpp"
#include "nodo/core/mesh.hpp"
#include "nodo/core/standard_attributes.hpp"

namespace nodo::test {

/**
 * @brief Helper function to convert GeometryContainer to legacy Mesh
 *
 * Used by tests that still need to work with the legacy Mesh type.
 * Handles vertex-to-point mapping and triangulates quads/n-gons.
 * TODO: Remove once all components are migrated to GeometryContainer
 */
inline core::Mesh container_to_mesh(const core::GeometryContainer& container) {
  namespace attrs = core::standard_attrs;
  const auto& topology = container.topology();

  // Extract positions
  const auto* p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (!p_storage) {
    return core::Mesh(); // Empty mesh
  }

  // Build vertex matrix from points
  Eigen::MatrixXd vertices(topology.point_count(), 3);
  auto p_span = p_storage->values();
  for (size_t i = 0; i < p_span.size(); ++i) {
    vertices.row(i) = p_span[i].cast<double>();
  }

  // Extract faces and triangulate if needed
  // Convert vertex indices to point indices
  std::vector<Eigen::Vector3i> triangle_list;
  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto& vert_indices = topology.get_primitive_vertices(prim_idx);

    // Convert vertex indices to point indices
    std::vector<int> point_indices;
    for (int vert_idx : vert_indices) {
      point_indices.push_back(topology.get_vertex_point(vert_idx));
    }

    if (point_indices.size() == 3) {
      // Triangle - add directly
      triangle_list.emplace_back(point_indices[0], point_indices[1], point_indices[2]);
    } else if (point_indices.size() == 4) {
      // Quad - triangulate (fan from first vertex)
      triangle_list.emplace_back(point_indices[0], point_indices[1], point_indices[2]);
      triangle_list.emplace_back(point_indices[0], point_indices[2], point_indices[3]);
    } else if (point_indices.size() > 4) {
      // N-gon - triangulate (fan from first vertex)
      for (size_t i = 1; i < point_indices.size() - 1; ++i) {
        triangle_list.emplace_back(point_indices[0], point_indices[i], point_indices[i + 1]);
      }
    }
  }

  // Convert to Eigen matrix
  Eigen::MatrixXi faces(triangle_list.size(), 3);
  for (size_t i = 0; i < triangle_list.size(); ++i) {
    faces.row(i) = triangle_list[i];
  }

  return core::Mesh(vertices, faces);
}

} // namespace nodo::test
