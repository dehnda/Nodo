#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/sop/extrude_sop.hpp"

#include <gtest/gtest.h>

namespace attrs = nodo::core::standard_attrs;

TEST(ExtrudeSOPTest, BasicExtrude) {
  // Create a simple quad as input
  auto input = std::make_shared<nodo::core::GeometryContainer>();
  input->set_point_count(4);
  input->set_vertex_count(4); // Set vertex count before using vertices
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

  auto result = extrude_node.cook();
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
  input->set_vertex_count(3);
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

  auto result = extrude_node.cook();
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
  input->set_vertex_count(4);
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

  auto result = extrude_node.cook();
  ASSERT_NE(result, nullptr);

  // Should still create proper extruded geometry
  EXPECT_GT(result->topology().point_count(), input->topology().point_count());
  EXPECT_GE(result->topology().primitive_count(), 6);
}

TEST(ExtrudeSOPTest, NoInput) {
  nodo::sop::ExtrudeSOP extrude_node("test_extrude_no_input");
  auto result = extrude_node.cook();

  // Should return nullptr and set error
  EXPECT_EQ(result, nullptr);
}

TEST(ExtrudeSOPTest, ExtrudeLinesShouldNotCrash) {
  // Create input geometry with line primitives (2 vertices each)
  auto input = std::make_shared<nodo::core::GeometryContainer>();
  input->set_point_count(3);
  input->set_vertex_count(4); // 2 lines * 2 vertices

  // Add position attribute
  input->add_point_attribute(attrs::P, nodo::core::AttributeType::VEC3F);
  auto* positions = input->get_point_attribute_typed<nodo::core::Vec3f>(attrs::P);
  ASSERT_NE(positions, nullptr);

  // Create 3 points
  (*positions)[0] = nodo::core::Vec3f(0.0F, 0.0F, 0.0F);
  (*positions)[1] = nodo::core::Vec3f(1.0F, 0.0F, 0.0F);
  (*positions)[2] = nodo::core::Vec3f(2.0F, 0.0F, 0.0F);

  // Set up vertex-to-point mapping
  input->topology().set_vertex_point(0, 0); // Line 0-1, vertex 0 -> point 0
  input->topology().set_vertex_point(1, 1); // Line 0-1, vertex 1 -> point 1
  input->topology().set_vertex_point(2, 1); // Line 1-2, vertex 0 -> point 1
  input->topology().set_vertex_point(3, 2); // Line 1-2, vertex 1 -> point 2

  // Create 2 line primitives (each with 2 vertices)
  input->add_primitive({0, 1}); // Line from point 0 to point 1
  input->add_primitive({2, 3}); // Line from point 1 to point 2

  // Create ExtrudeSOP
  nodo::sop::ExtrudeSOP extrude_node("test_extrude_lines");
  extrude_node.set_parameter("distance", 1.0F);
  extrude_node.set_input_data(0, input);

  // Should return an error instead of crashing
  auto result = extrude_node.cook();
  EXPECT_EQ(result, nullptr);
}

TEST(ExtrudeSOPTest, ExtrudeMixedGeometryShouldExtrudePolygonsOnly) {
  // Create input with both lines and polygons
  auto input = std::make_shared<nodo::core::GeometryContainer>();
  input->set_point_count(6);
  input->set_vertex_count(8); // 2 vertices (line) + 3 vertices (triangle) + 3
                              // vertices (triangle)

  // Add position attribute
  input->add_point_attribute(attrs::P, nodo::core::AttributeType::VEC3F);
  auto* positions = input->get_point_attribute_typed<nodo::core::Vec3f>(attrs::P);
  ASSERT_NE(positions, nullptr);

  // Points for line
  (*positions)[0] = nodo::core::Vec3f(0.0F, 0.0F, 0.0F);
  (*positions)[1] = nodo::core::Vec3f(1.0F, 0.0F, 0.0F);

  // Points for first triangle
  (*positions)[2] = nodo::core::Vec3f(2.0F, 0.0F, 0.0F);
  (*positions)[3] = nodo::core::Vec3f(2.5F, 1.0F, 0.0F);
  (*positions)[4] = nodo::core::Vec3f(3.0F, 0.0F, 0.0F);

  // Point for second triangle (shares some points)
  (*positions)[5] = nodo::core::Vec3f(2.5F, -1.0F, 0.0F);

  // Set up vertex-to-point mapping
  input->topology().set_vertex_point(0, 0); // Line
  input->topology().set_vertex_point(1, 1);

  input->topology().set_vertex_point(2, 2); // Triangle 1
  input->topology().set_vertex_point(3, 3);
  input->topology().set_vertex_point(4, 4);

  input->topology().set_vertex_point(5, 2); // Triangle 2
  input->topology().set_vertex_point(6, 4);
  input->topology().set_vertex_point(7, 5);

  // Create primitives: 1 line + 2 triangles
  input->add_primitive({0, 1});    // Line (should be skipped)
  input->add_primitive({2, 3, 4}); // Triangle 1 (should be extruded)
  input->add_primitive({5, 6, 7}); // Triangle 2 (should be extruded)

  // Create ExtrudeSOP
  nodo::sop::ExtrudeSOP extrude_node("test_extrude_mixed");
  extrude_node.set_parameter("distance", 1.0F);
  extrude_node.set_input_data(0, input);

  // Should succeed, extruding only the triangles
  auto result = extrude_node.cook();
  ASSERT_NE(result, nullptr);

  // Should have more primitives than just the 2 triangles
  // Each triangle creates: 1 bottom + 1 top + 3 sides = 5 primitives
  // 2 triangles * 5 = 10 primitives
  EXPECT_EQ(result->topology().primitive_count(), 10);

  // Verify positions exist
  auto* result_positions = result->get_point_attribute_typed<nodo::core::Vec3f>(attrs::P);
  ASSERT_NE(result_positions, nullptr);
}
