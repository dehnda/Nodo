#pragma once

#include "nodeflux/core/geometry_container.hpp"
#include <Eigen/Dense>
#include <memory>
#include <stdexcept>
#include <string>

namespace nodeflux::sop {

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
inline Eigen::MatrixXd get_positions(const core::GeometryContainer &container) {
  const auto &point_attrs = container.get_point_attributes();

  if (!point_attrs.has_attribute("P")) {
    return Eigen::MatrixXd(); // Return empty matrix
  }

  const auto *pos_attr = point_attrs.get_attribute<Eigen::Vector3f>("P");
  if (pos_attr == nullptr) {
    return Eigen::MatrixXd();
  }

  const auto &positions = pos_attr->get_data();
  const size_t num_points = positions.size();

  Eigen::MatrixXd result(static_cast<Eigen::Index>(num_points), 3);
  for (size_t i = 0; i < num_points; ++i) {
    result.row(static_cast<Eigen::Index>(i)) =
        positions[i].cast<double>().transpose();
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
inline void set_positions(core::GeometryContainer &container,
                          const Eigen::MatrixXd &positions) {
  const size_t num_points = static_cast<size_t>(positions.rows());

  // Get or create position attribute
  auto &point_attrs = container.get_point_attributes();

  // Create attribute if it doesn't exist
  if (!point_attrs.has_attribute("P")) {
    point_attrs.add_attribute<Eigen::Vector3f>("P");
  }

  auto *pos_attr = point_attrs.get_attribute<Eigen::Vector3f>("P");
  if (pos_attr == nullptr) {
    throw std::runtime_error("Failed to create position attribute");
  }

  // Resize to match number of points
  pos_attr->resize(num_points);

  // Copy data
  auto &pos_data = pos_attr->get_data();
  for (size_t i = 0; i < num_points; ++i) {
    pos_data[i] =
        positions.row(static_cast<Eigen::Index>(i)).cast<float>().transpose();
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
inline void compute_face_normals(core::GeometryContainer &container) {
  const auto &topology = container.get_topology();
  const auto &point_attrs = container.get_point_attributes();

  // Get positions
  if (!point_attrs.has_attribute("P")) {
    throw std::runtime_error("Cannot compute normals: no position attribute");
  }

  const auto *pos_attr = point_attrs.get_attribute<Eigen::Vector3f>("P");
  if (pos_attr == nullptr) {
    throw std::runtime_error("Position attribute has wrong type");
  }

  const auto &positions = pos_attr->get_data();
  const size_t num_prims = topology.num_primitives();

  // Create or get primitive normal attribute
  auto &prim_attrs = container.get_primitive_attributes();
  if (!prim_attrs.has_attribute("N")) {
    prim_attrs.add_attribute<Eigen::Vector3f>("N");
  }

  auto *normal_attr = prim_attrs.get_attribute<Eigen::Vector3f>("N");
  if (normal_attr == nullptr) {
    throw std::runtime_error("Failed to create normal attribute");
  }

  normal_attr->resize(num_prims);
  auto &normals = normal_attr->get_data();

  // Compute face normals
  for (size_t prim_idx = 0; prim_idx < num_prims; ++prim_idx) {
    const auto vertex_range = topology.get_primitive_vertices(prim_idx);
    const size_t num_verts = vertex_range.second - vertex_range.first;

    if (num_verts < 3) {
      normals[prim_idx] = Eigen::Vector3f::Zero();
      continue;
    }

    // Get first three vertices to compute normal
    const auto v0_idx = topology.get_vertex_point(vertex_range.first);
    const auto v1_idx = topology.get_vertex_point(vertex_range.first + 1);
    const auto v2_idx = topology.get_vertex_point(vertex_range.first + 2);

    const Eigen::Vector3f &p0 = positions[v0_idx];
    const Eigen::Vector3f &p1 = positions[v1_idx];
    const Eigen::Vector3f &p2 = positions[v2_idx];

    const Eigen::Vector3f edge1 = p1 - p0;
    const Eigen::Vector3f edge2 = p2 - p0;

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
inline void compute_vertex_normals(core::GeometryContainer &container,
                                   bool normalize = true) {
  const auto &topology = container.get_topology();
  const auto &point_attrs = container.get_point_attributes();

  // Get positions
  if (!point_attrs.has_attribute("P")) {
    throw std::runtime_error("Cannot compute normals: no position attribute");
  }

  const auto *pos_attr = point_attrs.get_attribute<Eigen::Vector3f>("P");
  if (pos_attr == nullptr) {
    throw std::runtime_error("Position attribute has wrong type");
  }

  const size_t num_points = topology.num_points();
  const size_t num_prims = topology.num_primitives();

  // Create or get point normal attribute
  auto &point_attrs_mut = container.get_point_attributes();
  if (!point_attrs_mut.has_attribute("N")) {
    point_attrs_mut.add_attribute<Eigen::Vector3f>("N");
  }

  auto *normal_attr = point_attrs_mut.get_attribute<Eigen::Vector3f>("N");
  if (normal_attr == nullptr) {
    throw std::runtime_error("Failed to create normal attribute");
  }

  normal_attr->resize(num_points);
  auto &normals = normal_attr->get_data();

  // Initialize to zero
  for (size_t i = 0; i < num_points; ++i) {
    normals[i] = Eigen::Vector3f::Zero();
  }

  const auto &positions = pos_attr->get_data();

  // Accumulate face normals to vertices (area-weighted)
  for (size_t prim_idx = 0; prim_idx < num_prims; ++prim_idx) {
    const auto vertex_range = topology.get_primitive_vertices(prim_idx);
    const size_t num_verts = vertex_range.second - vertex_range.first;

    if (num_verts < 3) {
      continue; // Skip degenerate primitives
    }

    // Compute face normal
    const auto v0_idx = topology.get_vertex_point(vertex_range.first);
    const auto v1_idx = topology.get_vertex_point(vertex_range.first + 1);
    const auto v2_idx = topology.get_vertex_point(vertex_range.first + 2);

    const Eigen::Vector3f &p0 = positions[v0_idx];
    const Eigen::Vector3f &p1 = positions[v1_idx];
    const Eigen::Vector3f &p2 = positions[v2_idx];

    const Eigen::Vector3f edge1 = p1 - p0;
    const Eigen::Vector3f edge2 = p2 - p0;
    const Eigen::Vector3f face_normal = edge1.cross(edge2);

    // Add face normal to all vertices of this primitive
    for (size_t v = vertex_range.first; v < vertex_range.second; ++v) {
      const auto point_idx = topology.get_vertex_point(v);
      normals[point_idx] += face_normal;
    }
  }

  // Normalize if requested
  if (normalize) {
    for (size_t i = 0; i < num_points; ++i) {
      const float length = normals[i].norm();
      if (length > 1e-8F) {
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
inline const core::TypedAttribute<Eigen::Vector3f> *
get_or_create_normals(core::GeometryContainer &container,
                      bool force_recompute = false) {
  auto &point_attrs = container.get_point_attributes();

  // Check if normals already exist
  if (!force_recompute && point_attrs.has_attribute("N")) {
    return point_attrs.get_attribute<Eigen::Vector3f>("N");
  }

  // Compute normals
  compute_vertex_normals(container, true);

  return point_attrs.get_attribute<Eigen::Vector3f>("N");
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
inline void copy_attributes(const core::GeometryContainer &src,
                            core::GeometryContainer &dst) {
  // Note: This is a placeholder for now
  // Full implementation would need AttributeSet::copy_from() or similar
  // For now, this serves as documentation of the intended utility
  (void)src;
  (void)dst;
  throw std::runtime_error(
      "copy_attributes not yet implemented - needs AttributeSet API");
}

} // namespace utils
} // namespace nodeflux::sop
