#pragma once

#include "attribute_types.hpp"
#include "geometry_container.hpp"

#include <span>
#include <vector>

namespace nodo::core {

/**
 * @brief Enhanced attribute interpolation for geometry operations
 *
 * Provides various interpolation modes for blending attribute values:
 * - LINEAR: Standard linear interpolation
 * - CUBIC: Smooth cubic interpolation (Hermite)
 * - CONSTANT: No interpolation (nearest value)
 * - WEIGHTED: Custom weighted average
 *
 * Used during:
 * - Subdivision surfaces
 * - Mesh resampling
 * - Point scattering
 * - Copy-to-points
 * - Any operation that creates new geometry elements
 */

/**
 * @brief Interpolate between two attribute values
 *
 * @param a First value
 * @param b Second value
 * @param t Interpolation factor [0,1], where 0=a, 1=b
 * @param mode Interpolation mode (from attribute descriptor)
 * @return Interpolated value
 */
template <typename T>
T interpolate_linear(const T& a, const T& b, float t);

/**
 * @brief Interpolate between two values with cubic smoothing
 *
 * Uses Hermite interpolation for smooth transitions.
 */
template <typename T>
T interpolate_cubic(const T& a, const T& b, float t);

/**
 * @brief Weighted average of multiple values
 *
 * @param values Array of values to blend
 * @param weights Corresponding weights (must sum to 1.0)
 * @return Weighted average
 */
template <typename T>
T interpolate_weighted(std::span<const T> values,
                       std::span<const float> weights);

/**
 * @brief Interpolate between three values (triangle)
 *
 * @param v0, v1, v2 Triangle vertex values
 * @param u, v Barycentric coordinates (u+v <= 1)
 * @return Interpolated value at (u,v)
 */
template <typename T>
T interpolate_barycentric(const T& v0, const T& v1, const T& v2, float u,
                          float v);

/**
 * @brief Interpolate between four values (quad)
 *
 * @param v00, v10, v01, v11 Quad corner values
 * @param u, v Parametric coordinates [0,1]
 * @return Bilinearly interpolated value
 */
template <typename T>
T interpolate_bilinear(const T& v00, const T& v10, const T& v01, const T& v11,
                       float u, float v);

/**
 * @brief Sample attribute at a fractional primitive location
 *
 * Given a primitive index and parametric coordinates, interpolate
 * the attribute value at that location.
 *
 * @param container Geometry container
 * @param attr_name Attribute to sample
 * @param element_class Which element class holds the attribute
 * @param prim_index Primitive index
 * @param u, v Parametric coordinates on the primitive
 * @return Interpolated attribute value
 */
template <typename T>
T sample_attribute_at_location(const GeometryContainer& container,
                               std::string_view attr_name,
                               ElementClass element_class, size_t prim_index,
                               float u, float v = 0.0f);

/**
 * @brief Blend attributes from multiple sources
 *
 * Useful for operations like:
 * - Mesh merging (blend overlapping regions)
 * - Vertex welding (average attributes at merged points)
 * - Point cloud blending
 *
 * @param container Geometry container
 * @param attr_name Attribute to blend
 * @param element_class Element class
 * @param source_indices Indices of elements to blend
 * @param target_index Index where blended result is stored
 * @param weights Optional weights (if empty, uses equal weighting)
 */
template <typename T>
bool blend_attributes(GeometryContainer& container, std::string_view attr_name,
                      ElementClass element_class,
                      const std::vector<size_t>& source_indices,
                      size_t target_index,
                      const std::vector<float>& weights = {});

/**
 * @brief Copy and interpolate attributes from source to target
 *
 * When creating new geometry, this copies all attributes from source
 * elements and interpolates them to the target element.
 *
 * @param container Geometry container
 * @param element_class Element class
 * @param source_indices Source element indices
 * @param target_index Target element index
 * @param weights Interpolation weights
 */
bool copy_and_interpolate_all_attributes(
    GeometryContainer& container, ElementClass element_class,
    const std::vector<size_t>& source_indices, size_t target_index,
    const std::vector<float>& weights);

/**
 * @brief Transfer attributes from points to a new primitive
 *
 * When creating a new primitive, interpolate point attributes to
 * the primitive (e.g., average point colors to face color).
 *
 * @param container Geometry container
 * @param point_indices Indices of points forming the primitive
 * @param prim_index Index of the primitive to populate
 */
bool transfer_point_to_primitive_attributes(
    GeometryContainer& container, const std::vector<int>& point_indices,
    size_t prim_index);

/**
 * @brief Resample an attribute along a curve
 *
 * Given a curve (sequence of points) and a target position along it,
 * interpolate the attribute value at that position.
 *
 * @param container Geometry container
 * @param attr_name Attribute to sample
 * @param point_indices Ordered points forming the curve
 * @param t Parametric position along curve [0,1]
 * @return Interpolated value
 */
template <typename T>
T resample_curve_attribute(const GeometryContainer& container,
                           std::string_view attr_name,
                           const std::vector<int>& point_indices, float t);

// ============================================================================
// Specialized interpolation for specific types
// ============================================================================

/**
 * @brief Spherical linear interpolation for quaternions/rotations
 *
 * Properly interpolates rotations without gimbal lock.
 */
Vec4f slerp(const Vec4f& q0, const Vec4f& q1, float t);

/**
 * @brief Normalize interpolated normals
 *
 * After interpolation, normals should be renormalized.
 */
Vec3f interpolate_normal(const Vec3f& n0, const Vec3f& n1, float t);

/**
 * @brief Interpolate colors in perceptually linear space
 *
 * Optional: Convert to linear RGB before interpolation, then back to sRGB.
 */
Vec3f interpolate_color(const Vec3f& c0, const Vec3f& c1, float t,
                        bool linearize = false);

/**
 * @brief Clamp interpolated values to valid range
 *
 * Useful for attributes like alpha [0,1] or material IDs [integers only].
 */
template <typename T>
T interpolate_clamped(const T& a, const T& b, float t, const T& min_val,
                      const T& max_val);

// ============================================================================
// Helper functions
// ============================================================================

/**
 * @brief Compute smooth interpolation factor (ease-in-ease-out)
 *
 * Converts linear t to smooth curve using 3t² - 2t³
 */
inline float smoothstep(float t) {
  if (t <= 0.0f)
    return 0.0f;
  if (t >= 1.0f)
    return 1.0f;
  return t * t * (3.0f - 2.0f * t);
}

/**
 * @brief Compute smooth interpolation with custom power
 */
inline float smootherstep(float t) {
  if (t <= 0.0f)
    return 0.0f;
  if (t >= 1.0f)
    return 1.0f;
  return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

/**
 * @brief Clamp value to range [0, 1]
 */
inline float saturate(float x) {
  return std::max(0.0f, std::min(1.0f, x));
}

} // namespace nodo::core
