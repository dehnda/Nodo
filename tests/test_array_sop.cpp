#include "nodeflux/geometry/sphere_generator.hpp"
#include "nodeflux/sop/array_sop.hpp"
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

  // Extract faces - convert vertex indices to point indices
  Eigen::MatrixXi faces(topology.primitive_count(), 3);
  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &vert_indices = topology.get_primitive_vertices(prim_idx);
    for (size_t j = 0; j < 3 && j < vert_indices.size(); ++j) {
      // Convert vertex index to point index
      faces(prim_idx, j) = topology.get_vertex_point(vert_indices[j]);
    }
  }

  return std::make_shared<core::Mesh>(vertices, faces);
}

class ArraySOPTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a simple sphere for testing (now returns GeometryContainer)
    auto sphere_result =
        geometry::SphereGenerator::generate_uv_sphere(0.5, 4, 4);
    ASSERT_TRUE(sphere_result.has_value());

    // Convert to Mesh and wrap in GeometryData
    auto mesh = container_to_mesh(sphere_result.value());
    ASSERT_NE(mesh, nullptr);
    input_geometry_ = std::make_shared<sop::GeometryData>(mesh);
  }

  std::shared_ptr<sop::GeometryData> input_geometry_;
};

TEST_F(ArraySOPTest, LinearArrayCreation) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_array");

  // Configure linear array
  array_node->set_parameter("array_type", 0); // LINEAR = 0
  array_node->set_parameter("count", 5);
  array_node->set_parameter("linear_offset_x", 1.0F);
  array_node->set_parameter("linear_offset_y", 0.0F);
  array_node->set_parameter("linear_offset_z", 0.0F);

  // Connect input (simulate node connection)
  auto input_port = array_node->get_input_ports().get_port("mesh");
  ASSERT_NE(input_port, nullptr);

  // Set input data directly on port
  input_port->set_data(input_geometry_);

  // Execute
  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);
  EXPECT_FALSE(result->is_empty());

  auto result_mesh = result->get_mesh();
  ASSERT_NE(result_mesh, nullptr);

  // Verify output has 5 copies
  size_t input_vertex_count = input_geometry_->get_vertex_count();
  EXPECT_EQ(result_mesh->vertex_count(), input_vertex_count * 5);
}

TEST_F(ArraySOPTest, RadialArrayCreation) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_radial");

  // Configure radial array
  array_node->set_parameter("array_type", 1); // RADIAL = 1
  array_node->set_parameter("count", 8);
  array_node->set_parameter("radial_radius", 2.0F);
  array_node->set_parameter("angle_step", 45.0F); // 360/8 = 45 degrees

  // Connect input
  auto input_port = array_node->get_input_ports().get_port("mesh");
  input_port->set_data(input_geometry_);

  // Execute
  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);
  EXPECT_FALSE(result->is_empty());

  auto result_mesh = result->get_mesh();
  ASSERT_NE(result_mesh, nullptr);

  // Verify output has 8 copies
  size_t input_vertex_count = input_geometry_->get_vertex_count();
  EXPECT_EQ(result_mesh->vertex_count(), input_vertex_count * 8);
}

TEST_F(ArraySOPTest, GridArrayCreation) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_grid");

  // Configure grid array
  array_node->set_parameter("array_type", 2); // GRID = 2
  array_node->set_parameter("grid_width", 3);
  array_node->set_parameter("grid_height", 4); // 3x4 grid = 12 copies
  array_node->set_parameter("grid_spacing_x", 1.5F);
  array_node->set_parameter("grid_spacing_y", 2.0F);

  // Connect input
  auto input_port = array_node->get_input_ports().get_port("mesh");
  input_port->set_data(input_geometry_);

  // Execute
  auto result = array_node->cook();

  ASSERT_NE(result, nullptr);
  EXPECT_FALSE(result->is_empty());

  auto result_mesh = result->get_mesh();
  ASSERT_NE(result_mesh, nullptr);

  // Verify output has 12 copies (3x4)
  size_t input_vertex_count = input_geometry_->get_vertex_count();
  EXPECT_EQ(result_mesh->vertex_count(), input_vertex_count * 12);
}

TEST_F(ArraySOPTest, InstanceAttributesPresent) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_attrs");

  // Configure array
  array_node->set_parameter("array_type", 0); // LINEAR = 0
  array_node->set_parameter("count", 3);
  array_node->set_parameter("linear_offset_x", 1.0F);
  array_node->set_parameter("linear_offset_y", 0.0F);
  array_node->set_parameter("linear_offset_z", 0.0F);

  // Connect input
  auto input_port = array_node->get_input_ports().get_port("mesh");
  input_port->set_data(input_geometry_);

  // Execute
  auto result = array_node->cook();
  ASSERT_NE(result, nullptr);

  // Check for instance_id vertex attribute
  auto vertex_instance_ids = result->get_vertex_attribute("instance_id");
  ASSERT_TRUE(vertex_instance_ids.has_value());
  EXPECT_EQ(vertex_instance_ids->size(), result->get_vertex_count());

  // Check for instance_id face attribute
  auto face_instance_ids = result->get_face_attribute("instance_id");
  ASSERT_TRUE(face_instance_ids.has_value());
  EXPECT_EQ(face_instance_ids->size(), result->get_face_count());

  // Check for global attributes
  auto instance_count = result->get_global_attribute("instance_count");
  ASSERT_TRUE(instance_count.has_value());
  EXPECT_EQ(std::get<int>(*instance_count), 3);

  auto array_type = result->get_global_attribute("array_type");
  ASSERT_TRUE(array_type.has_value());
}

