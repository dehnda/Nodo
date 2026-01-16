#include <gtest/gtest.h>
#include <nodo/geometry/sphere_generator.hpp>
#include <nodo/graph/execution_engine.hpp>
#include <nodo/graph/node_graph.hpp>

using namespace nodo;

class NoiseDisplacementTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Test setup if needed
  }
};

TEST_F(NoiseDisplacementTest, GraphExecutionIntegration) {
  // Create a graph with sphere -> noise displacement
  graph::NodeGraph node_graph;

  // Add sphere node
  int sphere_id = node_graph.add_node(graph::NodeType::Sphere);

  // Add noise displacement node
  int noise_id = node_graph.add_node(graph::NodeType::NoiseDisplacement);

  // Connect sphere to noise displacement (source_node, source_pin, target_node,
  // target_pin)
  node_graph.add_connection(sphere_id, 0, noise_id, 0);

  // Set display node
  node_graph.set_display_node(noise_id);

  // Execute graph
  graph::ExecutionEngine engine;
  bool success = engine.execute_graph(node_graph);

  EXPECT_TRUE(success) << "Graph execution should succeed";

  // Get result
  auto result = engine.get_node_geometry(noise_id);
  ASSERT_NE(result, nullptr) << "Noise displacement node should produce output";
  EXPECT_GT(result->point_count(), 0) << "Result should have points";
}
