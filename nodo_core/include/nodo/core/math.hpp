#pragma once

#include "types.hpp"

#include <cmath>

// Forward declaration
namespace nodo::core {
class Mesh;
} // namespace nodo::core

namespace nodo::core::math {

// ============================================================================
// Mathematical Constants
// ============================================================================

// Core constants using our own namespace to avoid conflicts
constexpr double PI = 3.14159265358979323846;
constexpr double TAU = 6.28318530717958647692;   // 2 * PI
constexpr double EULER = 2.71828182845904523536; // Euler's number
constexpr double SQRT2 = 1.41421356237309504880;
constexpr double SQRT3 = 1.73205080756887729352;
constexpr double PHI = 1.61803398874989484820; // Golden ratio

// Common fractions of PI
constexpr double PI_OVER_2 = PI / 2.0; // π/2
constexpr double PI_OVER_4 = PI / 4.0; // π/4

// Backwards compatibility - define M_PI etc. if not already defined (Windows)
#ifndef M_PI
constexpr double M_PI = PI;
#endif
#ifndef M_PI_2
constexpr double M_PI_2 = PI_OVER_2;
#endif
#ifndef M_PI_4
constexpr double M_PI_4 = PI_OVER_4;
#endif
#ifndef M_E
constexpr double M_E = EULER;
#endif

constexpr double DEGREES_TO_RADIANS_FACTOR = PI / 180.0;
constexpr double RADIANS_TO_DEGREES_FACTOR = 180.0 / PI;

// ============================================================================
// Angle Conversion
// ============================================================================

inline double degrees_to_radians(double degrees) {
  return degrees * DEGREES_TO_RADIANS_FACTOR;
}

inline double radians_to_degrees(double radians) {
  return radians * RADIANS_TO_DEGREES_FACTOR;
}

// ============================================================================
// Rotation Matrix Creation (always in radians)
// ============================================================================

Matrix3 rotation_x(double radians);
Matrix3 rotation_y(double radians);
Matrix3 rotation_z(double radians);

// ============================================================================
// Point/Vector Transformations (functional - returns new)
// ============================================================================

Vector3 apply_rotation(const Vector3& point, const Matrix3& rotation);
Vector3 apply_translation(const Vector3& point, const Vector3& offset);
Vector3 apply_transform(const Vector3& point, const Matrix3& rotation, const Vector3& offset);

// Mirror/Reflection operations
Vector3 mirror_point_across_plane(const Vector3& point, const Vector3& plane_point, const Vector3& plane_normal);

// Basic geometric utilities
Vector3 midpoint(const Vector3& point_a, const Vector3& point_b);
Vector3 triangle_centroid(const Vector3& vertex_a, const Vector3& vertex_b, const Vector3& vertex_c);

// Circular/polar utilities
Vector3 point_on_circle(double radius, double angle_radians, const Vector3& center = Vector3::Zero());
Vector3 circular_offset_2d(double radius, double angle_radians);

// Vector operations
Vector3 displace_along_direction(const Vector3& point, const Vector3& direction, double amount);

// ============================================================================
// High-Level Mesh Operations (forward declaration to avoid circular dependency)
// ============================================================================

/// Transform a range of vertices - implementation will use Eigen matrices
/// directly
void transform_vertices_range(const double* input_vertices, // Raw pointer to avoid circular dependency
                              double* output_vertices, int vertex_count, const Matrix3& rotation,
                              const Vector3& offset);

} // namespace nodo::core::math
