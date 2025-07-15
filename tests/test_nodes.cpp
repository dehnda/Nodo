#include <gtest/gtest.h>
#include <nodeflux/nodes/box_node.hpp>
#include <nodeflux/nodes/sphere_node.hpp>
#include <nodeflux/nodes/cylinder_node.hpp>
#include <nodeflux/nodes/plane_node.hpp>

using namespace nodeflux;

class NodesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Basic setup for node testing
    }
};

// Box Node Tests
TEST_F(NodesTest, BoxNodeCreation) {
    nodes::BoxNode box_node(2.0, 1.5, 0.8);
    
    auto mesh_result = box_node.generate();
    ASSERT_TRUE(mesh_result.has_value());
    EXPECT_GT(mesh_result->vertices().rows(), 0);
    EXPECT_GT(mesh_result->faces().rows(), 0);
}

TEST_F(NodesTest, BoxNodeParameterModification) {
    nodes::BoxNode box_node(1.0, 1.0, 1.0);
    
    box_node.set_width(3.0);
    box_node.set_height(2.0);
    box_node.set_depth(1.5);
    
    auto mesh_result = box_node.generate();
    ASSERT_TRUE(mesh_result.has_value());
}

// Sphere Node Tests
TEST_F(NodesTest, SphereNodeCreation) {
    nodes::SphereNode sphere_node(1.5, 16, 8);
    
    auto mesh_result = sphere_node.generate();
    ASSERT_TRUE(mesh_result.has_value());
    EXPECT_GT(mesh_result->vertices().rows(), 0);
    EXPECT_GT(mesh_result->faces().rows(), 0);
}

// Cylinder Node Tests
TEST_F(NodesTest, CylinderNodeCreation) {
    nodes::CylinderNode cylinder_node(0.5, 2.0, 12);
    
    auto mesh_result = cylinder_node.generate();
    ASSERT_TRUE(mesh_result.has_value());
    EXPECT_GT(mesh_result->vertices().rows(), 0);
    EXPECT_GT(mesh_result->faces().rows(), 0);
}

// Plane Node Tests
TEST_F(NodesTest, PlaneNodeCreation) {
    nodes::PlaneNode plane_node(2.0, 1.5, 4, 3);
    
    auto mesh_result = plane_node.generate();
    ASSERT_TRUE(mesh_result.has_value());
    EXPECT_GT(mesh_result->vertices().rows(), 0);
    EXPECT_GT(mesh_result->faces().rows(), 0);
}
