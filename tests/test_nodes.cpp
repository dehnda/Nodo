#include <gtest/gtest.h>
#include <nodeflux/geometry/box_generator.hpp>
#include <nodeflux/geometry/sphere_generator.hpp>
#include <nodeflux/geometry/cylinder_generator.hpp>
#include <nodeflux/geometry/plane_generator.hpp>
#include <nodeflux/geometry/torus_generator.hpp>

using namespace nodeflux;

class PrimitiveGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Basic setup for geometry generator testing
    }
};

// Box Generator Tests
TEST_F(PrimitiveGeneratorTest, BoxGeneration) {
    auto mesh_result = geometry::BoxGenerator::generate(2.0, 1.5, 0.8);

    ASSERT_TRUE(mesh_result.has_value());
    EXPECT_GT(mesh_result->vertices().rows(), 0);
    EXPECT_GT(mesh_result->faces().rows(), 0);
}

TEST_F(PrimitiveGeneratorTest, BoxWithDifferentDimensions) {
    auto mesh_result = geometry::BoxGenerator::generate(3.0, 2.0, 1.5);

    ASSERT_TRUE(mesh_result.has_value());
    EXPECT_GT(mesh_result->vertices().rows(), 0);
    EXPECT_GT(mesh_result->faces().rows(), 0);
}

// Sphere Generator Tests
TEST_F(PrimitiveGeneratorTest, SphereGeneration) {
    auto mesh_result = geometry::SphereGenerator::generate_uv_sphere(1.5, 16, 8);

    ASSERT_TRUE(mesh_result.has_value());
    EXPECT_GT(mesh_result->vertices().rows(), 0);
    EXPECT_GT(mesh_result->faces().rows(), 0);
}

// Cylinder Generator Tests
TEST_F(PrimitiveGeneratorTest, CylinderGeneration) {
    auto mesh_result = geometry::CylinderGenerator::generate(0.5, 2.0, 12);

    ASSERT_TRUE(mesh_result.has_value());
    EXPECT_GT(mesh_result->vertices().rows(), 0);
    EXPECT_GT(mesh_result->faces().rows(), 0);
}

// Plane Generator Tests
TEST_F(PrimitiveGeneratorTest, PlaneGeneration) {
    auto mesh_result = geometry::PlaneGenerator::generate(2.0, 1.5, 4, 3);

    ASSERT_TRUE(mesh_result.has_value());
    EXPECT_GT(mesh_result->vertices().rows(), 0);
    EXPECT_GT(mesh_result->faces().rows(), 0);
}

// Torus Generator Tests
TEST_F(PrimitiveGeneratorTest, TorusGeneration) {
    auto mesh_result = geometry::TorusGenerator::generate(1.0, 0.3, 24, 12);

    ASSERT_TRUE(mesh_result.has_value());
    EXPECT_GT(mesh_result->vertices().rows(), 0);
    EXPECT_GT(mesh_result->faces().rows(), 0);
}

TEST_F(PrimitiveGeneratorTest, TorusWithDifferentParameters) {
    auto mesh_result = geometry::TorusGenerator::generate(2.0, 0.5, 48, 16);

    ASSERT_TRUE(mesh_result.has_value());
    EXPECT_GT(mesh_result->vertices().rows(), 0);
    EXPECT_GT(mesh_result->faces().rows(), 0);
}
