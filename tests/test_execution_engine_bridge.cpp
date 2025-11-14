#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/graph/execution_engine.hpp"
#include "nodo/graph/node_graph.hpp"

#include <gtest/gtest.h>

using namespace nodo;
using namespace nodo::graph;
using namespace nodo::core;

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

  // Get result (GeometryContainer)
  auto result = engine->get_node_geometry(transform_id);

  // Verify result
  ASSERT_NE(result, nullptr);
  EXPECT_GT(result->point_count(), 0);

  // Check that geometry has points
  EXPECT_GT(result->point_count(), 0);

  // Check that primitives are valid
  EXPECT_GT(result->primitive_count(), 0);
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
  auto result = engine->get_node_geometry(sphere_id);

  // Verify result
  ASSERT_NE(result, nullptr);
  EXPECT_GT(result->point_count(), 0);

  // Basic sanity checks
  EXPECT_GT(result->point_count(), 0);
  EXPECT_GT(result->primitive_count(), 0);

  // Verify P attribute exists
  auto* positions =
      result->get_point_attribute_typed<Eigen::Vector3f>(standard_attrs::P);
  ASSERT_NE(positions, nullptr);
  EXPECT_EQ(positions->size(), result->point_count());

  // Verify all primitives reference valid vertices
  const auto& topo = result->topology();
  for (size_t i = 0; i < result->primitive_count(); ++i) {
    const auto& prim_verts = topo.get_primitive_vertices(static_cast<int>(i));
    for (int vert_idx : prim_verts) {
      EXPECT_GE(vert_idx, 0);
      EXPECT_LT(static_cast<size_t>(vert_idx), result->vertex_count());
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

  auto result = engine->get_node_geometry(mirror_id);

  ASSERT_NE(result, nullptr);
  EXPECT_GT(result->point_count(), 0);

  // Mirrored sphere should have points
  EXPECT_GT(result->point_count(), 0);

  // Should have primitives
  EXPECT_GT(result->primitive_count(), 0);
}

// Test that caching works correctly across Mesh<->Container conversions
TEST_F(ExecutionEngineBridgeTest, CachingAcrossBridgeConversions) {
  int sphere_id = graph->add_node(NodeType::Sphere, "Sphere");
  int transform_id = graph->add_node(NodeType::Transform, "Transform");

  graph->add_connection(sphere_id, 0, transform_id, 0);
  graph->set_display_node(transform_id);

  // Execute twice - second execution should use cache
  bool success1 = engine->execute_graph(*graph);
  auto result1 = engine->get_node_geometry(transform_id);

  bool success2 = engine->execute_graph(*graph);
  auto result2 = engine->get_node_geometry(transform_id);

  EXPECT_TRUE(success1);
  EXPECT_TRUE(success2);
  ASSERT_NE(result1, nullptr);
  ASSERT_NE(result2, nullptr);

  // Results should have same geometry (both from cache)
  EXPECT_EQ(result1->point_count(), result2->point_count());
  EXPECT_EQ(result1->primitive_count(), result2->primitive_count());

  // Cache should return same GeometryContainer pointer
  EXPECT_EQ(result1.get(), result2.get())
      << "Cached results should be identical pointers";
}
