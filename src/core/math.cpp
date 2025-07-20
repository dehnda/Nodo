#include "../../include/nodeflux/core/math.hpp"

namespace nodeflux::core::math {

// Mirror reflection constants
constexpr double MIRROR_REFLECTION_FACTOR = 2.0;

// Geometric calculation constants
constexpr double MIDPOINT_FACTOR = 2.0;
constexpr double TRIANGLE_VERTEX_COUNT = 3.0;

Matrix3 rotation_x(double radians) {
  double c = std::cos(radians);
  double s = std::sin(radians);
  Matrix3 result;

  // clang-format off
  result << 1,  0,  0,
            0,  c, -s,
            0,  s,  c;
  // clang-format on

  return result;
}
Matrix3 rotation_y(double radians) {
  double c = std::cos(radians);
  double s = std::sin(radians);
  Matrix3 result;

  // clang-format off
    result << c,  0,  s,
              0,  1,  0,
             -s,  0,  c;
  // clang-format on

  return result;
}
Matrix3 rotation_z(double radians) {
  double c = std::cos(radians);
  double s = std::sin(radians);
  Matrix3 result;

  // clang-format off
        result << c, -s,  0,
                  s,  c,  0,
                  0,  0,  1;
  // clang-format on

  return result;
}

Vector3 apply_rotation(const Vector3 &point, const Matrix3 &rotation) {
  return rotation * point;
}
Vector3 apply_translation(const Vector3 &point, const Vector3 &offset) {
  return point + offset;
}
Vector3 apply_transform(const Vector3 &point, const Matrix3 &rotation,
                        const Vector3 &offset) {
  return apply_translation(apply_rotation(point, rotation), offset);
}

Vector3 mirror_point_across_plane(const Vector3 &point,
                                  const Vector3 &plane_point,
                                  const Vector3 &plane_normal) {
  // Vector from plane point to the point being mirrored
  Vector3 to_point = point - plane_point;
  
  // Distance from point to plane (signed distance along normal)
  double distance = to_point.dot(plane_normal);
  
  // Mirror by reflecting across the plane
  // Formula: mirrored = point - 2 * distance * normal
  return point - MIRROR_REFLECTION_FACTOR * distance * plane_normal;
}

// ============================================================================
// Basic Geometric Utilities
// ============================================================================

Vector3 midpoint(const Vector3 &point_a, const Vector3 &point_b) {
  constexpr double MIDPOINT_FACTOR = 2.0;
  return (point_a + point_b) / MIDPOINT_FACTOR;
}

Vector3 triangle_centroid(const Vector3 &vertex_a, const Vector3 &vertex_b, const Vector3 &vertex_c) {
  constexpr double TRIANGLE_VERTEX_COUNT = 3.0;
  return (vertex_a + vertex_b + vertex_c) / TRIANGLE_VERTEX_COUNT;
}

// ============================================================================
// Circular/Polar Utilities
// ============================================================================

Vector3 point_on_circle(double radius, double angle_radians, const Vector3 &center) {
  Vector3 offset = circular_offset_2d(radius, angle_radians);
  return center + offset;
}

Vector3 circular_offset_2d(double radius, double angle_radians) {
  return Vector3(radius * std::cos(angle_radians),
                 radius * std::sin(angle_radians), 
                 0.0);
}

// ============================================================================
// Vector Operations
// ============================================================================

Vector3 displace_along_direction(const Vector3 &point, const Vector3 &direction, double amount) {
  return point + direction * amount;
}

void transform_vertices_range(
    const double *input_vertices, // Raw pointer to avoid circular dependency
    double *output_vertices, int vertex_count, const Matrix3 &rotation,
    const Vector3 &offset) {
  for (int v = 0; v < vertex_count; ++v) {
    ConstVector3Map input_point(input_vertices + v * 3);
    Vector3 transformed = apply_transform(input_point, rotation, offset);
    Vector3Map(output_vertices + v * 3) = transformed;
  }
}

} // namespace nodeflux::core::math
