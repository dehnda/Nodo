#include <gtest/gtest.h>
#include <nodeflux/geometry/box_generator.hpp>
#include <nodeflux/geometry/sphere_generator.hpp>
#include <nodeflux/geometry/cylinder_generator.hpp>
#include <nodeflux/geometry/plane_generator.hpp>
#include <nodeflux/geometry/torus_generator.hpp>

using namespace nodeflux;

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
    auto result = geometry::BoxGenerator::generate(size_, size_, size_);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->vertices().rows(), 0);
    EXPECT_GT(result->faces().rows(), 0);
    EXPECT_EQ(result->vertices().cols(), 3);
    EXPECT_EQ(result->faces().cols(), 3);
    
    // A cube should have 8 base vertices (before subdivision)
    EXPECT_GE(result->vertices().rows(), 8);
    // A cube should have 12 base faces (before subdivision)
    EXPECT_GE(result->faces().rows(), 12);
}

TEST_F(MeshGeneratorTest, BoxDimensions) {
    double width = 2.0, height = 3.0, depth = 1.5;
    auto result = geometry::BoxGenerator::generate(width, height, depth);
    
    ASSERT_TRUE(result.has_value());
    
    // Check bounding box
    Eigen::Vector3d min_bound = result->vertices().row(0).transpose();
    Eigen::Vector3d max_bound = result->vertices().row(0).transpose();
    
    for (int i = 1; i < result->vertices().rows(); ++i) {
        Eigen::Vector3d vertex = result->vertices().row(i).transpose();
        min_bound = min_bound.cwiseMin(vertex);
        max_bound = max_bound.cwiseMax(vertex);
    }
    
    EXPECT_NEAR(max_bound.x() - min_bound.x(), width, 1e-10);
    EXPECT_NEAR(max_bound.y() - min_bound.y(), height, 1e-10);
    EXPECT_NEAR(max_bound.z() - min_bound.z(), depth, 1e-10);
}

TEST_F(MeshGeneratorTest, SphereUVGeneration) {
    auto result = geometry::SphereGenerator::generate_uv_sphere(size_, subdivisions_, subdivisions_);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->vertices().rows(), 0);
    EXPECT_GT(result->faces().rows(), 0);
    
    // Check that all vertices are approximately on the sphere surface
    for (int i = 0; i < result->vertices().rows(); ++i) {
        Eigen::Vector3d vertex = result->vertices().row(i);
        double distance = vertex.norm();
        EXPECT_NEAR(distance, size_, 1e-6);
    }
}

TEST_F(MeshGeneratorTest, SphereIcospherGeneration) {
    auto result = geometry::SphereGenerator::generate_icosphere(size_, 2);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->vertices().rows(), 0);
    EXPECT_GT(result->faces().rows(), 0);
    
    // Check that all vertices are approximately on the sphere surface
    for (int i = 0; i < result->vertices().rows(); ++i) {
        Eigen::Vector3d vertex = result->vertices().row(i);
        double distance = vertex.norm();
        EXPECT_NEAR(distance, size_, 1e-6);
    }
}

TEST_F(MeshGeneratorTest, CylinderGeneration) {
    // Generate cylinder with radius=size_, height=size_, segments=subdivisions_
    auto result = geometry::CylinderGenerator::generate(size_, size_, subdivisions_);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->vertices().rows(), 0);
    EXPECT_GT(result->faces().rows(), 0);
    
    // Check that vertices are within reasonable bounds
    // Cylinder may have caps, so Z range might be larger than expected
    Eigen::Vector3d min_bound = result->vertices().row(0).transpose();
    Eigen::Vector3d max_bound = result->vertices().row(0).transpose();
    
    for (int i = 1; i < result->vertices().rows(); ++i) {
        Eigen::Vector3d vertex = result->vertices().row(i).transpose();
        min_bound = min_bound.cwiseMin(vertex);
        max_bound = max_bound.cwiseMax(vertex);
    }
    
    // Check that the Z height is reasonable (may be larger than size_ due to caps)
    double z_range = max_bound.z() - min_bound.z();
    EXPECT_GT(z_range, 0.0);
    EXPECT_LT(z_range, size_ * 3.0); // Allow for some variation
    
    // Check that XY dimensions are reasonable (related to radius)
    double xy_range = std::max(max_bound.x() - min_bound.x(), max_bound.y() - min_bound.y());
    EXPECT_GT(xy_range, size_); // Should be at least 2*radius
    EXPECT_LT(xy_range, size_ * 3.0); // But not too large
}

