#include <gtest/gtest.h>
#include <nodo/core/geometry_container.hpp>
#include <nodo/core/standard_attributes.hpp>
#include <nodo/geometry/box_generator.hpp>
#include <nodo/geometry/cylinder_generator.hpp>
#include <nodo/geometry/plane_generator.hpp>
#include <nodo/geometry/sphere_generator.hpp>
#include <nodo/geometry/torus_generator.hpp>

using namespace nodo;

class PrimitiveGeneratorTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Basic setup for geometry generator testing
  }
};

// Box Generator Tests
TEST_F(PrimitiveGeneratorTest, BoxGeneration) {
  auto container_result = geometry::BoxGenerator::generate(2.0, 1.5, 0.8);

  ASSERT_TRUE(container_result.has_value());
  EXPECT_GT(container_result->point_count(), 0);
  EXPECT_GT(container_result->topology().primitive_count(), 0);
}

TEST_F(PrimitiveGeneratorTest, BoxWithDifferentDimensions) {
  auto container_result = geometry::BoxGenerator::generate(3.0, 2.0, 1.5);

  ASSERT_TRUE(container_result.has_value());
  EXPECT_GT(container_result->point_count(), 0);
  EXPECT_GT(container_result->topology().primitive_count(), 0);
}

// Sphere Generator Tests
TEST_F(PrimitiveGeneratorTest, SphereGeneration) {
  auto container_result = geometry::SphereGenerator::generate_uv_sphere(1.5, 16, 8);

  ASSERT_TRUE(container_result.has_value());
  EXPECT_GT(container_result->point_count(), 0);
  EXPECT_GT(container_result->topology().primitive_count(), 0);
}

// Cylinder Generator Tests
TEST_F(PrimitiveGeneratorTest, CylinderGeneration) {
  auto container_result = geometry::CylinderGenerator::generate(0.5, 2.0, 12);

  ASSERT_TRUE(container_result.has_value());
  EXPECT_GT(container_result->point_count(), 0);
  EXPECT_GT(container_result->topology().primitive_count(), 0);
}

// Plane Generator Tests
TEST_F(PrimitiveGeneratorTest, PlaneGeneration) {
  auto container_result = geometry::PlaneGenerator::generate(2.0, 1.5, 4, 3);

  ASSERT_TRUE(container_result.has_value());
  EXPECT_GT(container_result->point_count(), 0);
  EXPECT_GT(container_result->topology().primitive_count(), 0);
}

// Torus Generator Tests
TEST_F(PrimitiveGeneratorTest, TorusGeneration) {
  auto container_result = geometry::TorusGenerator::generate(1.0, 0.3, 24, 12);

  ASSERT_TRUE(container_result.has_value());
  EXPECT_GT(container_result->point_count(), 0);
  EXPECT_GT(container_result->topology().primitive_count(), 0);
}

TEST_F(PrimitiveGeneratorTest, TorusWithDifferentParameters) {
  auto container_result = geometry::TorusGenerator::generate(2.0, 0.5, 48, 16);

  ASSERT_TRUE(container_result.has_value());
  EXPECT_GT(container_result->point_count(), 0);
  EXPECT_GT(container_result->topology().primitive_count(), 0);
}
