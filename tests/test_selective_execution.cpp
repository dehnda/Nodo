#include "nodo/graph/execution_engine.hpp"
#include "nodo/graph/node_graph.hpp"
#include <gtest/gtest.h>

using namespace nodo::graph;

class SelectiveExecutionTest : public ::testing::Test {
protected:
  void SetUp() override {
    graph = std::make_unique<NodeGraph>();
    engine = std::make_unique<ExecutionEngine>();
  }

  std::unique_ptr<NodeGraph> graph;
  std::unique_ptr<ExecutionEngine> engine;
};

TEST_F(SelectiveExecutionTest, UpstreamDependenciesLinearChain) {
  // Create a linear chain: Sphere -> Transform -> Array
  int sphere_id = graph->add_node(NodeType::Sphere, "Sphere");
  int transform_id = graph->add_node(NodeType::Transform, "Transform");
  int array_id = graph->add_node(NodeType::Array, "Array");

  graph->add_connection(sphere_id, 0, transform_id, 0);
  graph->add_connection(transform_id, 0, array_id, 0);

  // Get upstream dependencies of Array
  auto deps = graph->get_upstream_dependencies(array_id);

  // Should return: Sphere, Transform, Array (in order)
  ASSERT_EQ(deps.size(), 3);
  EXPECT_EQ(deps[0], sphere_id);
  EXPECT_EQ(deps[1], transform_id);
  EXPECT_EQ(deps[2], array_id);
}

TEST_F(SelectiveExecutionTest, UpstreamDependenciesBranching) {
  // Create branching graph:
  //   Sphere1 ─┐
  //            ├─> Boolean -> Transform
  //   Sphere2 ─┘
  int sphere1_id = graph->add_node(NodeType::Sphere, "Sphere1");
  int sphere2_id = graph->add_node(NodeType::Sphere, "Sphere2");
  int boolean_id = graph->add_node(NodeType::Boolean, "Boolean");
  int transform_id = graph->add_node(NodeType::Transform, "Transform");

  graph->add_connection(sphere1_id, 0, boolean_id, 0);
  graph->add_connection(sphere2_id, 0, boolean_id, 1);
  graph->add_connection(boolean_id, 0, transform_id, 0);

  // Get upstream dependencies of Transform
  auto deps = graph->get_upstream_dependencies(transform_id);

  // Should return all 4 nodes
  ASSERT_EQ(deps.size(), 4);

  // Should include both spheres, boolean, and transform
  EXPECT_TRUE(std::find(deps.begin(), deps.end(), sphere1_id) != deps.end());
  EXPECT_TRUE(std::find(deps.begin(), deps.end(), sphere2_id) != deps.end());
  EXPECT_TRUE(std::find(deps.begin(), deps.end(), boolean_id) != deps.end());
  EXPECT_EQ(deps.back(), transform_id); // Transform should be last
}

TEST_F(SelectiveExecutionTest, UpstreamDependenciesUnconnectedNode) {
  // Create disconnected nodes:
  //   Sphere1 -> Transform1
  //   Sphere2 (unconnected)
  int sphere1_id = graph->add_node(NodeType::Sphere, "Sphere1");
  int transform_id = graph->add_node(NodeType::Transform, "Transform");
  int sphere2_id = graph->add_node(NodeType::Sphere, "Sphere2"); // Unconnected!

  graph->add_connection(sphere1_id, 0, transform_id, 0);

  // Get upstream dependencies of Transform
  auto deps = graph->get_upstream_dependencies(transform_id);

  // Should only return connected nodes (NOT Sphere2)
  ASSERT_EQ(deps.size(), 2);
  EXPECT_EQ(deps[0], sphere1_id);
  EXPECT_EQ(deps[1], transform_id);

  // Sphere2 should NOT be in the list
  EXPECT_TRUE(std::find(deps.begin(), deps.end(), sphere2_id) == deps.end());
}

