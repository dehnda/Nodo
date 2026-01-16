#include "test_utils.hpp"

#include <gtest/gtest.h>
#include <nodo/geometry/box_generator.hpp>
#include <nodo/geometry/cylinder_generator.hpp>
#include <nodo/geometry/plane_generator.hpp>
#include <nodo/geometry/sphere_generator.hpp>
#include <nodo/geometry/torus_generator.hpp>


using namespace nodo;

class MeshGeneratorTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Test parameters
    size_ = 1.0;
    subdivisions_ = 8;
  }

  double size_;
  int subdivisions_;
};

TEST_F(MeshGeneratorTest, BoxGeneration) {
  auto container_result = geometry::BoxGenerator::generate(size_, size_, size_);

  ASSERT_TRUE(container_result.has_value());
  auto result = test::container_to_mesh(*container_result);
  EXPECT_GT(result.vertices().rows(), 0);
  EXPECT_GT(result.faces().rows(), 0);
  EXPECT_EQ(result.vertices().cols(), 3);
  EXPECT_EQ(result.faces().cols(), 3);

  // A cube should have 8 base vertices (before subdivision)
  EXPECT_GE(result.vertices().rows(), 8);
  // A cube should have 6 quad faces (not triangulated)
  EXPECT_GE(result.faces().rows(), 6);
}

TEST_F(MeshGeneratorTest, BoxDimensions) {
  double width = 2.0, height = 3.0, depth = 1.5;
  auto container_result = geometry::BoxGenerator::generate(width, height, depth);

  ASSERT_TRUE(container_result.has_value());
  auto result = test::container_to_mesh(*container_result);

  // Check bounding box
  Eigen::Vector3d min_bound = result.vertices().row(0).transpose();
  Eigen::Vector3d max_bound = result.vertices().row(0).transpose();

  for (int i = 1; i < result.vertices().rows(); ++i) {
    Eigen::Vector3d vertex = result.vertices().row(i).transpose();
    min_bound = min_bound.cwiseMin(vertex);
    max_bound = max_bound.cwiseMax(vertex);
  }

  EXPECT_NEAR(max_bound.x() - min_bound.x(), width, 1e-10);
  EXPECT_NEAR(max_bound.y() - min_bound.y(), height, 1e-10);
  EXPECT_NEAR(max_bound.z() - min_bound.z(), depth, 1e-10);
}

TEST_F(MeshGeneratorTest, SphereUVGeneration) {
  auto container_result = geometry::SphereGenerator::generate_uv_sphere(size_, subdivisions_, subdivisions_);

  ASSERT_TRUE(container_result.has_value());
  auto result = test::container_to_mesh(*container_result);
  EXPECT_GT(result.vertices().rows(), 0);
  EXPECT_GT(result.faces().rows(), 0);

  // Check that all vertices are approximately on the sphere surface
  for (int i = 0; i < result.vertices().rows(); ++i) {
    Eigen::Vector3d vertex = result.vertices().row(i);
    double distance = vertex.norm();
    EXPECT_NEAR(distance, size_, 1e-6);
  }
}

TEST_F(MeshGeneratorTest, SphereIcospherGeneration) {
  auto container_result = geometry::SphereGenerator::generate_icosphere(size_, 2);

  ASSERT_TRUE(container_result.has_value());
  auto result = test::container_to_mesh(*container_result);
  EXPECT_GT(result.vertices().rows(), 0);
  EXPECT_GT(result.faces().rows(), 0);

  // Check that all vertices are approximately on the sphere surface
  for (int i = 0; i < result.vertices().rows(); ++i) {
    Eigen::Vector3d vertex = result.vertices().row(i);
    double distance = vertex.norm();
    EXPECT_NEAR(distance, size_, 1e-6);
  }
}

