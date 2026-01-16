#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/sop/extrude_sop.hpp"

#include <gtest/gtest.h>

namespace attrs = nodo::core::standard_attrs;

TEST(ExtrudeSOPTest, BasicExtrude) {
  // Create a simple quad as input
  auto input = std::make_shared<nodo::core::GeometryContainer>();
  input->set_point_count(4);
  input->add_point_attribute(attrs::P, nodo::core::AttributeType::VEC3F);

  auto* positions = input->get_point_attribute_typed<nodo::core::Vec3f>(attrs::P);
  ASSERT_NE(positions, nullptr);

  // Create a quad on XY plane
  (*positions)[0] = nodo::core::Vec3f(0.0F, 0.0F, 0.0F);
  (*positions)[1] = nodo::core::Vec3f(1.0F, 0.0F, 0.0F);
  (*positions)[2] = nodo::core::Vec3f(1.0F, 1.0F, 0.0F);
  (*positions)[3] = nodo::core::Vec3f(0.0F, 1.0F, 0.0F);

  // Create topology for the quad
  std::vector<int> quad_verts;
  for (int i = 0; i < 4; ++i) {
    input->topology().set_vertex_point(i, i);
    quad_verts.push_back(i);
  }
  input->add_primitive(quad_verts);

  // Create and execute ExtrudeSOP
  nodo::sop::ExtrudeSOP extrude_node("test_extrude");
  extrude_node.set_distance(1.0);
  extrude_node.set_input_data(0, input);

  auto result = extrude_node.execute();
  ASSERT_NE(result, nullptr);

  // Should have more points than input (original + extruded)
  EXPECT_GT(result->topology().point_count(), input->topology().point_count());

  // Should have multiple primitives (bottom, top, and side walls)
  // For a quad: 1 bottom + 1 top + 4 sides = 6 primitives
  EXPECT_GE(result->topology().primitive_count(), 6);

  // Verify positions exist
  auto* result_positions = result->get_point_attribute_typed<nodo::core::Vec3f>(attrs::P);
  ASSERT_NE(result_positions, nullptr);
}

TEST(ExtrudeSOPTest, ExtrudeWithInset) {
  // Create a simple triangle
  auto input = std::make_shared<nodo::core::GeometryContainer>();
  input->set_point_count(3);
  input->add_point_attribute(attrs::P, nodo::core::AttributeType::VEC3F);

  auto* positions = input->get_point_attribute_typed<nodo::core::Vec3f>(attrs::P);
  (*positions)[0] = nodo::core::Vec3f(0.0F, 0.0F, 0.0F);
  (*positions)[1] = nodo::core::Vec3f(1.0F, 0.0F, 0.0F);
  (*positions)[2] = nodo::core::Vec3f(0.5F, 1.0F, 0.0F);

  // Create topology
  std::vector<int> tri_verts;
  for (int i = 0; i < 3; ++i) {
    input->topology().set_vertex_point(i, i);
    tri_verts.push_back(i);
  }
  input->add_primitive(tri_verts);

  // Create ExtrudeSOP with inset
  nodo::sop::ExtrudeSOP extrude_node("test_extrude_inset");
  extrude_node.set_distance(0.5);
  extrude_node.set_parameter("inset", 0.2F);
  extrude_node.set_input_data(0, input);

  auto result = extrude_node.execute();
  ASSERT_NE(result, nullptr);

  // Should have more points (inset creates new bottom points + extruded top
  // points)
  EXPECT_GT(result->topology().point_count(), input->topology().point_count());

  // For triangle: 1 bottom + 1 top + 3 sides = 5 primitives
  EXPECT_GE(result->topology().primitive_count(), 5);
}

TEST(ExtrudeSOPTest, UniformDirection) {
  // Create a simple quad
  auto input = std::make_shared<nodo::core::GeometryContainer>();
  input->set_point_count(4);
  input->add_point_attribute(attrs::P, nodo::core::AttributeType::VEC3F);

  auto* positions = input->get_point_attribute_typed<nodo::core::Vec3f>(attrs::P);
  (*positions)[0] = nodo::core::Vec3f(0.0F, 0.0F, 0.0F);
  (*positions)[1] = nodo::core::Vec3f(1.0F, 0.0F, 0.0F);
  (*positions)[2] = nodo::core::Vec3f(1.0F, 1.0F, 0.0F);
  (*positions)[3] = nodo::core::Vec3f(0.0F, 1.0F, 0.0F);

  std::vector<int> quad_verts;
  for (int i = 0; i < 4; ++i) {
    input->topology().set_vertex_point(i, i);
    quad_verts.push_back(i);
  }
  input->add_primitive(quad_verts);

  // Use uniform direction mode (mode = 1)
  nodo::sop::ExtrudeSOP extrude_node("test_extrude_uniform");
  extrude_node.set_distance(1.0);
  extrude_node.set_parameter("mode", 1); // Uniform Direction
  extrude_node.set_parameter("direction_x", 1.0F);
  extrude_node.set_parameter("direction_y", 0.0F);
  extrude_node.set_parameter("direction_z", 0.0F);
  extrude_node.set_input_data(0, input);

  auto result = extrude_node.execute();
  ASSERT_NE(result, nullptr);

  // Should still create proper extruded geometry
  EXPECT_GT(result->topology().point_count(), input->topology().point_count());
  EXPECT_GE(result->topology().primitive_count(), 6);
}

TEST(ExtrudeSOPTest, NoInput) {
  nodo::sop::ExtrudeSOP extrude_node("test_extrude_no_input");
  auto result = extrude_node.execute();

  // Should return nullptr and set error
  EXPECT_EQ(result, nullptr);
}
