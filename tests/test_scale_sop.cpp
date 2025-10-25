#include "nodeflux/geometry/sphere_generator.hpp"
#include "nodeflux/sop/scale_sop.hpp"
#include <gtest/gtest.h>

using namespace nodeflux;

// Helper to convert GeometryContainer to Mesh
static std::shared_ptr<core::Mesh>
container_to_mesh(const core::GeometryContainer &container) {
  const auto &topology = container.topology();

  // Extract positions
  auto *p_storage =
      container.get_point_attribute_typed<core::Vec3f>(core::standard_attrs::P);
  if (!p_storage)
    return nullptr;

  Eigen::MatrixXd vertices(topology.point_count(), 3);
  auto p_span = p_storage->values();
  for (size_t i = 0; i < p_span.size(); ++i) {
    vertices.row(i) = p_span[i].cast<double>();
  }

  // Extract faces
  Eigen::MatrixXi faces(topology.primitive_count(), 3);
  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &verts = topology.get_primitive_vertices(prim_idx);
    for (size_t j = 0; j < 3 && j < verts.size(); ++j) {
      faces(prim_idx, j) = verts[j];
    }
  }

  return std::make_shared<core::Mesh>(vertices, faces);
}

class ScaleSOPTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a simple sphere for testing (now returns GeometryContainer)
    auto sphere_result =
        geometry::SphereGenerator::generate_uv_sphere(1.0, 8, 8);
    ASSERT_TRUE(sphere_result.has_value());

    // Convert to Mesh and wrap in GeometryData
    auto mesh = container_to_mesh(sphere_result.value());
    ASSERT_NE(mesh, nullptr);
    input_geometry_ = std::make_shared<sop::GeometryData>(mesh);
  }

  std::shared_ptr<sop::GeometryData> input_geometry_;
};

TEST_F(ScaleSOPTest, UniformScale) {
  auto scale_node = std::make_shared<sop::ScaleSOP>("test_scale");

  // Configure uniform scale
  scale_node->set_uniform_scale(2.0F);

  // Connect input
  auto input_port = scale_node->get_input_ports().get_port("0");
  ASSERT_NE(input_port, nullptr);
  input_port->set_data(input_geometry_);

  // Execute
  auto result = scale_node->cook();

  ASSERT_NE(result, nullptr);
  EXPECT_FALSE(result->is_empty());

  auto result_mesh = result->get_mesh();
  ASSERT_NE(result_mesh, nullptr);

  // Check that vertex count is preserved
  EXPECT_EQ(result_mesh->vertex_count(), input_geometry_->get_vertex_count());

  // Check that positions are actually scaled
  const auto &result_verts = result_mesh->vertices();
  const auto &input_verts = input_geometry_->get_mesh()->vertices();

  for (int i = 0; i < result_verts.rows(); ++i) {
    // Each vertex should be approximately 2x the original
    double dist_result = result_verts.row(i).norm();
    double dist_input = input_verts.row(i).norm();

    EXPECT_NEAR(dist_result, dist_input * 2.0, 0.01);
  }
}

TEST_F(ScaleSOPTest, NonUniformScale) {
  auto scale_node = std::make_shared<sop::ScaleSOP>("test_non_uniform");

  // Scale X by 2, Y by 3, Z by 0.5
  scale_node->set_scale(2.0F, 3.0F, 0.5F);

  auto input_port = scale_node->get_input_ports().get_port("0");
  input_port->set_data(input_geometry_);

  auto result = scale_node->cook();

  ASSERT_NE(result, nullptr);
  auto result_mesh = result->get_mesh();
  ASSERT_NE(result_mesh, nullptr);

  // Verify mesh is valid
  EXPECT_GT(result_mesh->vertex_count(), 0);
}

TEST_F(ScaleSOPTest, ScaleFromCentroid) {
  auto scale_node = std::make_shared<sop::ScaleSOP>("test_centroid");

  scale_node->set_uniform_scale(1.5F);
  scale_node->set_scale_from_origin(false); // Scale from centroid

  auto input_port = scale_node->get_input_ports().get_port("0");
  input_port->set_data(input_geometry_);

  auto result = scale_node->cook();

  ASSERT_NE(result, nullptr);
  EXPECT_FALSE(result->is_empty());
}

TEST_F(ScaleSOPTest, NoInput) {
  auto scale_node = std::make_shared<sop::ScaleSOP>("test_no_input");

  scale_node->set_uniform_scale(2.0F);

  // Don't connect input
  auto result = scale_node->cook();

  EXPECT_EQ(result, nullptr);
  EXPECT_EQ(scale_node->get_state(), sop::SOPNode::ExecutionState::ERROR);
  EXPECT_FALSE(scale_node->get_last_error().empty());
}

TEST_F(ScaleSOPTest, Caching) {
  auto scale_node = std::make_shared<sop::ScaleSOP>("test_cache");

  scale_node->set_uniform_scale(2.0F);

  auto input_port = scale_node->get_input_ports().get_port("0");
  input_port->set_data(input_geometry_);

  // First cook
  auto result1 = scale_node->cook();
  ASSERT_NE(result1, nullptr);

  // Second cook (should be cached)
  auto result2 = scale_node->cook();
  ASSERT_NE(result2, nullptr);

  // Should return same result
  EXPECT_EQ(result1, result2);
  EXPECT_EQ(scale_node->get_state(), sop::SOPNode::ExecutionState::CLEAN);
}

TEST_F(ScaleSOPTest, MarkDirtyInvalidatesCache) {
  auto scale_node = std::make_shared<sop::ScaleSOP>("test_dirty");

  scale_node->set_uniform_scale(2.0F);

  auto input_port = scale_node->get_input_ports().get_port("0");
  input_port->set_data(input_geometry_);

  auto result1 = scale_node->cook();
  ASSERT_NE(result1, nullptr);
  EXPECT_EQ(scale_node->get_state(), sop::SOPNode::ExecutionState::CLEAN);

  // Change parameter - should mark dirty
  scale_node->set_uniform_scale(3.0F);
  EXPECT_EQ(scale_node->get_state(), sop::SOPNode::ExecutionState::DIRTY);

  // Cook again - should recalculate
  auto result2 = scale_node->cook();
  ASSERT_NE(result2, nullptr);

  // Results should differ
  EXPECT_NE(result1, result2);
}
