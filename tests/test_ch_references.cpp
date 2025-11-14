/**
 * Unit tests for ch() parameter references between nodes
 * Tests cross-node parameter resolution with unique node names
 */

#include "nodo/graph/node_graph.hpp"
#include "nodo/graph/parameter_expression_resolver.hpp"

#include <gtest/gtest.h>

using namespace nodo::graph;

class CHReferencesTest : public ::testing::Test {
protected:
  void SetUp() override { graph_ = std::make_unique<NodeGraph>(); }

  std::unique_ptr<NodeGraph> graph_;
};

// Test basic ch() resolution with absolute path
TEST_F(CHReferencesTest, BasicAbsolutePathReference) {
  // Create a sphere node with a radius parameter
  int sphere_id = graph_->add_node(NodeType::Sphere, "sphere");
  auto* sphere = graph_->get_node(sphere_id);
  ASSERT_NE(sphere, nullptr);
  sphere->add_parameter(NodeParameter("radius", 2.5F));

  // Create a box node that references the sphere's radius
  int box_id = graph_->add_node(NodeType::Box, "box");

  // Test path resolution (without ch() wrapper)
  auto result = graph_->resolve_parameter_path(box_id, "/sphere/radius");

  ASSERT_TRUE(result.has_value());
  // The result is a string representation of the value
  EXPECT_EQ(result.value(), "2.500000"); // std::to_string adds decimals
}

// Test ch() with unique node names (sphere, sphere1, sphere2)
TEST_F(CHReferencesTest, UniqueNodeNames) {
  // Create multiple sphere nodes - should get unique names
  int sphere1_id = graph_->add_node(NodeType::Sphere, "sphere");
  auto* sphere1 = graph_->get_node(sphere1_id);
  ASSERT_NE(sphere1, nullptr);
  sphere1->add_parameter(NodeParameter("radius", 1.0F));

  int sphere2_id = graph_->add_node(NodeType::Sphere, "sphere");
  auto* sphere2 = graph_->get_node(sphere2_id);
  ASSERT_NE(sphere2, nullptr);
  sphere2->add_parameter(NodeParameter("radius", 2.0F));

  int sphere3_id = graph_->add_node(NodeType::Sphere, "sphere");
  auto* sphere3 = graph_->get_node(sphere3_id);
  ASSERT_NE(sphere3, nullptr);
  sphere3->add_parameter(NodeParameter("radius", 3.0F));

  // Verify unique names were generated
  EXPECT_EQ(sphere1->get_name(), "sphere");
  EXPECT_EQ(sphere2->get_name(), "sphere1");
  EXPECT_EQ(sphere3->get_name(), "sphere2");

  // Create a box node to test references
  int box_id = graph_->add_node(NodeType::Box, "box");

  // Test ch() references to each sphere
  auto result1 = graph_->resolve_parameter_path(box_id, "/sphere/radius");
  ASSERT_TRUE(result1.has_value());
  EXPECT_EQ(result1.value(), "1.000000");

  auto result2 = graph_->resolve_parameter_path(box_id, "/sphere1/radius");
  ASSERT_TRUE(result2.has_value());
  EXPECT_EQ(result2.value(), "2.000000");

  auto result3 = graph_->resolve_parameter_path(box_id, "/sphere2/radius");
  ASSERT_TRUE(result3.has_value());
  EXPECT_EQ(result3.value(), "3.000000");
}