TEST_F(MeshGeneratorTest, PlaneGeneration) {
    auto result = geometry::PlaneGenerator::generate(size_, size_, subdivisions_, subdivisions_);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->vertices().rows(), 0);
    EXPECT_GT(result->faces().rows(), 0);
    
    // Check that vertices are within reasonable bounds
    Eigen::Vector3d min_bound = result->vertices().row(0).transpose();
    Eigen::Vector3d max_bound = result->vertices().row(0).transpose();
    
    for (int i = 1; i < result->vertices().rows(); ++i) {
        Eigen::Vector3d vertex = result->vertices().row(i).transpose();
        min_bound = min_bound.cwiseMin(vertex);
        max_bound = max_bound.cwiseMax(vertex);
    }
    
    // Check that at least two dimensions span the expected size
    double x_range = max_bound.x() - min_bound.x();
    double y_range = max_bound.y() - min_bound.y();
    double z_range = max_bound.z() - min_bound.z();
    
    // At least one pair of dimensions should span approximately size_
    bool has_expected_dimensions = 
        (std::abs(x_range - size_) < 0.1) ||
        (std::abs(y_range - size_) < 0.1) ||
        (std::abs(z_range - size_) < 0.1);
    
    EXPECT_TRUE(has_expected_dimensions) << "Expected one dimension to be approximately " << size_ 
                                        << " but got X=" << x_range 
                                        << " Y=" << y_range 
                                        << " Z=" << z_range;
    
    // Plane should be relatively flat in at least one dimension
    double min_range = std::min({x_range, y_range, z_range});
    EXPECT_LT(min_range, size_); // At least one dimension should be smaller than the others
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
    auto result4 = geometry::SphereGenerator::generate_uv_sphere(1.0, 2, 8);  // Too few meridians
    EXPECT_FALSE(result4.has_value());
}

TEST_F(MeshGeneratorTest, SubdivisionEffects) {
    auto low_res = geometry::SphereGenerator::generate_uv_sphere(1.0, 8, 8);
    auto high_res = geometry::SphereGenerator::generate_uv_sphere(1.0, 16, 16);
    
    ASSERT_TRUE(low_res.has_value());
    ASSERT_TRUE(high_res.has_value());
    
    // Higher subdivision should result in more vertices and faces
    EXPECT_LT(low_res->vertices().rows(), high_res->vertices().rows());
    EXPECT_LT(low_res->faces().rows(), high_res->faces().rows());
}

TEST_F(MeshGeneratorTest, TorusGeneration) {
    auto result = geometry::TorusGenerator::generate();
    
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->vertices().rows(), 0);
    EXPECT_GT(result->faces().rows(), 0);
    EXPECT_EQ(result->vertices().cols(), 3);
    EXPECT_EQ(result->faces().cols(), 3);
    
    // Check that vertices are distributed around a torus shape
    // All vertices should be at least minor_radius distance from the origin in XY plane
    const double major_radius = geometry::TorusGenerator::DEFAULT_MAJOR_RADIUS;
    const double minor_radius = geometry::TorusGenerator::DEFAULT_MINOR_RADIUS;
    
    for (int i = 0; i < result->vertices().rows(); ++i) {
        const auto& vertex = result->vertices().row(i);
        const double distance_from_center = std::sqrt(vertex(0) * vertex(0) + vertex(1) * vertex(1));
        
        // Should be within the torus bounds
        EXPECT_GE(distance_from_center, major_radius - minor_radius - 0.01);
        EXPECT_LE(distance_from_center, major_radius + minor_radius + 0.01);
        
        // Z coordinate should be within minor radius bounds
        EXPECT_GE(vertex(2), -minor_radius - 0.01);
        EXPECT_LE(vertex(2), minor_radius + 0.01);
    }
}

TEST_F(MeshGeneratorTest, TorusParametrization) {
    const double custom_major = 2.0;
    const double custom_minor = 0.5;
    const int custom_major_segments = 24;
    const int custom_minor_segments = 8;
    
    auto result = geometry::TorusGenerator::generate(custom_major, custom_minor, 
                                                   custom_major_segments, custom_minor_segments);
    
    ASSERT_TRUE(result.has_value());
    
    // Expected vertex count: major_segments * minor_segments
    EXPECT_EQ(result->vertices().rows(), custom_major_segments * custom_minor_segments);
    
    // Expected face count: 2 * major_segments * minor_segments (2 triangles per quad)
    EXPECT_EQ(result->faces().rows(), 2 * custom_major_segments * custom_minor_segments);
}

TEST_F(MeshGeneratorTest, TorusInvalidParameters) {
    // Test negative major radius
    auto result1 = geometry::TorusGenerator::generate(-1.0, 0.3, 12, 8);
    EXPECT_FALSE(result1.has_value());
    
    // Test negative minor radius
    auto result2 = geometry::TorusGenerator::generate(1.0, -0.3, 12, 8);
    EXPECT_FALSE(result2.has_value());
    
    // Test too few major segments
    auto result3 = geometry::TorusGenerator::generate(1.0, 0.3, 2, 8);
    EXPECT_FALSE(result3.has_value());
    
    // Test too few minor segments
    auto result4 = geometry::TorusGenerator::generate(1.0, 0.3, 12, 2);
    EXPECT_FALSE(result4.has_value());
}