TEST_F(ArraySOPTest, InstanceIDsAreCorrect) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_id_values");

  // Configure simple array with 3 copies
  array_node->set_parameter("array_type", 0); // LINEAR = 0
  array_node->set_parameter("count", 3);
  array_node->set_parameter("linear_offset_x", 1.0F);
  array_node->set_parameter("linear_offset_y", 0.0F);
  array_node->set_parameter("linear_offset_z", 0.0F);

  // Connect input
  auto input_port = array_node->get_input_ports().get_port("mesh");
  input_port->set_data(input_geometry_);

  // Execute
  auto result = array_node->cook();
  ASSERT_NE(result, nullptr);

  // Get instance IDs
  auto vertex_instance_ids = result->get_vertex_attribute("instance_id");
  ASSERT_TRUE(vertex_instance_ids.has_value());

  size_t input_vert_count = input_geometry_->get_vertex_count();

  // Verify first copy has ID 0
  for (size_t i = 0; i < input_vert_count; ++i) {
    EXPECT_EQ(std::get<int>((*vertex_instance_ids)[i]), 0);
  }

  // Verify second copy has ID 1
  for (size_t i = input_vert_count; i < input_vert_count * 2; ++i) {
    EXPECT_EQ(std::get<int>((*vertex_instance_ids)[i]), 1);
  }

  // Verify third copy has ID 2
  for (size_t i = input_vert_count * 2; i < input_vert_count * 3; ++i) {
    EXPECT_EQ(std::get<int>((*vertex_instance_ids)[i]), 2);
  }
}

TEST_F(ArraySOPTest, CachingWorks) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_cache");

  // Configure array
  array_node->set_parameter("array_type", 0); // LINEAR = 0
  array_node->set_parameter("count", 2);
  array_node->set_parameter("linear_offset_x", 1.0F);
  array_node->set_parameter("linear_offset_y", 0.0F);
  array_node->set_parameter("linear_offset_z", 0.0F);

  // Connect input
  auto input_port = array_node->get_input_ports().get_port("mesh");
  input_port->set_data(input_geometry_);

  // First cook
  auto result1 = array_node->cook();
  ASSERT_NE(result1, nullptr);
  auto cook_time1 = array_node->get_cook_duration();

  // Second cook (should be cached)
  auto result2 = array_node->cook();
  ASSERT_NE(result2, nullptr);
  auto cook_time2 = array_node->get_cook_duration();

  // Should return same result
  EXPECT_EQ(result1, result2);

  // Should be CLEAN state
  EXPECT_EQ(array_node->get_state(), sop::SOPNode::ExecutionState::CLEAN);
}

TEST_F(ArraySOPTest, MarkDirtyInvalidatesCache) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_dirty");

  // Configure and execute
  array_node->set_parameter("array_type", 0); // LINEAR = 0
  array_node->set_parameter("count", 2);

  auto input_port = array_node->get_input_ports().get_port("mesh");
  input_port->set_data(input_geometry_);

  auto result1 = array_node->cook();
  ASSERT_NE(result1, nullptr);
  EXPECT_EQ(array_node->get_state(), sop::SOPNode::ExecutionState::CLEAN);

  // Change parameter - should mark dirty
  array_node->set_parameter("count", 3);
  EXPECT_EQ(array_node->get_state(), sop::SOPNode::ExecutionState::DIRTY);

  // Cook again - should recalculate
  auto result2 = array_node->cook();
  ASSERT_NE(result2, nullptr);

  // Results should differ (different vertex counts)
  EXPECT_NE(result1->get_vertex_count(), result2->get_vertex_count());
}

TEST_F(ArraySOPTest, NoInputReturnsError) {
  auto array_node = std::make_shared<sop::ArraySOP>("test_no_input");

  // Configure but don't connect input
  array_node->set_parameter("array_type", 0); // LINEAR = 0
  array_node->set_parameter("count", 2);

  // Execute without input
  auto result = array_node->cook();

  EXPECT_EQ(result, nullptr);
  EXPECT_EQ(array_node->get_state(), sop::SOPNode::ExecutionState::ERROR);
  EXPECT_FALSE(array_node->get_last_error().empty());
}
