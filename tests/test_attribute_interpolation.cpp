#include "nodo/core/attribute_interpolation.hpp"
#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"

#include <cmath>

#include <gtest/gtest.h>

using namespace nodo::core;

class AttributeInterpolationTest : public ::testing::Test {
protected:
  void SetUp() override { geo = std::make_unique<GeometryContainer>(); }

  std::unique_ptr<GeometryContainer> geo;
};

// ============================================================================
// Linear Interpolation Tests
// ============================================================================

TEST_F(AttributeInterpolationTest, LinearInterpolation_Float) {
  float a = 0.0f;
  float b = 10.0f;

  EXPECT_FLOAT_EQ(interpolate_linear(a, b, 0.0f), 0.0f);
  EXPECT_FLOAT_EQ(interpolate_linear(a, b, 0.5f), 5.0f);
  EXPECT_FLOAT_EQ(interpolate_linear(a, b, 1.0f), 10.0f);
  EXPECT_FLOAT_EQ(interpolate_linear(a, b, 0.25f), 2.5f);
}

TEST_F(AttributeInterpolationTest, LinearInterpolation_Int) {
  int a = 0;
  int b = 10;

  EXPECT_EQ(interpolate_linear(a, b, 0.0f), 0);
  EXPECT_EQ(interpolate_linear(a, b, 0.5f), 5);
  EXPECT_EQ(interpolate_linear(a, b, 1.0f), 10);
  EXPECT_EQ(interpolate_linear(a, b, 0.3f), 3); // Should round
}

TEST_F(AttributeInterpolationTest, LinearInterpolation_Vec3f) {
  Vec3f a{0.0f, 0.0f, 0.0f};
  Vec3f b{10.0f, 20.0f, 30.0f};

  Vec3f result = interpolate_linear(a, b, 0.5f);
  EXPECT_FLOAT_EQ(result.x(), 5.0f);
  EXPECT_FLOAT_EQ(result.y(), 10.0f);
  EXPECT_FLOAT_EQ(result.z(), 15.0f);
}

// ============================================================================
// Cubic Interpolation Tests
// ============================================================================

TEST_F(AttributeInterpolationTest, CubicInterpolation_Smoothness) {
  float a = 0.0f;
  float b = 1.0f;

  // Cubic should be smoother at endpoints
  float linear_025 = interpolate_linear(a, b, 0.25f);
  float cubic_025 = interpolate_cubic(a, b, 0.25f);

  // Cubic should be less than linear in first half
  EXPECT_LT(cubic_025, linear_025);

  // At t=0.5, both should be close
  float linear_05 = interpolate_linear(a, b, 0.5f);
  float cubic_05 = interpolate_cubic(a, b, 0.5f);
  EXPECT_NEAR(cubic_05, linear_05, 0.1f);
}

// ============================================================================
// Weighted Interpolation Tests
// ============================================================================

TEST_F(AttributeInterpolationTest, WeightedAverage_Float) {
  std::vector<float> values = {1.0f, 2.0f, 3.0f, 4.0f};
  std::vector<float> weights = {0.25f, 0.25f, 0.25f, 0.25f};

  float result = interpolate_weighted<float>(values, weights);
  EXPECT_FLOAT_EQ(result, 2.5f); // Average
}

TEST_F(AttributeInterpolationTest, WeightedAverage_NonUniform) {
  std::vector<float> values = {10.0f, 20.0f};
  std::vector<float> weights = {0.75f, 0.25f}; // Weighted toward first

  float result = interpolate_weighted<float>(values, weights);
  EXPECT_FLOAT_EQ(result, 12.5f); // 10*0.75 + 20*0.25
}