// Test ch() in math expressions (uses ParameterExpressionResolver)
TEST_F(CHReferencesTest, CHInMathExpression) {
  int sphere_id = graph_->add_node(NodeType::Sphere, "sphere");
  auto* sphere = graph_->get_node(sphere_id);
  ASSERT_NE(sphere, nullptr);
  sphere->add_parameter(NodeParameter("radius", 5.0F));

  int box_id = graph_->add_node(NodeType::Box, "box");

  // Create expression resolver with box as current node
  nodo::graph::ParameterExpressionResolver resolver(*graph_, nullptr, box_id);

  // Test ch() * 2 - resolver handles ch() and math
  auto result1 = resolver.resolve_float("ch(\"/sphere/radius\") * 2");
  ASSERT_TRUE(result1.has_value());
  EXPECT_FLOAT_EQ(result1.value(), 10.0F);

  // Test ch() + 3
  auto result2 = resolver.resolve_float("ch(\"/sphere/radius\") + 3");
  ASSERT_TRUE(result2.has_value());
  EXPECT_FLOAT_EQ(result2.value(), 8.0F);

  // Test multiple ch() references
  int sphere2_id = graph_->add_node(NodeType::Sphere, "sphere1");
  auto* sphere2 = graph_->get_node(sphere2_id);
  ASSERT_NE(sphere2, nullptr);
  sphere2->add_parameter(NodeParameter("radius", 3.0F));

  auto result3 = resolver.resolve_float(
      "ch(\"/sphere/radius\") + ch(\"/sphere1/radius\")");
  ASSERT_TRUE(result3.has_value());
  EXPECT_FLOAT_EQ(result3.value(), 8.0F); // 5 + 3
}

// Test ch() with integer parameters
TEST_F(CHReferencesTest, IntegerParameters) {
  int sphere_id = graph_->add_node(NodeType::Sphere, "sphere");
  auto* sphere = graph_->get_node(sphere_id);
  ASSERT_NE(sphere, nullptr);
  sphere->add_parameter(NodeParameter("u_segments", 32));

  int box_id = graph_->add_node(NodeType::Box, "box");

  auto result = graph_->resolve_parameter_path(box_id, "/sphere/u_segments");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "32");
}

// Test error: node not found
TEST_F(CHReferencesTest, NodeNotFound) {
  int box_id = graph_->add_node(NodeType::Box, "box");

  auto result = graph_->resolve_parameter_path(box_id, "/nonexistent/radius");
  EXPECT_FALSE(result.has_value());
}

// Test error: parameter not found
TEST_F(CHReferencesTest, ParameterNotFound) {
  int sphere_id = graph_->add_node(NodeType::Sphere, "sphere");
  auto* sphere = graph_->get_node(sphere_id);
  ASSERT_NE(sphere, nullptr);
  sphere->add_parameter(NodeParameter("radius", 1.0F));

  int box_id = graph_->add_node(NodeType::Box, "box");

  auto result =
      graph_->resolve_parameter_path(box_id, "/sphere/nonexistent_param");
  EXPECT_FALSE(result.has_value());
}

// Test ch() with different node types
TEST_F(CHReferencesTest, DifferentNodeTypes) {
  int box_source_id = graph_->add_node(NodeType::Box, "box_source");
  auto* box_source = graph_->get_node(box_source_id);
  ASSERT_NE(box_source, nullptr);
  box_source->add_parameter(NodeParameter("width", 4.0F));
  box_source->add_parameter(NodeParameter("height", 3.0F));
  box_source->add_parameter(NodeParameter("depth", 2.0F));

  int sphere_id = graph_->add_node(NodeType::Sphere, "sphere");

  auto result1 = graph_->resolve_parameter_path(sphere_id, "/box_source/width");
  ASSERT_TRUE(result1.has_value());
  EXPECT_EQ(result1.value(), "4.000000");

  auto result2 =
      graph_->resolve_parameter_path(sphere_id, "/box_source/height");
  ASSERT_TRUE(result2.has_value());
  EXPECT_EQ(result2.value(), "3.000000");
}

