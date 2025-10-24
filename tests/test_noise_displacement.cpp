#include <gtest/gtest.h>
#include <nodeflux/geometry/sphere_generator.hpp>
#include <nodeflux/graph/execution_engine.hpp>
#include <nodeflux/graph/node_graph.hpp>

using namespace nodeflux;

class NoiseDisplacementTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Generate a test sphere
    auto sphere_result =
        geometry::SphereGenerator::generate_uv_sphere(1.0, 16, 8);
    ASSERT_TRUE(sphere_result.has_value());
    test_sphere_ = std::make_shared<core::Mesh>(*sphere_result);
  }

  std::shared_ptr<core::Mesh> test_sphere_;
};

TEST_F(NoiseDisplacementTest, GraphExecutionIntegration) {
  // Create a graph with sphere -> noise displacement
  graph::NodeGraph node_graph;

  // Add sphere node
  int sphere_id = node_graph.add_node(graph::NodeType::Sphere);

  // Add noise displacement node
  int noise_id = node_graph.add_node(graph::NodeType::NoiseDisplacement);

  // Connect sphere to noise displacement
  node_graph.add_connection(sphere_id, noise_id, 0, 0);

  // Set display node
  node_graph.set_display_node(noise_id);

  // Execute graph
  graph::ExecutionEngine engine;
  bool success = engine.execute_graph(node_graph);

  EXPECT_TRUE(success) << "Graph execution should succeed";

  // Get result
  auto result = engine.get_node_result(noise_id);
  ASSERT_NE(result, nullptr) << "Noise displacement node should produce output";
  EXPECT_GT(result->vertices().rows(), 0) << "Result should have vertices";
}
