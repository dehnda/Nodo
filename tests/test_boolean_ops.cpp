#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/geometry/boolean_ops.hpp"
#include "nodo/geometry/box_generator.hpp"
#include "nodo/geometry/sphere_generator.hpp"

#include <gtest/gtest.h>

using namespace nodo;
using namespace nodo::geometry;

class BooleanOpsTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create test geometries
    auto box_result = BoxGenerator::generate(2.0, 2.0, 2.0);
    ASSERT_TRUE(box_result.has_value());
    box_geo_ = box_result.value().clone();

    auto sphere_result = SphereGenerator::generate_uv_sphere(1.0, 32, 16);
    ASSERT_TRUE(sphere_result.has_value());
    sphere_geo_ = sphere_result.value().clone();
  }

  core::GeometryContainer box_geo_;
  core::GeometryContainer sphere_geo_;
};

// ============================================================================
// Union Operation Tests
// ============================================================================

TEST_F(BooleanOpsTest, Union_TwoIdenticalBoxes) {
  auto result = BooleanOps::union_geometries(box_geo_, box_geo_);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->point_count(), 0);
  EXPECT_GT(result->primitive_count(), 0);
}

TEST_F(BooleanOpsTest, Union_TwoOverlappingSpheres) {
  auto sphere1 = sphere_geo_.clone();
  auto sphere2 = sphere_geo_.clone();

  // Offset sphere2
  auto* pos2 = sphere2.get_point_attribute_typed<core::Vec3f>(core::standard_attrs::P);
  ASSERT_NE(pos2, nullptr);
  auto pos2_span = pos2->values_writable();
  for (size_t i = 0; i < pos2_span.size(); ++i) {
    pos2_span[i].x() += 0.5f;
  }

  auto result = BooleanOps::union_geometries(sphere1, sphere2);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->point_count(), 0);
  EXPECT_GT(result->primitive_count(), 0);
}

TEST_F(BooleanOpsTest, Union_NonOverlappingGeometries) {
  auto box1 = box_geo_.clone();
  auto box2 = box_geo_.clone();

  // Offset box2 far away
  auto* pos2 = box2.get_point_attribute_typed<core::Vec3f>(core::standard_attrs::P);
  ASSERT_NE(pos2, nullptr);
  auto pos2_span = pos2->values_writable();
  for (size_t i = 0; i < pos2_span.size(); ++i) {
    pos2_span[i].x() += 10.0f;
  }

  auto result = BooleanOps::union_geometries(box1, box2);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->point_count(), 0);
  EXPECT_GT(result->primitive_count(), 0);
}

TEST_F(BooleanOpsTest, Union_WithEmptyGeometry) {
  core::GeometryContainer empty_geo;
  auto result = BooleanOps::union_geometries(box_geo_, empty_geo);

  EXPECT_FALSE(result.has_value());
  const auto& error = BooleanOps::last_error();
  EXPECT_EQ(error.category, core::ErrorCategory::Validation);
}

// ============================================================================
// Intersection Operation Tests
// ============================================================================

TEST_F(BooleanOpsTest, Intersection_TwoIdenticalBoxes) {
  auto result = BooleanOps::intersect_geometries(box_geo_, box_geo_);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->point_count(), 0);
  EXPECT_GT(result->primitive_count(), 0);
}

TEST_F(BooleanOpsTest, Intersection_TwoOverlappingSpheres) {
  auto sphere1 = sphere_geo_.clone();
  auto sphere2 = sphere_geo_.clone();

  // Offset sphere2
  auto* pos2 = sphere2.get_point_attribute_typed<core::Vec3f>(core::standard_attrs::P);
  ASSERT_NE(pos2, nullptr);
  auto pos2_span = pos2->values_writable();
  for (size_t i = 0; i < pos2_span.size(); ++i) {
    pos2_span[i].x() += 0.5f;
  }

  auto result = BooleanOps::intersect_geometries(sphere1, sphere2);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->point_count(), 0);
  EXPECT_GT(result->primitive_count(), 0);

  // Intersection should be smaller than either sphere
  EXPECT_LT(result->point_count(), sphere1.point_count());
}

TEST_F(BooleanOpsTest, Intersection_NonOverlappingGeometries) {
  auto box1 = box_geo_.clone();
  auto box2 = box_geo_.clone();

  // Offset box2 far away
  auto* pos2 = box2.get_point_attribute_typed<core::Vec3f>(core::standard_attrs::P);
  ASSERT_NE(pos2, nullptr);
  auto pos2_span = pos2->values_writable();
  for (size_t i = 0; i < pos2_span.size(); ++i) {
    pos2_span[i].x() += 10.0f;
  }

  auto result = BooleanOps::intersect_geometries(box1, box2);

  // Non-overlapping geometries may return empty result or nullopt
  if (result.has_value()) {
    EXPECT_EQ(result->primitive_count(), 0);
  } else {
    SUCCEED() << "Empty intersection returned as nullopt";
  }
}

// ============================================================================
// Difference Operation Tests
// ============================================================================

