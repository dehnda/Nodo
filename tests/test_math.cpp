#include <gtest/gtest.h>
#include <nodo/core/math.hpp>
#include <nodo/core/types.hpp>

using namespace nodo::core;

TEST(MathTest, rotation_x_with_zero_radians_returns_identity) {
  auto result = ::math::rotation_x(0);
  ::Matrix3 expected;
  // clang-format off
    expected << 1, 0, 0,
                0, 1, 0,
                0, 0, 1;
  // clang-format on
  EXPECT_TRUE(result.isApprox(expected, 1e-10))
      << "Rotation X with 0 radians should return identity matrix";
}

TEST(MathTest, rotation_x_with_pi_radians_returns_correct_matrix) {
  auto result = ::math::rotation_x(::math::PI);
  ::Matrix3 expected;
  // clang-format off
    expected << 1,  0,  0,
                0, -1,  0,
                0,  0, -1;
  // clang-format on
  EXPECT_TRUE(result.isApprox(expected, 1e-10))
      << "Rotation X with π radians should flip Y and Z axes";
}

TEST(MathTest, rotation_x_with_pi_over_two_radians_returns_correct_matrix) {
  auto result = ::math::rotation_x(::math::PI / 2);
  ::Matrix3 expected;
  // clang-format off
    expected << 1,  0,  0,
                0,  0, -1,
                0,  1,  0;
  // clang-format on
  EXPECT_TRUE(result.isApprox(expected, 1e-10))
      << "Rotation X with π/2 radians should rotate Y to Z and Z to -Y";
}

TEST(MathTest, rotation_x_actually_rotates_points) {
  auto rotation = ::math::rotation_x(::math::PI / 2);

  // Test Y-axis point
  ::Vector3 point_y(0, 1, 0);
  ::Vector3 result_y = rotation * point_y;
  ::Vector3 expected_y(0, 0, 1);
  EXPECT_TRUE(result_y.isApprox(expected_y, 1e-10));

  // Test Z-axis point
  ::Vector3 point_z(0, 0, 1);
  ::Vector3 result_z = rotation * point_z;
  ::Vector3 expected_z(0, -1, 0);
  EXPECT_TRUE(result_z.isApprox(expected_z, 1e-10));
}

TEST(MathTest, rotation_y_with_zero_radians_returns_identity) {
  auto result = ::math::rotation_y(0);
  ::Matrix3 expected;
  // clang-format off
    expected << 1, 0, 0,
                0, 1, 0,
                0, 0, 1;
  // clang-format on
  EXPECT_TRUE(result.isApprox(expected, 1e-10))
      << "Rotation Y with 0 radians should return identity matrix";
}

TEST(MathTest, rotation_y_with_pi_radians_returns_correct_matrix) {
  auto result = ::math::rotation_y(::math::PI);
  ::Matrix3 expected;
  // clang-format off
    expected << -1, 0, 0,
                0, 1, 0,
                0, 0, -1;
  // clang-format on
  EXPECT_TRUE(result.isApprox(expected, 1e-10))
      << "Rotation Y with π radians should flip X and Z axes";
}

TEST(MathTest, rotation_y_with_pi_over_two_radians_returns_correct_matrix) {
  auto result = ::math::rotation_y(::math::PI / 2);
  ::Matrix3 expected;
  // clang-format off
    expected << 0, 0, 1,
                0, 1, 0,
               -1, 0, 0;
  // clang-format on
  EXPECT_TRUE(result.isApprox(expected, 1e-10))
      << "Rotation Y with π/2 radians should rotate X to Z and Z to -X";
}

TEST(MathTest, rotation_y_actually_rotates_points) {
  auto rotation = ::math::rotation_y(::math::PI / 2);

  // Test X-axis point
  ::Vector3 point_x(1, 0, 0);
  ::Vector3 result_x = rotation * point_x;
  ::Vector3 expected_x(0, 0, -1);
  EXPECT_TRUE(result_x.isApprox(expected_x, 1e-10));

  // Test Z-axis point
  ::Vector3 point_z(0, 0, 1);
  ::Vector3 result_z = rotation * point_z;
  ::Vector3 expected_z(1, 0, 0);
  EXPECT_TRUE(result_z.isApprox(expected_z, 1e-10));
}

TEST(MathTest, rotation_z_with_zero_radians_returns_identity) {
  auto result = ::math::rotation_z(0);
  ::Matrix3 expected;
  // clang-format off
    expected << 1, 0, 0,
                0, 1, 0,
                0, 0, 1;
  // clang-format on
  EXPECT_TRUE(result.isApprox(expected, 1e-10))
      << "Rotation Z with 0 radians should return identity matrix";
}

TEST(MathTest, rotation_z_with_pi_radians_returns_correct_matrix) {
  auto result = ::math::rotation_z(::math::PI);
  ::Matrix3 expected;
  // clang-format off
    expected << -1, 0, 0,
                0, -1, 0,
                0, 0, 1;
  // clang-format on
  EXPECT_TRUE(result.isApprox(expected, 1e-10))
      << "Rotation Z with π radians should flip X and Y axes";
}

TEST(MathTest, rotation_z_with_pi_over_two_radians_returns_correct_matrix) {
  auto result = ::math::rotation_z(::math::PI / 2);
  ::Matrix3 expected;
  // clang-format off
    expected << 0, -1, 0,
                1, 0, 0,
                0, 0, 1;
  // clang-format on
  EXPECT_TRUE(result.isApprox(expected, 1e-10))
      << "Rotation Z with π/2 radians should rotate X to -Y and Y to X";
}

TEST(MathTest, rotation_z_actually_rotates_points) {
  auto rotation = ::math::rotation_z(::math::PI / 2);

  // Test X-axis point
  ::Vector3 point_x(1, 0, 0);
  ::Vector3 result_x = rotation * point_x;
  ::Vector3 expected_x(0, 1, 0);
  EXPECT_TRUE(result_x.isApprox(expected_x, 1e-10));

  // Test Y-axis point
  ::Vector3 point_y(0, 1, 0);
  ::Vector3 result_y = rotation * point_y;
  ::Vector3 expected_y(-1, 0, 0);
  EXPECT_TRUE(result_y.isApprox(expected_y, 1e-10));
}

TEST(MathTest, transform_vertices_range_gives_same_result_as_apply_transform) {
  // Set up test data
  std::vector<double> input_vertices = {1, 0, 0, 0, 1, 0, 0, 0, 1};
  std::vector<double> output_vertices(9);

  Matrix3 rotation = ::math::rotation_z(math::PI / 2);
  Vector3 offset(10, 20, 30);

  // Test bulk operation
  math::transform_vertices_range(input_vertices.data(), output_vertices.data(),
                                 3, rotation, offset);

  // Test individual operations for comparison
  for (int i = 0; i < 3; ++i) {
    ::Vector3 input(input_vertices[i * 3], input_vertices[i * 3 + 1],
                    input_vertices[i * 3 + 2]);
    ::Vector3 expected = math::apply_transform(input, rotation, offset);
    Vector3 actual(output_vertices[i * 3], output_vertices[i * 3 + 1],
                   output_vertices[i * 3 + 2]);

    EXPECT_TRUE(actual.isApprox(expected, 1e-10));
  }
}
