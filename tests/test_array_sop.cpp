#include "nodo/geometry/sphere_generator.hpp"
#include "nodo/sop/array_sop.hpp"
#include <gtest/gtest.h>

using namespace nodo;

class ArraySOPTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a simple sphere for testing (now returns GeometryContainer)
    auto sphere_result =
        geometry::SphereGenerator::generate_uv_sphere(0.5, 4, 4);
    ASSERT_TRUE(sphere_result.has_value());

    // Use GeometryContainer directly via clone()
    input_geometry_ = std::make_shared<core::GeometryContainer>(
        sphere_result.value().clone());
  }

  std::shared_ptr<core::GeometryContainer> input_geometry_;
};

TEST_F(ArraySOPTest, LinearArrayCreation) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_array");

  // Configure linear array
  array_node->set_parameter("array_type", 0); // LINEAR = 0
  array_node->set_parameter("count", 5);
  array_node->set_parameter("linear_offset_x", 1.0F);
  array_node->set_parameter("linear_offset_y", 0.0F);
  array_node->set_parameter("linear_offset_z", 0.0F);

  // Connect input using set_input_data()
  array_node->set_input_data(0, input_geometry_);

  // Execute
  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);

  // Verify output has 5 copies
  size_t input_point_count = input_geometry_->topology().point_count();
  EXPECT_EQ(result->topology().point_count(), input_point_count * 5);
}

// NOTE: Radial and Grid array implementations are stubs that just clone input
// These tests are disabled until those methods are properly implemented
TEST_F(ArraySOPTest, RadialArrayCreation) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_radial");

  // Configure radial array
  array_node->set_parameter("array_type", 1); // RADIAL = 1
  array_node->set_parameter("count", 8);
  array_node->set_parameter("radial_radius", 2.0F);
  array_node->set_parameter("angle_step", 45.0F); // 360/8 = 45 degrees

  // Connect input using set_input_data()
  array_node->set_input_data(0, input_geometry_);

  // Execute
  auto result = array_node->cook();

  ASSERT_NE(result, nullptr) << "Error: " << array_node->get_last_error();

  // Verify output has 8 copies
  size_t input_point_count = input_geometry_->topology().point_count();
  EXPECT_EQ(result->topology().point_count(), input_point_count * 8);
}

TEST_F(ArraySOPTest, GridArrayCreation) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_grid");

  // Configure grid array
  array_node->set_parameter("array_type", 2); // GRID = 2
  array_node->set_parameter("grid_width", 3);
  array_node->set_parameter("grid_height", 4); // 3x4 grid = 12 copies
  array_node->set_parameter("grid_spacing_x", 1.5F);
  array_node->set_parameter("grid_spacing_y", 2.0F);

  // Connect input using set_input_data()
  array_node->set_input_data(0, input_geometry_);

  // Execute
  auto result = array_node->cook();

  ASSERT_NE(result, nullptr) << "Error: " << array_node->get_last_error();

  // Verify output has 12 copies (3x4)
  size_t input_point_count = input_geometry_->topology().point_count();
  EXPECT_EQ(result->topology().point_count(), input_point_count * 12);
}

// Note: Instance attributes were part of the old GeometryData system
// The new GeometryContainer doesn't have a concept of instance_id attributes
// These tests would need ArraySOP to be updated to add such attributes if
// needed Skipping attribute-specific tests for now

TEST_F(ArraySOPTest, CachingWorks) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_cache");

  // Configure array
  array_node->set_parameter("array_type", 0); // LINEAR = 0
  array_node->set_parameter("count", 2);
  array_node->set_parameter("linear_offset_x", 1.0F);
  array_node->set_parameter("linear_offset_y", 0.0F);
  array_node->set_parameter("linear_offset_z", 0.0F);

  // Connect input using set_input_data()
  array_node->set_input_data(0, input_geometry_);

  // First cook
  auto result1 = array_node->cook();
  ASSERT_NE(result1, nullptr);
  auto cook_time1 = array_node->get_cook_duration();

  // Second cook (should be cached)
  auto result2 = array_node->cook();
  ASSERT_NE(result2, nullptr);
  auto cook_time2 = array_node->get_cook_duration();

  // Should return same result
  EXPECT_EQ(result1, result2);

  // Should be CLEAN state
  EXPECT_EQ(array_node->get_state(), sop::SOPNode::ExecutionState::CLEAN);
}