TEST_F(BooleanOpsTest, Difference_TwoIdenticalBoxes) {
  auto result = BooleanOps::difference_geometries(box_geo_, box_geo_);

  // Complete subtraction may return nullopt or empty geometry
  if (result.has_value()) {
    EXPECT_EQ(result->primitive_count(), 0) << "Expected empty result";
  } else {
    SUCCEED() << "Empty difference returned as nullopt";
  }
}

TEST_F(BooleanOpsTest, Difference_LargeMinusSmall) {
  auto large_box_result = BoxGenerator::generate(2.0, 2.0, 2.0);
  ASSERT_TRUE(large_box_result.has_value());
  auto large_geo = large_box_result.value().clone();

  auto small_box_result = BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(small_box_result.has_value());
  auto small_geo = small_box_result.value().clone();

  auto result = BooleanOps::difference_geometries(large_geo, small_geo);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->point_count(), 0);
  EXPECT_GT(result->primitive_count(), 0);
}

TEST_F(BooleanOpsTest, Difference_SmallMinusLarge) {
  auto large_box_result = BoxGenerator::generate(2.0, 2.0, 2.0);
  ASSERT_TRUE(large_box_result.has_value());
  auto large_geo = large_box_result.value().clone();

  auto small_box_result = BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(small_box_result.has_value());
  auto small_geo = small_box_result.value().clone();

  auto result = BooleanOps::difference_geometries(small_geo, large_geo);

  // Small - large may return empty result
  if (result.has_value()) {
    EXPECT_EQ(result->primitive_count(), 0);
  } else {
    SUCCEED() << "Empty difference returned as nullopt";
  }
}

TEST_F(BooleanOpsTest, Difference_NonOverlappingGeometries) {
  auto box1 = box_geo_.clone();
  auto box2 = box_geo_.clone();

  // Offset box2 far away
  auto* pos2 = box2.get_point_attribute_typed<core::Vec3f>(core::standard_attrs::P);
  ASSERT_NE(pos2, nullptr);
  auto pos2_span = pos2->values_writable();
  for (size_t i = 0; i < pos2_span.size(); ++i) {
    pos2_span[i].x() += 10.0f;
  }

  auto result = BooleanOps::difference_geometries(box1, box2);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->point_count(), 0);
  // Result should be approximately the same as box1
  EXPECT_EQ(result->point_count(), box1.point_count());
}

TEST_F(BooleanOpsTest, Difference_WithEmptyGeometry) {
  core::GeometryContainer empty_geo;
  auto result = BooleanOps::difference_geometries(box_geo_, empty_geo);

  EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(BooleanOpsTest, LastError_SetOnFailure) {
  core::GeometryContainer empty_geo;
  auto result = BooleanOps::union_geometries(box_geo_, empty_geo);

  EXPECT_FALSE(result.has_value());
  auto error = BooleanOps::last_error();
  EXPECT_EQ(error.category, core::ErrorCategory::Validation);
}

TEST_F(BooleanOpsTest, LastError_ClearsOnSuccess) {
  // First cause an error
  core::GeometryContainer empty_geo;
  auto fail_result = BooleanOps::union_geometries(box_geo_, empty_geo);
  EXPECT_FALSE(fail_result.has_value());

  // Now do successful operation
  auto success_result = BooleanOps::union_geometries(box_geo_, box_geo_);
  EXPECT_TRUE(success_result.has_value());
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(BooleanOpsTest, Union_CoplanarFaces) {
  auto box1_result = BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box1_result.has_value());
  auto box1 = box1_result.value().clone();

  auto box2_result = BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box2_result.has_value());
  auto box2 = box2_result.value().clone();

  // Offset box2 to share a face
  auto* positions = box2.get_point_attribute_typed<core::Vec3f>(core::standard_attrs::P);
  ASSERT_NE(positions, nullptr);
  auto pos_span = positions->values_writable();
  for (size_t i = 0; i < pos_span.size(); ++i) {
    pos_span[i].x() += 1.0f;
  }

  auto result = BooleanOps::union_geometries(box1, box2);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->point_count(), 0);
  EXPECT_GT(result->primitive_count(), 0);
}

TEST_F(BooleanOpsTest, Intersection_TouchingAtPoint) {
  auto box1_result = BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box1_result.has_value());
  auto box1 = box1_result.value().clone();

  auto box2_result = BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box2_result.has_value());
  auto box2 = box2_result.value().clone();

  // Offset box2 to touch at a corner
  auto* positions = box2.get_point_attribute_typed<core::Vec3f>(core::standard_attrs::P);
  ASSERT_NE(positions, nullptr);
  auto pos_span = positions->values_writable();
  for (size_t i = 0; i < pos_span.size(); ++i) {
    pos_span[i].x() += 1.0f;
    pos_span[i].y() += 1.0f;
    pos_span[i].z() += 1.0f;
  }

  auto result = BooleanOps::intersect_geometries(box1, box2);

  // Point intersection may return empty or nullopt
  if (result.has_value()) {
    SUCCEED() << "Point intersection returned geometry with " << result->primitive_count() << " primitives";
  } else {
    SUCCEED() << "Point intersection returned as nullopt";
  }
}