TEST_F(AttributeInterpolationTest, WeightedAverage_Vec3f) {
  std::vector<Vec3f> values = {Vec3f{1.0f, 0.0f, 0.0f}, Vec3f{0.0f, 1.0f, 0.0f}, Vec3f{0.0f, 0.0f, 1.0f}};
  std::vector<float> weights = {1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f};

  Vec3f result = interpolate_weighted<Vec3f>(values, weights);
  EXPECT_NEAR(result.x(), 1.0f / 3.0f, 1e-5f);
  EXPECT_NEAR(result.y(), 1.0f / 3.0f, 1e-5f);
  EXPECT_NEAR(result.z(), 1.0f / 3.0f, 1e-5f);
}

// ============================================================================
// Barycentric Interpolation Tests
// ============================================================================

TEST_F(AttributeInterpolationTest, BarycentricInterpolation_Corners) {
  float v0 = 1.0f;
  float v1 = 2.0f;
  float v2 = 3.0f;

  // At corner 0: (u=0, v=0)
  EXPECT_FLOAT_EQ(interpolate_barycentric(v0, v1, v2, 0.0f, 0.0f), 1.0f);

  // At corner 1: (u=1, v=0)
  EXPECT_FLOAT_EQ(interpolate_barycentric(v0, v1, v2, 1.0f, 0.0f), 2.0f);

  // At corner 2: (u=0, v=1)
  EXPECT_FLOAT_EQ(interpolate_barycentric(v0, v1, v2, 0.0f, 1.0f), 3.0f);
}

TEST_F(AttributeInterpolationTest, BarycentricInterpolation_Center) {
  Vec3f v0{1.0f, 0.0f, 0.0f};
  Vec3f v1{0.0f, 1.0f, 0.0f};
  Vec3f v2{0.0f, 0.0f, 1.0f};

  // At center of triangle (u=1/3, v=1/3)
  Vec3f result = interpolate_barycentric(v0, v1, v2, 1.0f / 3.0f, 1.0f / 3.0f);

  EXPECT_NEAR(result.x(), 1.0f / 3.0f, 1e-5f);
  EXPECT_NEAR(result.y(), 1.0f / 3.0f, 1e-5f);
  EXPECT_NEAR(result.z(), 1.0f / 3.0f, 1e-5f);
}

// ============================================================================
// Bilinear Interpolation Tests
// ============================================================================

TEST_F(AttributeInterpolationTest, BilinearInterpolation_Corners) {
  float v00 = 0.0f;
  float v10 = 1.0f;
  float v01 = 2.0f;
  float v11 = 3.0f;

  EXPECT_FLOAT_EQ(interpolate_bilinear(v00, v10, v01, v11, 0.0f, 0.0f), 0.0f);
  EXPECT_FLOAT_EQ(interpolate_bilinear(v00, v10, v01, v11, 1.0f, 0.0f), 1.0f);
  EXPECT_FLOAT_EQ(interpolate_bilinear(v00, v10, v01, v11, 0.0f, 1.0f), 2.0f);
  EXPECT_FLOAT_EQ(interpolate_bilinear(v00, v10, v01, v11, 1.0f, 1.0f), 3.0f);
}

TEST_F(AttributeInterpolationTest, BilinearInterpolation_Center) {
  float v00 = 0.0f;
  float v10 = 2.0f;
  float v01 = 2.0f;
  float v11 = 4.0f;

  // Center should be average
  float result = interpolate_bilinear(v00, v10, v01, v11, 0.5f, 0.5f);
  EXPECT_FLOAT_EQ(result, 2.0f);
}

// ============================================================================
// Blend Attributes Tests
// ============================================================================

TEST_F(AttributeInterpolationTest, BlendAttributes_EqualWeights) {
  // Create 4 points with different heights
  geo->set_point_count(4);
  geo->add_point_attribute("height", AttributeType::FLOAT);

  auto* height = geo->get_point_attribute_typed<float>("height");
  auto height_span = height->values_writable();
  height_span[0] = 1.0f;
  height_span[1] = 2.0f;
  height_span[2] = 3.0f;
  height_span[3] = 4.0f;

  // Blend points 0,1,2 into point 3 (should average to 2.0)
  std::vector<size_t> sources = {0, 1, 2};
  EXPECT_TRUE(blend_attributes<float>(*geo, "height", ElementClass::POINT, sources, 3, {}));

  auto result = height->values();
  EXPECT_FLOAT_EQ(result[3], 2.0f);
}

