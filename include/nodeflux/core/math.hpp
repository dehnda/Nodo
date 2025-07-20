#pragma once

#include "types.hpp"
#include <cmath>

// Forward declaration
namespace nodeflux::core {
class Mesh;
}

namespace nodeflux::core::math {

// ============================================================================
// Mathematical Constants
// ============================================================================

constexpr double PI = 3.14159265358979323846;
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

Vector3 apply_rotation(const Vector3 &point, const Matrix3 &rotation);
Vector3 apply_translation(const Vector3 &point, const Vector3 &offset);
Vector3 apply_transform(const Vector3 &point, const Matrix3 &rotation,
                        const Vector3 &offset);

// Mirror/Reflection operations
Vector3 mirror_point_across_plane(const Vector3 &point,
                                  const Vector3 &plane_point,
                                  const Vector3 &plane_normal);

// ============================================================================
// High-Level Mesh Operations (forward declaration to avoid circular dependency)
// ============================================================================

/// Transform a range of vertices - implementation will use Eigen matrices
/// directly
void transform_vertices_range(
    const double *input_vertices, // Raw pointer to avoid circular dependency
    double *output_vertices, int vertex_count, const Matrix3 &rotation,
    const Vector3 &offset);

} // namespace nodeflux::core::math
