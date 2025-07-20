#include "../../include/nodeflux/core/math.hpp"

namespace nodeflux::core::math {

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

void transform_vertices_range(
    const double *input_vertices, // Raw pointer to avoid circular dependency
    double *output_vertices, int vertex_count, const Matrix3 &rotation,
    const Vector3 &offset) {
  for (int v = 0; v < vertex_count; ++v) {
    Vector3 input_point = Eigen::Map<const Vector3>(input_vertices + v * 3);
    Vector3 transformed = apply_transform(input_point, rotation, offset);
    Eigen::Map<Vector3>(output_vertices + v * 3) = transformed;
  }
}

} // namespace nodeflux::core::math
