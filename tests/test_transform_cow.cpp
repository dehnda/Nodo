#include <gtest/gtest.h>
#include <nodo/geometry/box_generator.hpp>
#include <nodo/sop/transform_sop.hpp>

using namespace nodo;

class TransformCOWTest : public ::testing::Test {};

// Test: Transform with COW optimization (linear chain)
TEST_F(TransformCOWTest, LinearChainZeroCopy) {
  // Create a box
  auto box = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box.has_value());

  // Create transform node
  sop::TransformSOP transform("transform1");

  // Set input - simulate what execution engine does
  transform.set_input_data(0, std::make_shared<core::GeometryContainer>(std::move(*box)));

  // Execute transform
  auto result = transform.cook();
  ASSERT_NE(result, nullptr);
  EXPECT_GT(result->point_count(), 0);
}

// Test: Transform on shared geometry (triggers COW)
TEST_F(TransformCOWTest, SharedInputTriggersCOW) {
  // Create a box
  auto box = geometry::BoxGenerator::generate(2.0, 1.0, 0.5);
  ASSERT_TRUE(box.has_value());

  auto box_geo = std::make_shared<core::GeometryContainer>(std::move(*box));

  // Create two transform nodes sharing the same input
  sop::TransformSOP transform1("transform1");
  sop::TransformSOP transform2("transform2");

  // Both transforms use the same input (simulates branching)
  transform1.set_input_data(0, box_geo);
  transform2.set_input_data(0, box_geo);

  // Set different parameters
  transform1.set_parameter("translate", Eigen::Vector3f(1.0F, 0.0F, 0.0F));
  transform2.set_parameter("translate", Eigen::Vector3f(0.0F, 1.0F, 0.0F));

  // Execute both
  auto result1 = transform1.cook();
  auto result2 = transform2.cook();

  ASSERT_NE(result1, nullptr);
  ASSERT_NE(result2, nullptr);

  // Results should be different
  EXPECT_NE(result1.get(), result2.get());

  // Check that translations were applied differently
  auto* pos1 = result1->get_point_attribute_typed<core::Vec3f>("P");
  auto* pos2 = result2->get_point_attribute_typed<core::Vec3f>("P");

  ASSERT_NE(pos1, nullptr);
  ASSERT_NE(pos2, nullptr);

  // First transform should have moved in X
  // Second transform should have moved in Y
  // (Exact values depend on box center, but centroid should differ)
  core::Vec3f centroid1(0, 0, 0);
  core::Vec3f centroid2(0, 0, 0);

  for (size_t i = 0; i < pos1->size(); ++i) {
    centroid1 += (*pos1)[i];
  }
  centroid1 /= static_cast<float>(pos1->size());

  for (size_t i = 0; i < pos2->size(); ++i) {
    centroid2 += (*pos2)[i];
  }
  centroid2 /= static_cast<float>(pos2->size());

  // Centroid1 should be offset in X (~1.0), centroid2 in Y (~1.0)
  EXPECT_NEAR(centroid1.x(), 1.0F, 0.01F);
  EXPECT_NEAR(centroid1.y(), 0.0F, 0.01F);

  EXPECT_NEAR(centroid2.x(), 0.0F, 0.01F);
  EXPECT_NEAR(centroid2.y(), 1.0F, 0.01F);
}

// Test: Transform preserves original when branching
TEST_F(TransformCOWTest, OriginalPreservedAfterCOW) {
  // Create a box at origin
  auto box = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box.has_value());

  auto box_geo = std::make_shared<core::GeometryContainer>(std::move(*box));

  // Get original centroid
  auto* original_pos = box_geo->get_point_attribute_typed<core::Vec3f>("P");
  ASSERT_NE(original_pos, nullptr);

  core::Vec3f original_centroid(0, 0, 0);
  for (size_t i = 0; i < original_pos->size(); ++i) {
    original_centroid += (*original_pos)[i];
  }
  original_centroid /= static_cast<float>(original_pos->size());

  // Transform with large translation
  sop::TransformSOP transform("transform1");
  transform.set_input_data(0, box_geo);
  transform.set_parameter("translate", Eigen::Vector3f(10.0F, 10.0F, 10.0F));

  auto result = transform.cook();
  ASSERT_NE(result, nullptr);

  // Original should be unchanged
  core::Vec3f unchanged_centroid(0, 0, 0);
  for (size_t i = 0; i < original_pos->size(); ++i) {
    unchanged_centroid += (*original_pos)[i];
  }
  unchanged_centroid /= static_cast<float>(original_pos->size());

  // Original centroid should still be at ~(0,0,0)
  EXPECT_NEAR(unchanged_centroid.x(), original_centroid.x(), 0.001F);
  EXPECT_NEAR(unchanged_centroid.y(), original_centroid.y(), 0.001F);
  EXPECT_NEAR(unchanged_centroid.z(), original_centroid.z(), 0.001F);

  // Result should be translated
  auto* result_pos = result->get_point_attribute_typed<core::Vec3f>("P");
  ASSERT_NE(result_pos, nullptr);

  core::Vec3f result_centroid(0, 0, 0);
  for (size_t i = 0; i < result_pos->size(); ++i) {
    result_centroid += (*result_pos)[i];
  }
  result_centroid /= static_cast<float>(result_pos->size());

  // Result should be at ~(10,10,10)
  EXPECT_NEAR(result_centroid.x(), 10.0F, 0.01F);
  EXPECT_NEAR(result_centroid.y(), 10.0F, 0.01F);
  EXPECT_NEAR(result_centroid.z(), 10.0F, 0.01F);
}
