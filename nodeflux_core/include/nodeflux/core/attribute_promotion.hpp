#pragma once

#include "attribute_types.hpp"
#include "geometry_container.hpp"
#include <stdexcept>

namespace nodeflux::core {

/**
 * @file attribute_promotion.hpp
 * @brief Attribute promotion and demotion utilities
 *
 * These functions convert attributes between different element classes:
 * - Promotion: Lower class → Higher class (e.g., Point → Vertex)
 * - Demotion: Higher class → Lower class (e.g., Vertex → Point)
 *
 * Hierarchy: Detail → Point → Vertex → Primitive
 */

/**
 * @brief Promote a point attribute to vertex attribute (replicate)
 *
 * Each vertex gets a copy of its referenced point's attribute value.
 * This is useful when you need per-vertex data but only have per-point data.
 *
 * Example: Point colors → Vertex colors (for split UVs)
 *
 * @param container Geometry container to operate on
 * @param attr_name Name of the point attribute to promote
 * @param output_name Name for the new vertex attribute (defaults to same name)
 * @return true if successful, false if attribute doesn't exist or wrong type
 */
bool promote_point_to_vertex(GeometryContainer &container,
                             std::string_view attr_name,
                             std::string_view output_name = "");

/**
 * @brief Demote a vertex attribute to point attribute (average)
 *
 * For each point, average all vertex attribute values that reference it.
 * This is useful for smoothing vertex data back to shared point data.
 *
 * Example: Split vertex normals → Smooth point normals
 *
 * @param container Geometry container to operate on
 * @param attr_name Name of the vertex attribute to demote
 * @param output_name Name for the new point attribute (defaults to same name)
 * @return true if successful, false if attribute doesn't exist or wrong type
 */
bool demote_vertex_to_point(GeometryContainer &container,
                            std::string_view attr_name,
                            std::string_view output_name = "");

/**
 * @brief Promote a point attribute to primitive attribute (average)
 *
 * For each primitive, average the attribute values of all its points.
 * This is useful for converting per-point data to per-face data.
 *
 * Example: Point density → Face density
 *
 * @param container Geometry container to operate on
 * @param attr_name Name of the point attribute to promote
 * @param output_name Name for the new primitive attribute (defaults to same
 * name)
 * @return true if successful, false if attribute doesn't exist or wrong type
 */
bool promote_point_to_primitive(GeometryContainer &container,
                                std::string_view attr_name,
                                std::string_view output_name = "");

/**
 * @brief Demote a primitive attribute to point attribute (splat/distribute)
 *
 * For each point, average all primitive attribute values that reference it.
 * This distributes face-level data to points.
 *
 * Example: Face material ID → Point group membership
 *
 * @param container Geometry container to operate on
 * @param attr_name Name of the primitive attribute to demote
 * @param output_name Name for the new point attribute (defaults to same name)
 * @return true if successful, false if attribute doesn't exist or wrong type
 */
bool demote_primitive_to_point(GeometryContainer &container,
                               std::string_view attr_name,
                               std::string_view output_name = "");

/**
 * @brief Promote a vertex attribute to primitive attribute (average)
 *
 * For each primitive, average the attribute values of all its vertices.
 *
 * Example: Vertex colors → Face colors
 *
 * @param container Geometry container to operate on
 * @param attr_name Name of the vertex attribute to promote
 * @param output_name Name for the new primitive attribute (defaults to same
 * name)
 * @return true if successful, false if attribute doesn't exist or wrong type
 */
bool promote_vertex_to_primitive(GeometryContainer &container,
                                 std::string_view attr_name,
                                 std::string_view output_name = "");

/**
 * @brief Demote a primitive attribute to vertex attribute (replicate)
 *
 * Each vertex gets a copy of its primitive's attribute value.
 *
 * Example: Face normals → Vertex normals (for flat shading)
 *
 * @param container Geometry container to operate on
 * @param attr_name Name of the primitive attribute to demote
 * @param output_name Name for the new vertex attribute (defaults to same name)
 * @return true if successful, false if attribute doesn't exist or wrong type
 */
bool demote_primitive_to_vertex(GeometryContainer &container,
                                std::string_view attr_name,
                                std::string_view output_name = "");

// ============================================================================
// Template implementations for type-safe promotion/demotion
// ============================================================================

namespace detail {

/**
 * @brief Helper to average values (supports float, Vec2f, Vec3f, Vec4f)
 */
template <typename T> T average_values(const std::vector<T> &values) {
  if (values.empty()) {
    return T{}; // Zero/default
  }

  T sum = values[0];
  for (size_t i = 1; i < values.size(); ++i) {
    sum = sum + values[i];
  }

  // Divide by count
  if constexpr (std::is_same_v<T, float>) {
    return sum / static_cast<float>(values.size());
  } else {
    // Vec2f, Vec3f, Vec4f have operator/
    return sum / static_cast<float>(values.size());
  }
}

/**
 * @brief Specialization for int (use majority vote or first value)
 */
template <> inline int average_values<int>(const std::vector<int> &values) {
  if (values.empty()) {
    return 0;
  }
  // For integers, compute average and round
  int sum = 0;
  for (int v : values) {
    sum += v;
  }
  return sum / static_cast<int>(values.size());
}

} // namespace detail

// Template function declarations for typed promotion/demotion
template <typename T>
bool promote_point_to_vertex_typed(GeometryContainer &container,
                                   std::string_view attr_name,
                                   std::string_view output_name);

template <typename T>
bool demote_vertex_to_point_typed(GeometryContainer &container,
                                  std::string_view attr_name,
                                  std::string_view output_name);

template <typename T>
bool promote_point_to_primitive_typed(GeometryContainer &container,
                                      std::string_view attr_name,
                                      std::string_view output_name);

template <typename T>
bool demote_primitive_to_point_typed(GeometryContainer &container,
                                     std::string_view attr_name,
                                     std::string_view output_name);

template <typename T>
bool promote_vertex_to_primitive_typed(GeometryContainer &container,
                                       std::string_view attr_name,
                                       std::string_view output_name);

} // namespace nodeflux::core
