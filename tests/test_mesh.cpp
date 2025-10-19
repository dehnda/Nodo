#include <gtest/gtest.h>
#include <nodeflux/core/mesh.hpp>
#include <Eigen/Dense>

using namespace nodeflux;

class MeshTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple triangle mesh for testing
        Eigen::MatrixXd vertices(3, 3);
        vertices << 
            0.0, 0.0, 0.0,
            1.0, 0.0, 0.0,
            0.5, 1.0, 0.0;
        
        Eigen::MatrixXi faces(1, 3);
        faces << 0, 1, 2;
        
        triangle_mesh_.vertices() = vertices;
        triangle_mesh_.faces() = faces;
    }
    
    core::Mesh triangle_mesh_;
};

TEST_F(MeshTest, ConstructorCreatesEmptyMesh) {
    core::Mesh empty_mesh;
    EXPECT_EQ(empty_mesh.vertices().rows(), 0);
    EXPECT_EQ(empty_mesh.faces().rows(), 0);
}

TEST_F(MeshTest, VerticesAssignment) {
    EXPECT_EQ(triangle_mesh_.vertices().rows(), 3);
    EXPECT_EQ(triangle_mesh_.vertices().cols(), 3);
    
    // Check first vertex
    EXPECT_DOUBLE_EQ(triangle_mesh_.vertices()(0, 0), 0.0);
    EXPECT_DOUBLE_EQ(triangle_mesh_.vertices()(0, 1), 0.0);
    EXPECT_DOUBLE_EQ(triangle_mesh_.vertices()(0, 2), 0.0);
    
    // Check second vertex
    EXPECT_DOUBLE_EQ(triangle_mesh_.vertices()(1, 0), 1.0);
    EXPECT_DOUBLE_EQ(triangle_mesh_.vertices()(1, 1), 0.0);
    EXPECT_DOUBLE_EQ(triangle_mesh_.vertices()(1, 2), 0.0);
}

TEST_F(MeshTest, FacesAssignment) {
    EXPECT_EQ(triangle_mesh_.faces().rows(), 1);
    EXPECT_EQ(triangle_mesh_.faces().cols(), 3);
    
    // Check face indices
    EXPECT_EQ(triangle_mesh_.faces()(0, 0), 0);
    EXPECT_EQ(triangle_mesh_.faces()(0, 1), 1);
    EXPECT_EQ(triangle_mesh_.faces()(0, 2), 2);
}

TEST_F(MeshTest, MeshModification) {
    // Test modifying vertices
    triangle_mesh_.vertices()(0, 0) = 2.0;
    EXPECT_DOUBLE_EQ(triangle_mesh_.vertices()(0, 0), 2.0);
    
    // Test modifying faces
    triangle_mesh_.faces()(0, 0) = 1;
    EXPECT_EQ(triangle_mesh_.faces()(0, 0), 1);
}

TEST_F(MeshTest, MeshCopy) {
    core::Mesh copied_mesh = triangle_mesh_;
    
    EXPECT_EQ(copied_mesh.vertices().rows(), triangle_mesh_.vertices().rows());
    EXPECT_EQ(copied_mesh.faces().rows(), triangle_mesh_.faces().rows());
    
    // Verify the data is the same
    for (int i = 0; i < triangle_mesh_.vertices().rows(); ++i) {
        for (int j = 0; j < triangle_mesh_.vertices().cols(); ++j) {
            EXPECT_DOUBLE_EQ(copied_mesh.vertices()(i, j), triangle_mesh_.vertices()(i, j));
        }
    }
    
    for (int i = 0; i < triangle_mesh_.faces().rows(); ++i) {
        for (int j = 0; j < triangle_mesh_.faces().cols(); ++j) {
            EXPECT_EQ(copied_mesh.faces()(i, j), triangle_mesh_.faces()(i, j));
        }
    }
}

TEST_F(MeshTest, MeshMove) {
    core::Mesh original_mesh = triangle_mesh_;
    core::Mesh moved_mesh = std::move(triangle_mesh_);
    
    // Original mesh should be intact in moved_mesh
    EXPECT_EQ(moved_mesh.vertices().rows(), 3);
    EXPECT_EQ(moved_mesh.faces().rows(), 1);
    
    // Verify the data matches original
    for (int i = 0; i < original_mesh.vertices().rows(); ++i) {
        for (int j = 0; j < original_mesh.vertices().cols(); ++j) {
            EXPECT_DOUBLE_EQ(moved_mesh.vertices()(i, j), original_mesh.vertices()(i, j));
        }
    }
}

TEST_F(MeshTest, LargeMeshHandling) {
    // Test with a larger mesh
    const int num_vertices = 1000;
    const int num_faces = 500;
    
    Eigen::MatrixXd large_vertices = Eigen::MatrixXd::Random(num_vertices, 3);
    Eigen::MatrixXi large_faces = Eigen::MatrixXi::Random(num_faces, 3);
    
    // Ensure face indices are valid
    large_faces = large_faces.cwiseAbs();
    for (int i = 0; i < num_faces; ++i) {
        for (int j = 0; j < 3; ++j) {
            large_faces(i, j) = large_faces(i, j) % num_vertices;
        }
    }
    
    core::Mesh large_mesh;
    large_mesh.vertices() = large_vertices;
    large_mesh.faces() = large_faces;
    
    EXPECT_EQ(large_mesh.vertices().rows(), num_vertices);
    EXPECT_EQ(large_mesh.faces().rows(), num_faces);
}
