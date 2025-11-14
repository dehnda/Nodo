#include <gtest/gtest.h>
#include <nodo/geometry/sphere_generator.hpp>
#include <nodo/graph/execution_engine.hpp>
#include <nodo/graph/node_graph.hpp>

using namespace nodo;

class NoiseDisplacementTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Generate a test sphere (now returns GeometryContainer)
    auto sphere_result =
        geometry::SphereGenerator::generate_uv_sphere(1.0, 16, 8);
    ASSERT_TRUE(sphere_result.has_value());

    // Convert GeometryContainer to Mesh for testing
    const auto& container = sphere_result.value();
    const auto& topology = container.topology();

    // Extract positions
    auto* p_storage = container.get_point_attribute_typed<core::Vec3f>(
        core::standard_attrs::P);
    ASSERT_NE(p_storage, nullptr);

    Eigen::MatrixXd vertices(topology.point_count(), 3);
    auto p_span = p_storage->values();
    for (size_t i = 0; i < p_span.size(); ++i) {
      vertices.row(i) = p_span[i].cast<double>();
    }

    // Extract faces
    Eigen::MatrixXi faces(topology.primitive_count(), 3);
    for (size_t prim_idx = 0; prim_idx < topology.primitive_count();
         ++prim_idx) {
      const auto& verts = topology.get_primitive_vertices(prim_idx);
      for (size_t j = 0; j < 3 && j < verts.size(); ++j) {
        faces(prim_idx, j) = verts[j];
      }
    }

    test_sphere_ = std::make_shared<core::Mesh>(vertices, faces);
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