// Test ch() with nested expressions
TEST_F(CHReferencesTest, NestedExpressions) {
  int sphere_id = graph_->add_node(NodeType::Sphere, "sphere");
  auto* sphere = graph_->get_node(sphere_id);
  ASSERT_NE(sphere, nullptr);
  sphere->add_parameter(NodeParameter("radius", 5.0F));
  sphere->add_parameter(NodeParameter("u_segments", 16));

  int box_id = graph_->add_node(NodeType::Box, "box");

  // Create expression resolver
  ParameterExpressionResolver resolver(*graph_, nullptr, box_id);

  // Complex expression: (radius * 2) + (u_segments / 4)
  auto result = resolver.resolve_float(
      "(ch(\"/sphere/radius\") * 2) + (ch(\"/sphere/u_segments\") / 4)");
  ASSERT_TRUE(result.has_value());
  // (5.0 * 2) + (16 / 4) = 10.0 + 4.0 = 14.0
  EXPECT_FLOAT_EQ(result.value(), 14.0F);
}

// Test ch() with single quotes
TEST_F(CHReferencesTest, SingleQuotes) {
  int sphere_id = graph_->add_node(NodeType::Sphere, "sphere");
  auto* sphere = graph_->get_node(sphere_id);
  ASSERT_NE(sphere, nullptr);
  sphere->add_parameter(NodeParameter("radius", 7.5F));

  int box_id = graph_->add_node(NodeType::Box, "box");

  auto result = graph_->resolve_parameter_path(box_id, "/sphere/radius");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "7.500000");
}

// Test updating referenced parameter
TEST_F(CHReferencesTest, UpdateReferencedParameter) {
  int sphere_id = graph_->add_node(NodeType::Sphere, "sphere");
  auto* sphere = graph_->get_node(sphere_id);
  ASSERT_NE(sphere, nullptr);
  sphere->add_parameter(NodeParameter("radius", 1.0F));

  int box_id = graph_->add_node(NodeType::Box, "box");

  // First resolution
  auto result1 = graph_->resolve_parameter_path(box_id, "/sphere/radius");
  ASSERT_TRUE(result1.has_value());
  EXPECT_EQ(result1.value(), "1.000000");

  // Update sphere radius
  auto radius_param = sphere->get_parameter("radius");
  ASSERT_TRUE(radius_param.has_value());
  auto updated_param = radius_param.value();
  updated_param.float_value = 5.0F;
  sphere->set_parameter("radius", updated_param);

  // Second resolution should reflect the update
  auto result2 = graph_->resolve_parameter_path(box_id, "/sphere/radius");
  ASSERT_TRUE(result2.has_value());
  EXPECT_EQ(result2.value(), "5.000000");
}

// Test ch() with zero values
TEST_F(CHReferencesTest, ZeroValues) {
  int sphere_id = graph_->add_node(NodeType::Sphere, "sphere");
  auto* sphere = graph_->get_node(sphere_id);
  ASSERT_NE(sphere, nullptr);
  sphere->add_parameter(NodeParameter("radius", 0.0F));

  int box_id = graph_->add_node(NodeType::Box, "box");

  auto result = graph_->resolve_parameter_path(box_id, "/sphere/radius");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "0.000000");
}

// Test ch() with negative values
TEST_F(CHReferencesTest, NegativeValues) {
  int sphere_id = graph_->add_node(NodeType::Sphere, "sphere");
  auto* sphere = graph_->get_node(sphere_id);
  ASSERT_NE(sphere, nullptr);
  sphere->add_parameter(NodeParameter("radius", -3.5F));

  int box_id = graph_->add_node(NodeType::Box, "box");

  auto result = graph_->resolve_parameter_path(box_id, "/sphere/radius");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "-3.500000");
}

// Test ch() with very large values
TEST_F(CHReferencesTest, LargeValues) {
  int sphere_id = graph_->add_node(NodeType::Sphere, "sphere");
  auto* sphere = graph_->get_node(sphere_id);
  ASSERT_NE(sphere, nullptr);
  sphere->add_parameter(NodeParameter("u_segments", 1000));

  int box_id = graph_->add_node(NodeType::Box, "box");

  auto result = graph_->resolve_parameter_path(box_id, "/sphere/u_segments");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "1000");
}