TEST_F(AttributeInterpolationTest, BlendAttributes_CustomWeights) {
  geo->set_point_count(3);
  geo->add_point_attribute("value", AttributeType::FLOAT);

  auto* value = geo->get_point_attribute_typed<float>("value");
  auto value_span = value->values_writable();
  value_span[0] = 10.0f;
  value_span[1] = 20.0f;
  value_span[2] = 0.0f;

  // Blend with weights [0.25, 0.75]
  std::vector<size_t> sources = {0, 1};
  std::vector<float> weights = {0.25f, 0.75f};
  EXPECT_TRUE(blend_attributes<float>(*geo, "value", ElementClass::POINT, sources, 2, weights));

  auto result = value->values();
  EXPECT_FLOAT_EQ(result[2], 17.5f); // 10*0.25 + 20*0.75
}

TEST_F(AttributeInterpolationTest, BlendAttributes_Vec3f) {
  geo->set_point_count(3);
  geo->add_point_attribute("Cd", AttributeType::VEC3F);

  auto* color = geo->get_point_attribute_typed<Vec3f>("Cd");
  auto color_span = color->values_writable();
  color_span[0] = Vec3f{1.0f, 0.0f, 0.0f}; // Red
  color_span[1] = Vec3f{0.0f, 0.0f, 1.0f}; // Blue
  color_span[2] = Vec3f{0.0f, 0.0f, 0.0f};

  // Equal blend should give purple
  std::vector<size_t> sources = {0, 1};
  EXPECT_TRUE(blend_attributes<Vec3f>(*geo, "Cd", ElementClass::POINT, sources, 2, {}));

  auto result = color->values();
  EXPECT_FLOAT_EQ(result[2].x(), 0.5f);
  EXPECT_FLOAT_EQ(result[2].y(), 0.0f);
  EXPECT_FLOAT_EQ(result[2].z(), 0.5f);
}

// ============================================================================
// Copy and Interpolate All Attributes Tests
// ============================================================================

TEST_F(AttributeInterpolationTest, CopyAndInterpolateAll) {
  geo->set_point_count(3);

  // Create multiple attributes
  geo->add_point_attribute("height", AttributeType::FLOAT);
  geo->add_point_attribute("Cd", AttributeType::VEC3F);

  auto* height = geo->get_point_attribute_typed<float>("height");
  auto height_span = height->values_writable();
  height_span[0] = 1.0f;
  height_span[1] = 3.0f;

  auto* color = geo->get_point_attribute_typed<Vec3f>("Cd");
  auto color_span = color->values_writable();
  color_span[0] = Vec3f{1.0f, 0.0f, 0.0f};
  color_span[1] = Vec3f{0.0f, 1.0f, 0.0f};

  // Interpolate both attributes to point 2
  std::vector<size_t> sources = {0, 1};
  std::vector<float> weights = {0.5f, 0.5f};

  EXPECT_TRUE(copy_and_interpolate_all_attributes(*geo, ElementClass::POINT, sources, 2, weights));

  // Check height
  auto height_result = height->values();
  EXPECT_FLOAT_EQ(height_result[2], 2.0f);

  // Check color
  auto color_result = color->values();
  EXPECT_FLOAT_EQ(color_result[2].x(), 0.5f);
  EXPECT_FLOAT_EQ(color_result[2].y(), 0.5f);
  EXPECT_FLOAT_EQ(color_result[2].z(), 0.0f);
}

// ============================================================================
// Resample Curve Attribute Tests
// ============================================================================