TEST_F(SelectiveExecutionTest, UpstreamDependenciesComplexGraph) {
  // Create complex graph:
  //   Sphere1 -> Transform1 ─┐
  //   Sphere2 ───────────────├─> Merge -> Array
  //   Box -> Transform2 ─────┘
  //   Cylinder (unconnected)

  int sphere1_id = graph->add_node(NodeType::Sphere, "Sphere1");
  int sphere2_id = graph->add_node(NodeType::Sphere, "Sphere2");
  int box_id = graph->add_node(NodeType::Box, "Box");
  int cylinder_id = graph->add_node(NodeType::Cylinder, "Cylinder"); // Unconnected

  int transform1_id = graph->add_node(NodeType::Transform, "Transform1");
  int transform2_id = graph->add_node(NodeType::Transform, "Transform2");
  int merge_id = graph->add_node(NodeType::Merge, "Merge");
  int array_id = graph->add_node(NodeType::Array, "Array");

  // Connections
  graph->add_connection(sphere1_id, 0, transform1_id, 0);
  graph->add_connection(transform1_id, 0, merge_id, 0);
  graph->add_connection(sphere2_id, 0, merge_id, 1);
  graph->add_connection(box_id, 0, transform2_id, 0);
  graph->add_connection(transform2_id, 0, merge_id, 2);
  graph->add_connection(merge_id, 0, array_id, 0);

  // Get upstream dependencies of Array
  auto deps = graph->get_upstream_dependencies(array_id);

  // Should return 7 nodes (NOT cylinder)
  EXPECT_EQ(deps.size(), 7);

  // Should include all connected nodes
  EXPECT_TRUE(std::find(deps.begin(), deps.end(), sphere1_id) != deps.end());
  EXPECT_TRUE(std::find(deps.begin(), deps.end(), sphere2_id) != deps.end());
  EXPECT_TRUE(std::find(deps.begin(), deps.end(), box_id) != deps.end());
  EXPECT_TRUE(std::find(deps.begin(), deps.end(), transform1_id) != deps.end());
  EXPECT_TRUE(std::find(deps.begin(), deps.end(), transform2_id) != deps.end());
  EXPECT_TRUE(std::find(deps.begin(), deps.end(), merge_id) != deps.end());
  EXPECT_TRUE(std::find(deps.begin(), deps.end(), array_id) != deps.end());

  // Should NOT include cylinder
  EXPECT_TRUE(std::find(deps.begin(), deps.end(), cylinder_id) == deps.end());

  // Array should be last
  EXPECT_EQ(deps.back(), array_id);
}

TEST_F(SelectiveExecutionTest, DisplayFlagManagement) {
  // Create nodes
  int sphere_id = graph->add_node(NodeType::Sphere);
  int transform_id = graph->add_node(NodeType::Transform);
  int array_id = graph->add_node(NodeType::Array);

  // Initially no display flag
  EXPECT_EQ(graph->get_display_node(), -1);

  // Set display flag on sphere
  graph->set_display_node(sphere_id);
  EXPECT_EQ(graph->get_display_node(), sphere_id);
  EXPECT_TRUE(graph->get_node(sphere_id)->has_display_flag());
  EXPECT_FALSE(graph->get_node(transform_id)->has_display_flag());

  // Set display flag on array (should clear sphere's flag)
  graph->set_display_node(array_id);
  EXPECT_EQ(graph->get_display_node(), array_id);
  EXPECT_FALSE(graph->get_node(sphere_id)->has_display_flag());
  EXPECT_FALSE(graph->get_node(transform_id)->has_display_flag());
  EXPECT_TRUE(graph->get_node(array_id)->has_display_flag());
}

TEST_F(SelectiveExecutionTest, UpstreamDependenciesGeneratorOnly) {
  // Single generator node (no inputs)
  int sphere_id = graph->add_node(NodeType::Sphere);

  auto deps = graph->get_upstream_dependencies(sphere_id);

  // Should return only the sphere itself
  ASSERT_EQ(deps.size(), 1);
  EXPECT_EQ(deps[0], sphere_id);
}

TEST_F(SelectiveExecutionTest, UpstreamDependenciesInvalidNode) {
  // Create a node
  int sphere_id = graph->add_node(NodeType::Sphere);

  // Query dependencies of non-existent node
  auto deps = graph->get_upstream_dependencies(999);

  // Should return empty vector
  EXPECT_TRUE(deps.empty());
}