TEST_F(ArraySOPTest, MarkDirtyInvalidatesCache) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_dirty");

  // Configure and execute
  array_node->set_parameter("array_type", 0); // LINEAR = 0
  array_node->set_parameter("count", 2);

  // Connect input using set_input_data()
  array_node->set_input_data(0, input_geometry_);

  auto result1 = array_node->cook();
  ASSERT_NE(result1, nullptr);
  EXPECT_EQ(array_node->get_state(), sop::SOPNode::ExecutionState::CLEAN);

  // Change parameter - should mark dirty
  array_node->set_parameter("count", 3);
  EXPECT_EQ(array_node->get_state(), sop::SOPNode::ExecutionState::DIRTY);

  // Cook again - should recalculate
  auto result2 = array_node->cook();
  ASSERT_NE(result2, nullptr);

  // Results should differ (different point counts)
  EXPECT_NE(result1->topology().point_count(),
            result2->topology().point_count());
}

TEST_F(ArraySOPTest, NoInputReturnsError) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_no_input");

  // Configure but don't connect input
  array_node->set_parameter("array_type", 0); // LINEAR = 0
  array_node->set_parameter("count", 2);

  // Execute without input
  auto result = array_node->cook();

  EXPECT_EQ(result, nullptr);
  EXPECT_EQ(array_node->get_state(), sop::SOPNode::ExecutionState::ERROR);
  EXPECT_FALSE(array_node->get_last_error().empty());
}

// ===== NEW COMPREHENSIVE TESTS =====

TEST_F(ArraySOPTest, ParameterRegistration) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_params");

  // Test array_type parameter
  EXPECT_TRUE(array_node->has_parameter("array_type"));
  EXPECT_EQ(array_node->get_parameter<int>("array_type", -1), 0);

  // Test count parameter
  EXPECT_TRUE(array_node->has_parameter("count"));
  EXPECT_EQ(array_node->get_parameter<int>("count", -1), 3);

  // Test linear parameters
  EXPECT_TRUE(array_node->has_parameter("linear_offset_x"));
  EXPECT_TRUE(array_node->has_parameter("linear_offset_y"));
  EXPECT_TRUE(array_node->has_parameter("linear_offset_z"));
  EXPECT_FLOAT_EQ(array_node->get_parameter<float>("linear_offset_x", -1.0F),
                  1.0F);

  // Test radial parameters
  EXPECT_TRUE(array_node->has_parameter("radial_center_x"));
  EXPECT_TRUE(array_node->has_parameter("radial_center_y"));
  EXPECT_TRUE(array_node->has_parameter("radial_center_z"));
  EXPECT_TRUE(array_node->has_parameter("radial_radius"));
  EXPECT_TRUE(array_node->has_parameter("angle_step"));
  EXPECT_FLOAT_EQ(array_node->get_parameter<float>("radial_radius", -1.0F),
                  2.0F);
  EXPECT_FLOAT_EQ(array_node->get_parameter<float>("angle_step", -1.0F), 60.0F);

  // Test grid parameters
  EXPECT_TRUE(array_node->has_parameter("grid_width"));
  EXPECT_TRUE(array_node->has_parameter("grid_height"));
  EXPECT_TRUE(array_node->has_parameter("grid_spacing_x"));
  EXPECT_TRUE(array_node->has_parameter("grid_spacing_y"));
  EXPECT_EQ(array_node->get_parameter<int>("grid_width", -1), 3);
  EXPECT_EQ(array_node->get_parameter<int>("grid_height", -1), 3);
}

TEST_F(ArraySOPTest, EmptyInputGeometry) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_empty");

  // Create empty geometry
  auto empty_geo = std::make_shared<core::GeometryContainer>();
  empty_geo->set_point_count(0);

  array_node->set_parameter("array_type", 0); // LINEAR
  array_node->set_parameter("count", 2);
  array_node->set_input_data(0, empty_geo);

  auto result = array_node->cook();

  EXPECT_EQ(result, nullptr);
  EXPECT_EQ(array_node->get_state(), sop::SOPNode::ExecutionState::ERROR);
  EXPECT_FALSE(array_node->get_last_error().empty());
}

TEST_F(ArraySOPTest, LinearArrayWithSingleCopy) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_single");

  array_node->set_parameter("array_type", 0); // LINEAR
  array_node->set_parameter("count", 1);
  array_node->set_parameter("linear_offset_x", 5.0F);
  array_node->set_input_data(0, input_geometry_);

  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);
  size_t input_count = input_geometry_->topology().point_count();
  EXPECT_EQ(result->topology().point_count(), input_count);
}