TEST_F(AttributeInterpolationTest, ResampleCurve_Endpoints) {
  geo->set_point_count(3);
  geo->add_point_attribute("value", AttributeType::FLOAT);

  auto* value = geo->get_point_attribute_typed<float>("value");
  auto value_span = value->values_writable();
  value_span[0] = 0.0f;
  value_span[1] = 5.0f;
  value_span[2] = 10.0f;

  std::vector<int> curve = {0, 1, 2};

  // At start
  EXPECT_FLOAT_EQ(resample_curve_attribute<float>(*geo, "value", curve, 0.0f), 0.0f);

  // At end
  EXPECT_FLOAT_EQ(resample_curve_attribute<float>(*geo, "value", curve, 1.0f), 10.0f);
}

TEST_F(AttributeInterpolationTest, ResampleCurve_Midpoint) {
  geo->set_point_count(3);
  geo->add_point_attribute("value", AttributeType::FLOAT);

  auto* value = geo->get_point_attribute_typed<float>("value");
  auto value_span = value->values_writable();
  value_span[0] = 0.0f;
  value_span[1] = 10.0f;
  value_span[2] = 20.0f;

  std::vector<int> curve = {0, 1, 2};

  // Halfway between 0 and 1 (t=0.25)
  float result = resample_curve_attribute<float>(*geo, "value", curve, 0.25f);
  EXPECT_FLOAT_EQ(result, 5.0f);

  // Halfway between 1 and 2 (t=0.75)
  result = resample_curve_attribute<float>(*geo, "value", curve, 0.75f);
  EXPECT_FLOAT_EQ(result, 15.0f);
}

// ============================================================================
// Specialized Interpolation Tests
// ============================================================================

TEST_F(AttributeInterpolationTest, InterpolateNormal) {
  Vec3f n0{1.0f, 0.0f, 0.0f};
  Vec3f n1{0.0f, 1.0f, 0.0f};

  Vec3f result = interpolate_normal(n0, n1, 0.5f);

  // Result should be normalized
  float length = std::sqrt(result.x() * result.x() + result.y() * result.y() + result.z() * result.z());
  EXPECT_NEAR(length, 1.0f, 1e-5f);

  // Should be roughly 45 degrees
  EXPECT_NEAR(result.x(), result.y(), 1e-5f);
}

TEST_F(AttributeInterpolationTest, InterpolateColor_NoLinearization) {
  Vec3f c0{0.0f, 0.0f, 0.0f}; // Black
  Vec3f c1{1.0f, 1.0f, 1.0f}; // White

  Vec3f result = interpolate_color(c0, c1, 0.5f, false);

  EXPECT_FLOAT_EQ(result.x(), 0.5f);
  EXPECT_FLOAT_EQ(result.y(), 0.5f);
  EXPECT_FLOAT_EQ(result.z(), 0.5f);
}

TEST_F(AttributeInterpolationTest, InterpolateClamped) {
  float a = 0.0f;
  float b = 10.0f;

  // Normal case
  float result = interpolate_clamped(a, b, 0.5f, 0.0f, 10.0f);
  EXPECT_FLOAT_EQ(result, 5.0f);

  // Clamped to max
  result = interpolate_clamped(a, b, 0.8f, 0.0f, 5.0f);
  EXPECT_FLOAT_EQ(result, 5.0f); // Would be 8, but clamped

  // Clamped to min
  result = interpolate_clamped(a, b, -0.2f, 0.0f, 10.0f);
  EXPECT_FLOAT_EQ(result, 0.0f);
}

// ============================================================================
// Helper Function Tests
// ============================================================================

TEST_F(AttributeInterpolationTest, Smoothstep) {
  EXPECT_FLOAT_EQ(smoothstep(0.0f), 0.0f);
  EXPECT_FLOAT_EQ(smoothstep(1.0f), 1.0f);
  EXPECT_FLOAT_EQ(smoothstep(0.5f), 0.5f);

  // Should be smoother at edges
  float linear_025 = 0.25f;
  float smooth_025 = smoothstep(0.25f);
  EXPECT_LT(smooth_025, linear_025);
}

