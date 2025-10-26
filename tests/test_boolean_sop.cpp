#include <gtest/gtest.h>
#include "nodeflux/sop/boolean_sop.hpp"
#include "nodeflux/geometry/box_generator.hpp"
#include "nodeflux/geometry/sphere_generator.hpp"
#include "nodeflux/core/geometry_container.hpp"
#include <memory>

using namespace nodeflux;

class BooleanSOPTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Generate two simple box geometries for testing
    auto box1_result = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
    auto box2_result = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);

    ASSERT_TRUE(box1_result.has_value());
    ASSERT_TRUE(box2_result.has_value());

    geo1_ = std::make_shared<core::GeometryContainer>(box1_result.value().clone());
    geo2_ = std::make_shared<core::GeometryContainer>(box2_result.value().clone());
  }

  std::shared_ptr<core::GeometryContainer> geo1_;
  std::shared_ptr<core::GeometryContainer> geo2_;
};

TEST_F(BooleanSOPTest, UnionOperation) {
  auto boolean_node = std::make_shared<sop::BooleanSOP>("test_boolean");

  // Connect inputs via ports (using numeric port names)
  auto *port_a = boolean_node->get_input_ports().get_port("0");
  auto *port_b = boolean_node->get_input_ports().get_port("1");

  ASSERT_NE(port_a, nullptr);
  ASSERT_NE(port_b, nullptr);

  port_a->set_data(geo1_);
  port_b->set_data(geo2_);

  // Set operation to UNION (0)
  boolean_node->set_parameter("operation", 0);

  // Execute boolean operation
  auto result = boolean_node->cook();

  // Check result is valid
  ASSERT_NE(result, nullptr);
  EXPECT_GT(result->topology().point_count(), 0);
  EXPECT_GT(result->topology().primitive_count(), 0);
}

TEST_F(BooleanSOPTest, IntersectionOperation) {
  auto boolean_node = std::make_shared<sop::BooleanSOP>("test_intersection");

  // Connect inputs
  auto *port_a = boolean_node->get_input_ports().get_port("0");
  auto *port_b = boolean_node->get_input_ports().get_port("1");

  port_a->set_data(geo1_);
  port_b->set_data(geo2_);

  // Set operation to INTERSECTION (1)
  boolean_node->set_parameter("operation", 1);

  // Execute
  auto result = boolean_node->cook();

  // Check result is valid (intersection of two identical boxes = same box)
  ASSERT_NE(result, nullptr);
  EXPECT_GT(result->topology().point_count(), 0);
}

TEST_F(BooleanSOPTest, DifferenceOperation) {
  auto boolean_node = std::make_shared<sop::BooleanSOP>("test_difference");

  // Create two boxes, one slightly offset so difference is non-empty
  auto box1_result = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
  auto box2_result = geometry::BoxGenerator::generate(0.5, 0.5, 0.5);

  ASSERT_TRUE(box1_result.has_value());
  ASSERT_TRUE(box2_result.has_value());

  auto geo1 = std::make_shared<core::GeometryContainer>(box1_result.value().clone());
  auto geo2 = std::make_shared<core::GeometryContainer>(box2_result.value().clone());

  // Connect inputs
  auto *port_a = boolean_node->get_input_ports().get_port("0");
  auto *port_b = boolean_node->get_input_ports().get_port("1");

  port_a->set_data(geo1);
  port_b->set_data(geo2);

  // Set operation to DIFFERENCE (2)
  boolean_node->set_parameter("operation", 2);

  // Execute
  auto result = boolean_node->cook();

  // Check result is valid (large box - small box = non-empty)
  ASSERT_NE(result, nullptr);
  EXPECT_GT(result->topology().point_count(), 0);
}

TEST_F(BooleanSOPTest, MissingInputA) {
  auto boolean_node = std::make_shared<sop::BooleanSOP>("test_missing_a");

  // Try to execute with no inputs
  auto result = boolean_node->cook();

  // Should return nullptr
  EXPECT_EQ(result, nullptr);
}

TEST_F(BooleanSOPTest, MissingInputB) {
  auto boolean_node = std::make_shared<sop::BooleanSOP>("test_missing_b");

  // Only connect input A
  auto *port_a = boolean_node->get_input_ports().get_port("0");
  port_a->set_data(geo1_);

  // Try to execute
  auto result = boolean_node->cook();

  // Should return nullptr
  EXPECT_EQ(result, nullptr);
}

TEST_F(BooleanSOPTest, InvalidOperationType) {
  auto boolean_node = std::make_shared<sop::BooleanSOP>("test_invalid_op");

  // Connect both inputs
  auto *port_a = boolean_node->get_input_ports().get_port("0");
  auto *port_b = boolean_node->get_input_ports().get_port("1");

  port_a->set_data(geo1_);
  port_b->set_data(geo2_);

  // Set invalid operation type
  boolean_node->set_parameter("operation", 99);

  // Execute should fail gracefully
  auto result = boolean_node->cook();
  EXPECT_EQ(result, nullptr);
}