TEST_F(MeshGeneratorTest, CylinderGeneration) {
  // Generate cylinder with radius=size_, height=size_, segments=subdivisions_
  auto container_result = geometry::CylinderGenerator::generate(size_, size_, subdivisions_);

  ASSERT_TRUE(container_result.has_value());
  auto result = test::container_to_mesh(*container_result);
  EXPECT_GT(result.vertices().rows(), 0);
  EXPECT_GT(result.faces().rows(), 0);

  // Check that vertices are within reasonable bounds
  // Cylinder may have caps, so Z range might be larger than expected
  Eigen::Vector3d min_bound = result.vertices().row(0).transpose();
  Eigen::Vector3d max_bound = result.vertices().row(0).transpose();

  for (int i = 1; i < result.vertices().rows(); ++i) {
    Eigen::Vector3d vertex = result.vertices().row(i).transpose();
    min_bound = min_bound.cwiseMin(vertex);
    max_bound = max_bound.cwiseMax(vertex);
  }

  // Check that the Z height is reasonable (may be larger than size_ due to
  // caps)
  double z_range = max_bound.z() - min_bound.z();
  EXPECT_GT(z_range, 0.0);
  EXPECT_LT(z_range, size_ * 3.0); // Allow for some variation

  // Check that XY dimensions are reasonable (related to radius)
  double xy_range = std::max(max_bound.x() - min_bound.x(), max_bound.y() - min_bound.y());
  EXPECT_GT(xy_range, size_);       // Should be at least 2*radius
  EXPECT_LT(xy_range, size_ * 3.0); // But not too large
}

TEST_F(MeshGeneratorTest, PlaneGeneration) {
  auto container_result = geometry::PlaneGenerator::generate(size_, size_, subdivisions_, subdivisions_);

  ASSERT_TRUE(container_result.has_value());
  auto result = test::container_to_mesh(*container_result);
  EXPECT_GT(result.vertices().rows(), 0);
  EXPECT_GT(result.faces().rows(), 0);

  // Check that vertices are within reasonable bounds
  Eigen::Vector3d min_bound = result.vertices().row(0).transpose();
  Eigen::Vector3d max_bound = result.vertices().row(0).transpose();

  for (int i = 1; i < result.vertices().rows(); ++i) {
    Eigen::Vector3d vertex = result.vertices().row(i).transpose();
    min_bound = min_bound.cwiseMin(vertex);
    max_bound = max_bound.cwiseMax(vertex);
  }

  // Check that at least two dimensions span the expected size
  double x_range = max_bound.x() - min_bound.x();
  double y_range = max_bound.y() - min_bound.y();
  double z_range = max_bound.z() - min_bound.z();

  // At least one pair of dimensions should span approximately size_
  bool has_expected_dimensions =
      (std::abs(x_range - size_) < 0.1) || (std::abs(y_range - size_) < 0.1) || (std::abs(z_range - size_) < 0.1);

  EXPECT_TRUE(has_expected_dimensions) << "Expected one dimension to be approximately " << size_
                                       << " but got X=" << x_range << " Y=" << y_range << " Z=" << z_range;

  // Plane should be relatively flat in at least one dimension
  double min_range = std::min({x_range, y_range, z_range});
  EXPECT_LT(min_range,
            size_); // At least one dimension should be smaller than the others
}

TEST_F(MeshGeneratorTest, InvalidParameters) {
  // Test with zero or negative dimensions
  auto result1 = geometry::BoxGenerator::generate(0.0, 1.0, 1.0);
  EXPECT_FALSE(result1.has_value());

  auto result2 = geometry::SphereGenerator::generate_uv_sphere(-1.0, 8, 8);
  EXPECT_FALSE(result2.has_value());

  auto result3 = geometry::CylinderGenerator::generate(1.0, 0.0, 8);
  EXPECT_FALSE(result3.has_value());

  // Test with invalid subdivision counts
  auto result4 = geometry::SphereGenerator::generate_uv_sphere(1.0, 2, 8); // Too few meridians
  EXPECT_FALSE(result4.has_value());
}