TEST_F(AttributeInterpolationTest, Saturate) {
  EXPECT_FLOAT_EQ(saturate(-1.0f), 0.0f);
  EXPECT_FLOAT_EQ(saturate(0.5f), 0.5f);
  EXPECT_FLOAT_EQ(saturate(2.0f), 1.0f);
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(AttributeInterpolationTest, BlendAttributes_EmptySources) {
  geo->set_point_count(2);
  geo->add_point_attribute("value", AttributeType::FLOAT);

  std::vector<size_t> empty_sources;
  EXPECT_FALSE(blend_attributes<float>(*geo, "value", ElementClass::POINT, empty_sources, 0, {}));
}

TEST_F(AttributeInterpolationTest, BlendAttributes_NonexistentAttribute) {
  geo->set_point_count(2);

  std::vector<size_t> sources = {0};
  EXPECT_FALSE(blend_attributes<float>(*geo, "nonexistent", ElementClass::POINT, sources, 1, {}));
}

TEST_F(AttributeInterpolationTest, ResampleCurve_TooFewPoints) {
  geo->set_point_count(1);
  geo->add_point_attribute("value", AttributeType::FLOAT);

  std::vector<int> curve = {0};
  float result = resample_curve_attribute<float>(*geo, "value", curve, 0.5f);

  // Should return default value
  EXPECT_FLOAT_EQ(result, 0.0f);
}

TEST_F(AttributeInterpolationTest, WeightedAverage_MismatchedSizes) {
  std::vector<float> values = {1.0f, 2.0f, 3.0f};
  std::vector<float> weights = {0.5f, 0.5f}; // Wrong size

  float result = interpolate_weighted<float>(values, weights);

  // Should return default
  EXPECT_FLOAT_EQ(result, 0.0f);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(AttributeInterpolationTest, ComplexWorkflow_PointAveraging) {
  // Create a triangle with vertex colors
  geo->set_point_count(3);
  geo->add_point_attribute(standard_attrs::P, AttributeType::VEC3F);
  geo->add_point_attribute("Cd", AttributeType::VEC3F);

  auto* pos = geo->get_point_attribute_typed<Vec3f>(standard_attrs::P);
  auto pos_span = pos->values_writable();
  pos_span[0] = Vec3f{0.0f, 0.0f, 0.0f};
  pos_span[1] = Vec3f{1.0f, 0.0f, 0.0f};
  pos_span[2] = Vec3f{0.5f, 1.0f, 0.0f};

  auto* color = geo->get_point_attribute_typed<Vec3f>("Cd");
  auto color_span = color->values_writable();
  color_span[0] = Vec3f{1.0f, 0.0f, 0.0f}; // Red
  color_span[1] = Vec3f{0.0f, 1.0f, 0.0f}; // Green
  color_span[2] = Vec3f{0.0f, 0.0f, 1.0f}; // Blue

  // Create a new point at the center by blending all three
  geo->set_point_count(4);
  std::vector<size_t> sources = {0, 1, 2};
  std::vector<float> weights = {1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f};

  EXPECT_TRUE(copy_and_interpolate_all_attributes(*geo, ElementClass::POINT, sources, 3, weights));

  // Check position
  auto pos_result = pos->values();
  EXPECT_NEAR(pos_result[3].x(), 0.5f, 1e-5f);
  EXPECT_NEAR(pos_result[3].y(), 1.0f / 3.0f, 1e-5f);

  // Check color (should be gray)
  auto color_result = color->values();
  EXPECT_NEAR(color_result[3].x(), 1.0f / 3.0f, 1e-5f);
  EXPECT_NEAR(color_result[3].y(), 1.0f / 3.0f, 1e-5f);
  EXPECT_NEAR(color_result[3].z(), 1.0f / 3.0f, 1e-5f);
}