TEST_F(ArraySOPTest, LinearArrayWithZeroOffset) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_zero_offset");

  array_node->set_parameter("array_type", 0); // LINEAR
  array_node->set_parameter("count", 3);
  array_node->set_parameter("linear_offset_x", 0.0F);
  array_node->set_parameter("linear_offset_y", 0.0F);
  array_node->set_parameter("linear_offset_z", 0.0F);
  array_node->set_input_data(0, input_geometry_);

  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);
  size_t input_count = input_geometry_->topology().point_count();
  EXPECT_EQ(result->topology().point_count(), input_count * 3);
}

TEST_F(ArraySOPTest, LinearArrayWithNegativeOffset) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_negative");

  array_node->set_parameter("array_type", 0); // LINEAR
  array_node->set_parameter("count", 4);
  array_node->set_parameter("linear_offset_x", -2.0F);
  array_node->set_parameter("linear_offset_y", 1.5F);
  array_node->set_parameter("linear_offset_z", -0.5F);
  array_node->set_input_data(0, input_geometry_);

  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);
  size_t input_count = input_geometry_->topology().point_count();
  EXPECT_EQ(result->topology().point_count(), input_count * 4);
}

TEST_F(ArraySOPTest, RadialArrayWithSingleCopy) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_radial_single");

  array_node->set_parameter("array_type", 1); // RADIAL
  array_node->set_parameter("count", 1);
  array_node->set_parameter("radial_radius", 5.0F);
  array_node->set_input_data(0, input_geometry_);

  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);
  size_t input_count = input_geometry_->topology().point_count();
  EXPECT_EQ(result->topology().point_count(), input_count);
}

TEST_F(ArraySOPTest, RadialArrayWithCustomCenter) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_radial_center");

  array_node->set_parameter("array_type", 1); // RADIAL
  array_node->set_parameter("count", 6);
  array_node->set_parameter("radial_center_x", 10.0F);
  array_node->set_parameter("radial_center_y", 5.0F);
  array_node->set_parameter("radial_center_z", -3.0F);
  array_node->set_parameter("radial_radius", 3.0F);
  array_node->set_parameter("angle_step", 60.0F);
  array_node->set_input_data(0, input_geometry_);

  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);
  size_t input_count = input_geometry_->topology().point_count();
  EXPECT_EQ(result->topology().point_count(), input_count * 6);
}

TEST_F(ArraySOPTest, RadialArrayWithZeroRadius) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_radial_zero");

  array_node->set_parameter("array_type", 1); // RADIAL
  array_node->set_parameter("count", 4);
  array_node->set_parameter("radial_radius", 0.0F);
  array_node->set_parameter("angle_step", 90.0F);
  array_node->set_input_data(0, input_geometry_);

  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);
  size_t input_count = input_geometry_->topology().point_count();
  EXPECT_EQ(result->topology().point_count(), input_count * 4);
}

TEST_F(ArraySOPTest, RadialArrayWithLargeAngle) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_radial_large");

  array_node->set_parameter("array_type", 1); // RADIAL
  array_node->set_parameter("count", 3);
  array_node->set_parameter("radial_radius", 2.0F);
  array_node->set_parameter("angle_step", 120.0F);
  array_node->set_input_data(0, input_geometry_);

  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);
  size_t input_count = input_geometry_->topology().point_count();
  EXPECT_EQ(result->topology().point_count(), input_count * 3);
}

TEST_F(ArraySOPTest, GridArraySingleCell) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_grid_single");

  array_node->set_parameter("array_type", 2); // GRID
  array_node->set_parameter("grid_width", 1);
  array_node->set_parameter("grid_height", 1);
  array_node->set_parameter("grid_spacing_x", 2.0F);
  array_node->set_parameter("grid_spacing_y", 2.0F);
  array_node->set_input_data(0, input_geometry_);

  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);
  size_t input_count = input_geometry_->topology().point_count();
  EXPECT_EQ(result->topology().point_count(), input_count);
}

