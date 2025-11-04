#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/sop/polyextrude_sop.hpp"
#include <gtest/gtest.h>

namespace attrs = nodo::core::standard_attrs;

TEST(PolyExtrudeSOPTest, ExtrudeEdges) {
  // Create input geometry with edge primitives
  auto input = std::make_shared<nodo::core::GeometryContainer>();
  input->set_point_count(4);
  input->set_vertex_count(6); // 3 edges * 2 vertices

  // Add position attribute
  input->add_point_attribute(attrs::P, nodo::core::AttributeType::VEC3F);
  auto *positions =
      input->get_point_attribute_typed<nodo::core::Vec3f>(attrs::P);
  ASSERT_NE(positions, nullptr);

  // Create a simple line with 4 points
  (*positions)[0] = nodo::core::Vec3f(0.0F, 0.0F, 0.0F);
  (*positions)[1] = nodo::core::Vec3f(1.0F, 0.0F, 0.0F);
  (*positions)[2] = nodo::core::Vec3f(2.0F, 0.0F, 0.0F);
  (*positions)[3] = nodo::core::Vec3f(3.0F, 0.0F, 0.0F);

  // Set up vertex-to-point mapping (each vertex references the correct point)
  input->topology().set_vertex_point(0, 0); // Edge 0-1, vertex 0 -> point 0
  input->topology().set_vertex_point(1, 1); // Edge 0-1, vertex 1 -> point 1
  input->topology().set_vertex_point(2, 1); // Edge 1-2, vertex 0 -> point 1
  input->topology().set_vertex_point(3, 2); // Edge 1-2, vertex 1 -> point 2
  input->topology().set_vertex_point(4, 2); // Edge 2-3, vertex 0 -> point 2
  input->topology().set_vertex_point(5, 3); // Edge 2-3, vertex 1 -> point 3

  // Create 3 edge primitives (each with 2 vertices)
  input->add_primitive({0, 1}); // Edge point 0 to point 1
  input->add_primitive({2, 3}); // Edge point 1 to point 2
  input->add_primitive({4, 5}); // Edge point 2 to point 3

  // Create PolyExtrudeSOP and set to edge mode
  nodo::sop::PolyExtrudeSOP extrude_node("test_edge_extrude");
  extrude_node.set_parameter("extrusion_type", 1); // Edges mode
  extrude_node.set_parameter("distance", 1.0F);
  extrude_node.set_input_data(0, input);

  auto result = extrude_node.execute_for_test();
  ASSERT_NE(result, nullptr);

  // Each edge should produce a quad (4 vertices)
  // 3 edges * 4 vertices = 12 vertices
  EXPECT_EQ(result->topology().vertex_count(), 12);

  // Should have 3 quad primitives
  EXPECT_EQ(result->topology().primitive_count(), 3);

  // Verify positions exist
  auto *result_positions =
      result->get_point_attribute_typed<nodo::core::Vec3f>(attrs::P);
  ASSERT_NE(result_positions, nullptr);

  // In individual mode, we should have 4 points per edge: 3 edges * 4 = 12
  // points
  EXPECT_EQ(result->topology().point_count(), 12);
}

TEST(PolyExtrudeSOPTest, ExtrudeFacesStillWorks) {
  // Create input geometry with a quad face
  auto input = std::make_shared<nodo::core::GeometryContainer>();
  input->set_point_count(4);
  input->set_vertex_count(4);

  input->add_point_attribute(attrs::P, nodo::core::AttributeType::VEC3F);
  auto *positions =
      input->get_point_attribute_typed<nodo::core::Vec3f>(attrs::P);
  ASSERT_NE(positions, nullptr);

  (*positions)[0] = nodo::core::Vec3f(0.0F, 0.0F, 0.0F);
  (*positions)[1] = nodo::core::Vec3f(1.0F, 0.0F, 0.0F);
  (*positions)[2] = nodo::core::Vec3f(1.0F, 1.0F, 0.0F);
  (*positions)[3] = nodo::core::Vec3f(0.0F, 1.0F, 0.0F);

  for (int i = 0; i < 4; ++i) {
    input->topology().set_vertex_point(i, i);
  }

  input->add_primitive({0, 1, 2, 3}); // Quad face

  // Create PolyExtrudeSOP with faces mode (default)
  nodo::sop::PolyExtrudeSOP extrude_node("test_face_extrude");
  extrude_node.set_parameter("extrusion_type", 0); // Faces mode
  extrude_node.set_parameter("distance", 1.0F);
  extrude_node.set_input_data(0, input);

  auto result = extrude_node.execute_for_test();
  ASSERT_NE(result, nullptr);

  // Should have more points than input (original + extruded)
  EXPECT_GT(result->topology().point_count(), input->topology().point_count());

  // Should have bottom face + top face + 4 side quads = 6 primitives
  EXPECT_EQ(result->topology().primitive_count(), 6);
}

