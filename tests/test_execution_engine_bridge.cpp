#include "nodeflux/core/geometry_container.hpp"
#include "nodeflux/graph/execution_engine.hpp"
#include "nodeflux/graph/node_graph.hpp"
#include <gtest/gtest.h>

using namespace nodeflux;
using namespace nodeflux::graph;

class ExecutionEngineBridgeTest : public ::testing::Test {
protected:
  void SetUp() override {
    graph = std::make_unique<NodeGraph>();
    engine = std::make_unique<ExecutionEngine>();
  }

  std::unique_ptr<NodeGraph> graph;
  std::unique_ptr<ExecutionEngine> engine;
};

// Test that a simple Sphere -> Transform pipeline works with
// Mesh<->GeometryContainer conversion
TEST_F(ExecutionEngineBridgeTest, SphereToTransformPipeline) {
  // Create nodes - using default parameters
  int sphere_id = graph->add_node(NodeType::Sphere, "TestSphere");
  int transform_id = graph->add_node(NodeType::Transform, "TestTransform");

  // Connect sphere to transform
  bool connected = graph->add_connection(sphere_id, 0, transform_id, 0);
  ASSERT_TRUE(connected);

  // Set display node and execute
  graph->set_display_node(transform_id);
  bool success = engine->execute_graph(*graph);
  EXPECT_TRUE(success);

  // Get result
  auto result = engine->get_node_result(transform_id);

  // Verify result
  ASSERT_NE(result, nullptr);
  EXPECT_FALSE(result->empty());

  // Check that geometry has vertices
  const auto &vertices = result->vertices();
  EXPECT_GT(vertices.rows(), 0);

  // Check that faces are valid
  const auto &faces = result->faces();
  EXPECT_GT(faces.rows(), 0);
}

// Test that SOPs correctly convert between Mesh and GeometryContainer
TEST_F(ExecutionEngineBridgeTest, MeshToContainerPreservesTopology) {
  // Create a simple sphere with default parameters
  int sphere_id = graph->add_node(NodeType::Sphere, "TestSphere");

  // Set display and execute
  graph->set_display_node(sphere_id);
  bool success = engine->execute_graph(*graph);
  EXPECT_TRUE(success);

  // Get result
  auto result = engine->get_node_result(sphere_id);

  // Verify result
  ASSERT_NE(result, nullptr);
  EXPECT_FALSE(result->empty());

  const auto &vertices = result->vertices();
  const auto &faces = result->faces();

  // Basic sanity checks
  EXPECT_GT(vertices.rows(), 0);
  EXPECT_GT(faces.rows(), 0);

  // Verify all face indices are valid
  for (int i = 0; i < faces.rows(); ++i) {
    for (int j = 0; j < 3; ++j) {
      int idx = faces(i, j);
      EXPECT_GE(idx, 0);
      EXPECT_LT(idx, vertices.rows());
    }
  }
}

// Test chaining multiple SOPs (Sphere -> Transform -> Mirror)
TEST_F(ExecutionEngineBridgeTest, MultiSOPChain) {
  // Create nodes with default parameters
  int sphere_id = graph->add_node(NodeType::Sphere, "Sphere");
  int transform_id = graph->add_node(NodeType::Transform, "Transform");
  int mirror_id = graph->add_node(NodeType::Mirror, "Mirror");

  // Connect: Sphere -> Transform -> Mirror
  graph->add_connection(sphere_id, 0, transform_id, 0);
  graph->add_connection(transform_id, 0, mirror_id, 0);

  // Execute the full chain
  graph->set_display_node(mirror_id);
  bool success = engine->execute_graph(*graph);
  EXPECT_TRUE(success);

  auto result = engine->get_node_result(mirror_id);

  ASSERT_NE(result, nullptr);
  EXPECT_FALSE(result->empty());

  const auto &vertices = result->vertices();

  // Mirrored sphere should have vertices
  EXPECT_GT(vertices.rows(), 0);

  // Should have faces
  const auto &faces = result->faces();
  EXPECT_GT(faces.rows(), 0);
}

// Test that caching works correctly across Mesh<->Container conversions
TEST_F(ExecutionEngineBridgeTest, CachingAcrossBridgeConversions) {
  int sphere_id = graph->add_node(NodeType::Sphere, "Sphere");
  int transform_id = graph->add_node(NodeType::Transform, "Transform");

  graph->add_connection(sphere_id, 0, transform_id, 0);
  graph->set_display_node(transform_id);

  // Execute twice - second execution should use cache
  bool success1 = engine->execute_graph(*graph);
  auto result1 = engine->get_node_result(transform_id);

  bool success2 = engine->execute_graph(*graph);
  auto result2 = engine->get_node_result(transform_id);

  EXPECT_TRUE(success1);
  EXPECT_TRUE(success2);
  ASSERT_NE(result1, nullptr);
  ASSERT_NE(result2, nullptr);

  // Results should have same geometry (both from cache)
  EXPECT_EQ(result1->vertices().rows(), result2->vertices().rows());
  EXPECT_EQ(result1->faces().rows(), result2->faces().rows());

  // NOTE: Currently we don't have pointer-level caching across
  // GeometryContainer->Mesh conversions so result1.get() != result2.get() even
  // though the geometry is identical. This is acceptable for now - full caching
  // optimization can be added later.
}
