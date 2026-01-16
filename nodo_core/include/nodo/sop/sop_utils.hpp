#pragma once

#include "nodo/core/attribute_storage.hpp"
#include "nodo/core/attribute_types.hpp"
#include "nodo/core/geometry_container.hpp"

#include <Eigen/Dense>

#include <memory>
#include <stdexcept>
#include <string>

namespace nodo::sop {

/**
 * @brief Utility functions for working with GeometryContainer in SOPs
 *
 * These functions provide common operations that SOP nodes frequently need,
 * avoiding code duplication and providing a consistent API.
 */
namespace utils {

/**
 * @brief Extract point positions as an Eigen matrix
 *
 * @param container The geometry container
 * @return Eigen::MatrixXd with shape (num_points, 3), or empty matrix if no
 * positions
 *
 * This is useful for algorithms that work with position matrices directly
 * (e.g., matrix operations, transformations, spatial queries).
 */
inline Eigen::MatrixXd get_positions(const core::GeometryContainer& container) {
  if (!container.has_point_attribute("P")) {
    return Eigen::MatrixXd(); // Return empty matrix
  }

  const auto* pos_attr = container.get_point_attribute_typed<Eigen::Vector3f>("P");
  if (pos_attr == nullptr) {
    return Eigen::MatrixXd();
  }

  auto positions = pos_attr->values();
  const size_t num_points = positions.size();

  Eigen::MatrixXd result(static_cast<Eigen::Index>(num_points), 3);
  for (size_t i = 0; i < num_points; ++i) {
    result.row(static_cast<Eigen::Index>(i)) = positions[i].template cast<double>().transpose();
  }

  return result;
}

/**
 * @brief Set point positions from an Eigen matrix
 *
 * @param container The geometry container to modify
 * @param positions Matrix with shape (num_points, 3)
 *
 * Creates or updates the "P" point attribute with the provided positions.
 */
inline void set_positions(core::GeometryContainer& container, const Eigen::MatrixXd& positions) {
  const size_t num_points = static_cast<size_t>(positions.rows());

  // Create attribute if it doesn't exist
  if (!container.has_point_attribute("P")) {
    container.add_point_attribute("P", core::AttributeType::VEC3F);
  }

  auto* pos_attr = container.get_point_attribute_typed<Eigen::Vector3f>("P");
  if (pos_attr == nullptr) {
    throw std::runtime_error("Failed to create position attribute");
  }

  // Copy data
  auto pos_writable = pos_attr->values_writable();
  for (size_t i = 0; i < num_points; ++i) {
    pos_writable[i] = positions.row(static_cast<Eigen::Index>(i)).template cast<float>().transpose();
  }
}

/**
 * @brief Compute face normals and store as primitive attribute "N"
 *
 * @param container The geometry container
 *
 * Computes per-face normals using cross product of edge vectors.
 * Normals are NOT normalized by default (magnitude = 2 * triangle area).
 * Stores result in primitive attribute "N" (Eigen::Vector3f).
 */
inline void compute_face_normals(core::GeometryContainer& container) {
  const auto& topology = container.topology();

  // Get positions
  if (!container.has_point_attribute("P")) {
    throw std::runtime_error("Cannot compute normals: no position attribute");
  }

  const auto* pos_attr = container.get_point_attribute_typed<Eigen::Vector3f>("P");
  if (pos_attr == nullptr) {
    throw std::runtime_error("Position attribute has wrong type");
  }

  auto positions = pos_attr->values();
  const size_t num_prims = topology.primitive_count();

  // Create or get primitive normal attribute
  if (!container.has_primitive_attribute("N")) {
    container.add_primitive_attribute("N", core::AttributeType::VEC3F);
  }

  auto* normal_attr = container.get_primitive_attribute_typed<Eigen::Vector3f>("N");
  if (normal_attr == nullptr) {
    throw std::runtime_error("Failed to create normal attribute");
  }

  auto normals = normal_attr->values_writable();

  // Compute face normals
  for (size_t prim_idx = 0; prim_idx < num_prims; ++prim_idx) {
    const auto& prim_verts = topology.get_primitive_vertices(prim_idx);
    const size_t num_verts = prim_verts.size();

    if (num_verts < 3) {
      normals[prim_idx] = Eigen::Vector3f::Zero();
      continue;
    }

    // Get first three vertices to compute normal
    const auto v0_idx = topology.get_vertex_point(prim_verts[0]);
    const auto v1_idx = topology.get_vertex_point(prim_verts[1]);
    const auto v2_idx = topology.get_vertex_point(prim_verts[2]);

    const Eigen::Vector3f& pos0 = positions[v0_idx];
    const Eigen::Vector3f& pos1 = positions[v1_idx];
    const Eigen::Vector3f& pos2 = positions[v2_idx];

    const Eigen::Vector3f edge1 = pos1 - pos0;
    const Eigen::Vector3f edge2 = pos2 - pos0;

    // Cross product (not normalized - magnitude = 2 * area)
    normals[prim_idx] = edge1.cross(edge2);
  }
}

/**
 * @brief Compute vertex normals and store as point attribute "N"
 *
 * @param container The geometry container
 * @param normalize If true, normalize the result (default: true)
 *
 * Computes per-vertex normals by averaging face normals.
 * Uses area-weighted averaging (unnormalized face normals).
 * Stores result in point attribute "N" (Eigen::Vector3f).
 */
inline void compute_vertex_normals(core::GeometryContainer& container, bool normalize = true) {
  const auto& topology = container.topology();

  // Get positions
  if (!container.has_point_attribute("P")) {
    throw std::runtime_error("Cannot compute normals: no position attribute");
  }

  const auto* pos_attr = container.get_point_attribute_typed<Eigen::Vector3f>("P");
  if (pos_attr == nullptr) {
    throw std::runtime_error("Position attribute has wrong type");
  }

  auto positions = pos_attr->values();
  const size_t num_points = topology.point_count();
  const size_t num_prims = topology.primitive_count();

  // Create or get point normal attribute
  if (!container.has_point_attribute("N")) {
    container.add_point_attribute("N", core::AttributeType::VEC3F);
  }

  auto* normal_attr = container.get_point_attribute_typed<Eigen::Vector3f>("N");
  if (normal_attr == nullptr) {
    throw std::runtime_error("Failed to create normal attribute");
  }

  auto normals = normal_attr->values_writable();

  // Initialize to zero
  for (size_t i = 0; i < num_points; ++i) {
    normals[i] = Eigen::Vector3f::Zero();
  }

  // Accumulate face normals to vertices (area-weighted)
  for (size_t prim_idx = 0; prim_idx < num_prims; ++prim_idx) {
    const auto& prim_verts = topology.get_primitive_vertices(prim_idx);
    const size_t num_verts = prim_verts.size();

    if (num_verts < 3) {
      continue; // Skip degenerate primitives
    }

    // Compute face normal
    const auto v0_idx = topology.get_vertex_point(prim_verts[0]);
    const auto v1_idx = topology.get_vertex_point(prim_verts[1]);
    const auto v2_idx = topology.get_vertex_point(prim_verts[2]);

    const Eigen::Vector3f& pos0 = positions[v0_idx];
    const Eigen::Vector3f& pos1 = positions[v1_idx];
    const Eigen::Vector3f& pos2 = positions[v2_idx];

    const Eigen::Vector3f edge1 = pos1 - pos0;
    const Eigen::Vector3f edge2 = pos2 - pos0;
    const Eigen::Vector3f face_normal = edge1.cross(edge2);

    // Add face normal to all vertices of this primitive
    for (size_t vert_idx = 0; vert_idx < num_verts; ++vert_idx) {
      const auto point_idx = topology.get_vertex_point(prim_verts[vert_idx]);
      normals[point_idx] += face_normal;
    }
  }

  // Normalize if requested
  constexpr float epsilon = 1e-8F;
  if (normalize) {
    for (size_t i = 0; i < num_points; ++i) {
      const float length = normals[i].norm();
      if (length > epsilon) {
        normals[i] /= length;
      } else {
        normals[i] = Eigen::Vector3f::UnitZ(); // Default to +Z
      }
    }
  }
}

/**
 * @brief Get or compute vertex normals
 *
 * @param container The geometry container
 * @param force_recompute If true, always recompute (default: false)
 * @return Pointer to the normal attribute
 *
 * If point attribute "N" exists and force_recompute is false, returns it.
 * Otherwise, computes vertex normals and stores them.
 */
inline core::AttributeStorage<Eigen::Vector3f>* get_or_create_normals(core::GeometryContainer& container,
                                                                      bool force_recompute = false) {
  // Check if normals already exist
  if (!force_recompute && container.has_point_attribute("N")) {
    return container.get_point_attribute_typed<Eigen::Vector3f>("N");
  }

  // Compute normals
  compute_vertex_normals(container, true);

  return container.get_point_attribute_typed<Eigen::Vector3f>("N");
}

/**
 * @brief Compute hard edge normals by splitting vertices
 *
 * @param container The geometry container to modify
 * @param normalize If true, normalize the normals (default: true)
 *
 * Creates hard edges (faceted look) by splitting vertices so each face corner
 * has its own normal. This is the opposite of smooth shading.
 *
 * Process:
 * 1. For each primitive, compute its face normal
 * 2. Create new vertices (one per face corner)
 * 3. Each new vertex points to the same point but has unique normal
 * 4. Normals are stored as VERTEX attributes (not point attributes)
 * 5. Primitives are updated to reference the new split vertices
 *
 * Key difference from smooth normals (compute_vertex_normals):
 * - Smooth: Normals are POINT attributes (shared/averaged across faces)
 * - Hard: Normals are VERTEX attributes (unique per face corner)
 *
 * Example result:
 * - A cube corner (1 point, 3 adjacent faces) becomes 3 vertices
 * - Each vertex points to the same point position
 * - Each vertex has its own normal (the face normal)
 * - Renderer sees sharp edges because normals aren't averaged
 *
 * After this operation:
 * - Points remain the same (positions unchanged)
 * - Vertices are duplicated (one per face corner)
 * - Each vertex gets the face normal as a vertex attribute
 * - Topology is rebuilt with new vertex references
 */
inline void compute_hard_edge_normals(core::GeometryContainer& container, bool normalize = true) {
  const auto& old_topology = container.topology();

  // Get positions
  if (!container.has_point_attribute("P")) {
    throw std::runtime_error("Cannot compute normals: no position attribute");
  }

  const auto* pos_attr = container.get_point_attribute_typed<Eigen::Vector3f>("P");
  if (pos_attr == nullptr) {
    throw std::runtime_error("Position attribute has wrong type");
  }

  auto positions = pos_attr->values();
  const size_t num_prims = old_topology.primitive_count();

  // Count total vertices needed (one per face corner)
  size_t total_vertices = 0;
  for (size_t prim_idx = 0; prim_idx < num_prims; ++prim_idx) {
    total_vertices += old_topology.get_primitive_vertex_count(prim_idx);
  }

  // Save old topology data before we modify it
  std::vector<std::vector<int>> old_prim_vertices;
  old_prim_vertices.reserve(num_prims);
  for (size_t prim_idx = 0; prim_idx < num_prims; ++prim_idx) {
    old_prim_vertices.push_back(old_topology.get_primitive_vertices(prim_idx));
  }

  // Save old vertex-to-point mapping
  std::vector<int> old_vertex_points;
  const auto old_vertex_points_span = old_topology.get_vertex_points();
  old_vertex_points.assign(old_vertex_points_span.begin(), old_vertex_points_span.end());

  const size_t num_points = old_topology.point_count();

  // Build new topology
  auto& topology = container.topology();
  topology.clear();
  topology.set_point_count(num_points);      // Points unchanged
  topology.set_vertex_count(total_vertices); // More vertices now
  topology.reserve_primitives(num_prims);

  // Create vertex normal storage
  std::vector<Eigen::Vector3f> vertex_normals;
  vertex_normals.reserve(total_vertices);

  size_t new_vertex_idx = 0;

  // Process each primitive
  for (size_t prim_idx = 0; prim_idx < num_prims; ++prim_idx) {
    const auto& old_prim_verts = old_prim_vertices[prim_idx];
    const size_t num_verts = old_prim_verts.size();

    if (num_verts < 3) {
      // Degenerate primitive - skip or handle gracefully
      std::vector<int> new_prim_verts;
      for (size_t i = 0; i < num_verts; ++i) {
        const auto old_vertex_idx = old_prim_verts[i];
        const auto point_idx = old_vertex_points[old_vertex_idx];
        topology.set_vertex_point(new_vertex_idx, point_idx);
        vertex_normals.push_back(Eigen::Vector3f::UnitZ());
        new_prim_verts.push_back(static_cast<int>(new_vertex_idx));
        new_vertex_idx++;
      }
      topology.add_primitive(new_prim_verts);
      continue;
    }

    // Compute face normal
    const auto v0_idx = old_vertex_points[old_prim_verts[0]];
    const auto v1_idx = old_vertex_points[old_prim_verts[1]];
    const auto v2_idx = old_vertex_points[old_prim_verts[2]];

    const Eigen::Vector3f& pos0 = positions[v0_idx];
    const Eigen::Vector3f& pos1 = positions[v1_idx];
    const Eigen::Vector3f& pos2 = positions[v2_idx];

    const Eigen::Vector3f edge1 = pos1 - pos0;
    const Eigen::Vector3f edge2 = pos2 - pos0;
    Eigen::Vector3f face_normal = edge1.cross(edge2);

    // Normalize if requested
    if (normalize) {
      const float length = face_normal.norm();
      constexpr float epsilon = 1e-8F;
      if (length > epsilon) {
        face_normal /= length;
      } else {
        face_normal = Eigen::Vector3f::UnitZ(); // Default for degenerate faces
      }
    }

    // Create new vertices for this primitive (one per corner)
    std::vector<int> new_prim_verts;
    new_prim_verts.reserve(num_verts);

    for (size_t i = 0; i < num_verts; ++i) {
      const auto old_vertex_idx = old_prim_verts[i];
      const auto point_idx = old_vertex_points[old_vertex_idx];

      // Create new vertex pointing to same point
      topology.set_vertex_point(new_vertex_idx, point_idx);

      // Assign face normal to this vertex
      vertex_normals.push_back(face_normal);

      new_prim_verts.push_back(static_cast<int>(new_vertex_idx));
      new_vertex_idx++;
    }

    // Add primitive with new vertex indices
    topology.add_primitive(new_prim_verts);
  }

  // Update vertex attribute count
  container.set_vertex_count(total_vertices);

  // Store normals as VERTEX attribute (not point attribute!)
  if (!container.has_vertex_attribute("N")) {
    container.add_vertex_attribute("N", core::AttributeType::VEC3F);
  }

  auto* normal_attr = container.get_vertex_attribute_typed<Eigen::Vector3f>("N");
  if (normal_attr == nullptr) {
    throw std::runtime_error("Failed to create vertex normal attribute");
  }

  auto normals_writable = normal_attr->values_writable();
  for (size_t i = 0; i < vertex_normals.size(); ++i) {
    normals_writable[i] = vertex_normals[i];
  }
}

/**
 * @brief Copy all attributes from source to destination container
 *
 * @param src Source container
 * @param dst Destination container
 *
 * Copies point, vertex, and primitive attributes.
 * Assumes topology is compatible (same number of elements).
 */
inline void copy_attributes(const core::GeometryContainer& src, core::GeometryContainer& dst) {
  // Note: This is a placeholder for now
  // Full implementation would need AttributeSet::copy_from() or similar
  // For now, this serves as documentation of the intended utility
  (void)src;
  (void)dst;
  throw std::runtime_error("copy_attributes not yet implemented - needs AttributeSet API");
}

} // namespace utils
} // namespace nodo::sop