TEST(PolyExtrudeSOPTest, ExtrudeEdgesWithDistance) {
  auto input = std::make_shared<nodo::core::GeometryContainer>();
  input->set_point_count(2);
  input->set_vertex_count(2);

  input->add_point_attribute(attrs::P, nodo::core::AttributeType::VEC3F);
  auto *positions =
      input->get_point_attribute_typed<nodo::core::Vec3f>(attrs::P);
  ASSERT_NE(positions, nullptr);

  // Simple horizontal edge
  (*positions)[0] = nodo::core::Vec3f(0.0F, 0.0F, 0.0F);
  (*positions)[1] = nodo::core::Vec3f(1.0F, 0.0F, 0.0F);

  input->topology().set_vertex_point(0, 0);
  input->topology().set_vertex_point(1, 1);
  input->add_primitive({0, 1});

  nodo::sop::PolyExtrudeSOP extrude_node("test_edge_distance");
  extrude_node.set_parameter("extrusion_type", 1);
  extrude_node.set_parameter("distance", 2.0F);
  extrude_node.set_input_data(0, input);

  auto result = extrude_node.execute_for_test();
  ASSERT_NE(result, nullptr);

  auto *result_positions =
      result->get_point_attribute_typed<nodo::core::Vec3f>(attrs::P);
  ASSERT_NE(result_positions, nullptr);

  // Check that extruded points are offset by distance
  // Points should be extruded perpendicular to the edge
  // The edge is along X axis, extrusion perpendicular creates a quad
  // Original points and extruded points should exist
  const auto &bottom_p0 = (*result_positions)[0]; // Original p0
  const auto &bottom_p1 = (*result_positions)[1]; // Original p1
  const auto &top_p1 = (*result_positions)[2];    // Extruded p1
  const auto &top_p0 = (*result_positions)[3];    // Extruded p0

  // Verify that bottom edge is horizontal
  EXPECT_NEAR(bottom_p0.y(), 0.0F, 0.01F);
  EXPECT_NEAR(bottom_p1.y(), 0.0F, 0.01F);

  // The extrusion creates a perpendicular offset
  // Distance from bottom to top should equal extrusion distance
  float dist_p0 = (top_p0 - bottom_p0).norm();
  float dist_p1 = (top_p1 - bottom_p1).norm();

  EXPECT_NEAR(dist_p0, 2.0F, 0.01F);
  EXPECT_NEAR(dist_p1, 2.0F, 0.01F);
}

TEST(PolyExtrudeSOPTest, ExtrudeEdgesWithCustomDirection) {
  auto input = std::make_shared<nodo::core::GeometryContainer>();
  input->set_point_count(2);
  input->set_vertex_count(2);

  input->add_point_attribute(attrs::P, nodo::core::AttributeType::VEC3F);
  auto *positions =
      input->get_point_attribute_typed<nodo::core::Vec3f>(attrs::P);
  ASSERT_NE(positions, nullptr);

  // Horizontal edge along X-axis
  (*positions)[0] = nodo::core::Vec3f(0.0F, 0.0F, 0.0F);
  (*positions)[1] = nodo::core::Vec3f(1.0F, 0.0F, 0.0F);

  input->topology().set_vertex_point(0, 0);
  input->topology().set_vertex_point(1, 1);
  input->add_primitive({0, 1});

  nodo::sop::PolyExtrudeSOP extrude_node("test_custom_direction");
  extrude_node.set_parameter("extrusion_type", 1); // Edges mode
  extrude_node.set_parameter("distance", 1.0F);

  // Set custom direction mode
  extrude_node.set_parameter("edge_direction_mode", 1); // Custom direction
  extrude_node.set_parameter("edge_direction_x", 0.0F);
  extrude_node.set_parameter("edge_direction_y", 0.0F);
  extrude_node.set_parameter("edge_direction_z", 1.0F); // Extrude in +Z

  extrude_node.set_input_data(0, input);

  auto result = extrude_node.execute_for_test();
  ASSERT_NE(result, nullptr);

  auto *result_positions =
      result->get_point_attribute_typed<nodo::core::Vec3f>(attrs::P);
  ASSERT_NE(result_positions, nullptr);

  // Points: [bottom_p0, bottom_p1, top_p1, top_p0]
  const auto &bottom_p0 = (*result_positions)[0];
  const auto &top_p0 = (*result_positions)[3];

  // With custom direction (0, 0, 1), extrusion should be along Z-axis
  EXPECT_NEAR(bottom_p0.x(), 0.0F, 0.01F);
  EXPECT_NEAR(bottom_p0.y(), 0.0F, 0.01F);
  EXPECT_NEAR(bottom_p0.z(), 0.0F, 0.01F);

  EXPECT_NEAR(top_p0.x(), 0.0F, 0.01F);
  EXPECT_NEAR(top_p0.y(), 0.0F, 0.01F);
  EXPECT_NEAR(top_p0.z(), 1.0F, 0.01F); // Moved +1.0 in Z direction
}