TEST_F(ArraySOPTest, GridArraySingleRow) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_grid_row");

  array_node->set_parameter("array_type", 2); // GRID
  array_node->set_parameter("grid_width", 5);
  array_node->set_parameter("grid_height", 1);
  array_node->set_parameter("grid_spacing_x", 1.0F);
  array_node->set_parameter("grid_spacing_y", 1.0F);
  array_node->set_input_data(0, input_geometry_);

  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);
  size_t input_count = input_geometry_->topology().point_count();
  EXPECT_EQ(result->topology().point_count(), input_count * 5);
}

TEST_F(ArraySOPTest, GridArraySingleColumn) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_grid_col");

  array_node->set_parameter("array_type", 2); // GRID
  array_node->set_parameter("grid_width", 1);
  array_node->set_parameter("grid_height", 7);
  array_node->set_parameter("grid_spacing_x", 1.0F);
  array_node->set_parameter("grid_spacing_y", 1.5F);
  array_node->set_input_data(0, input_geometry_);

  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);
  size_t input_count = input_geometry_->topology().point_count();
  EXPECT_EQ(result->topology().point_count(), input_count * 7);
}

TEST_F(ArraySOPTest, GridArrayCustomSpacing) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_grid_spacing");

  array_node->set_parameter("array_type", 2); // GRID
  array_node->set_parameter("grid_width", 2);
  array_node->set_parameter("grid_height", 3);
  array_node->set_parameter("grid_spacing_x", 5.0F);
  array_node->set_parameter("grid_spacing_y", 3.5F);
  array_node->set_input_data(0, input_geometry_);

  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);
  size_t input_count = input_geometry_->topology().point_count();
  EXPECT_EQ(result->topology().point_count(), input_count * 6);
}

TEST_F(ArraySOPTest, GridArrayLarge) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_grid_large");

  array_node->set_parameter("array_type", 2); // GRID
  array_node->set_parameter("grid_width", 10);
  array_node->set_parameter("grid_height", 10);
  array_node->set_parameter("grid_spacing_x", 0.5F);
  array_node->set_parameter("grid_spacing_y", 0.5F);
  array_node->set_input_data(0, input_geometry_);

  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);
  size_t input_count = input_geometry_->topology().point_count();
  EXPECT_EQ(result->topology().point_count(), input_count * 100);
}

TEST_F(ArraySOPTest, SwitchArrayTypes) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_switch");

  array_node->set_input_data(0, input_geometry_);
  size_t input_count = input_geometry_->topology().point_count();

  // Start with linear
  array_node->set_parameter("array_type", 0); // LINEAR
  array_node->set_parameter("count", 3);
  array_node->set_parameter("linear_offset_x", 1.0F);

  auto result1 = array_node->cook();
  ASSERT_NE(result1, nullptr);
  EXPECT_EQ(result1->topology().point_count(), input_count * 3);

  // Switch to radial
  array_node->set_parameter("array_type", 1); // RADIAL
  array_node->set_parameter("count", 4);

  auto result2 = array_node->cook();
  ASSERT_NE(result2, nullptr);
  EXPECT_EQ(result2->topology().point_count(), input_count * 4);

  // Switch to grid
  array_node->set_parameter("array_type", 2); // GRID
  array_node->set_parameter("grid_width", 2);
  array_node->set_parameter("grid_height", 3);

  auto result3 = array_node->cook();
  ASSERT_NE(result3, nullptr);
  EXPECT_EQ(result3->topology().point_count(), input_count * 6);
}

TEST_F(ArraySOPTest, VerifyPrimitiveTopology) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_topology");

  array_node->set_parameter("array_type", 0); // LINEAR
  array_node->set_parameter("count", 2);
  array_node->set_parameter("linear_offset_x", 1.0F);
  array_node->set_input_data(0, input_geometry_);

  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);

  size_t input_prim_count = input_geometry_->topology().primitive_count();
  size_t output_prim_count = result->topology().primitive_count();

  // Should have 2x the number of primitives
  EXPECT_EQ(output_prim_count, input_prim_count * 2);
}

TEST_F(ArraySOPTest, VerifyVertexCount) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_vertices");

  array_node->set_parameter("array_type", 1); // RADIAL
  array_node->set_parameter("count", 5);
  array_node->set_input_data(0, input_geometry_);

  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);

  size_t input_vertex_count = input_geometry_->topology().vertex_count();
  size_t output_vertex_count = result->topology().vertex_count();

  // Should have 5x the number of vertices
  EXPECT_EQ(output_vertex_count, input_vertex_count * 5);
}
