#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/sop/bevel_sop.hpp"

#include <gtest/gtest.h>

namespace attrs = nodo::core::standard_attrs;

static std::shared_ptr<nodo::core::GeometryContainer> make_cube() {
  auto geo = std::make_shared<nodo::core::GeometryContainer>();
  geo->set_point_count(8);
  geo->add_point_attribute(attrs::P, nodo::core::AttributeType::VEC3F);
  auto* P = geo->get_point_attribute_typed<nodo::core::Vec3f>(attrs::P);
  // Cube from (-1,-1,-1) to (1,1,1)
  (*P)[0] = nodo::core::Vec3f(-1, -1, -1);
  (*P)[1] = nodo::core::Vec3f(1, -1, -1);
  (*P)[2] = nodo::core::Vec3f(1, 1, -1);
  (*P)[3] = nodo::core::Vec3f(-1, 1, -1);
  (*P)[4] = nodo::core::Vec3f(-1, -1, 1);
  (*P)[5] = nodo::core::Vec3f(-1, 1, 1);
  (*P)[6] = nodo::core::Vec3f(1, 1, 1);
  (*P)[7] = nodo::core::Vec3f(1, -1, 1);

  // 6 quad faces (closed manifold cube), each primitive uses 4 vertices
  geo->set_vertex_count(6 * 4);
  // Faces: back (z=-1), front (z=1), left (x=-1), right (x=1), bottom (y=-1),
  // top (y=1)
  int faces[6][4] = {
      {0, 1, 2, 3}, // back
      {4, 5, 6, 7}, // front
      {0, 3, 5, 4}, // left
      {1, 7, 6, 2}, // right
      {0, 4, 7, 1}, // bottom
      {3, 2, 6, 5}  // top
  };
  int v = 0;
  for (int f = 0; f < 6; ++f) {
    std::vector<int> prim;
    for (int k = 0; k < 4; ++k) {
      geo->topology().set_vertex_point(v, faces[f][k]);
      prim.push_back(v);
      ++v;
    }
    geo->add_primitive(prim);
  }
  return geo;
}

TEST(BevelSOPTest, NoOpWidthZero) {
  auto cube = make_cube();
  nodo::sop::BevelSOP bevel("bevel_noop");
  bevel.set_width(0.0F); // no-op
  bevel.set_segments(3);
  bevel.set_input_data(0, cube);
  auto result = bevel.cook();
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->point_count(), cube->point_count());
  EXPECT_EQ(result->primitive_count(), cube->primitive_count());
}

TEST(BevelSOPTest, EdgeModeSegments1Counts) {
  auto cube = make_cube();
  nodo::sop::BevelSOP bevel("bevel_edge_s1");
  bevel.set_width(0.1F);
  bevel.set_segments(1);
  bevel.set_parameter("mode", static_cast<int>(nodo::sop::BevelSOP::BevelType::Edge));
  bevel.set_input_data(0, cube);
  auto result = bevel.cook();
  ASSERT_NE(result, nullptr);
  // Each of 8 corners gets 3 beveled positions (one per adjacent face) = 24 points
  EXPECT_EQ(result->point_count(), 24u);
  // 6 original faces + 12 bevel quads = 18 primitives
  EXPECT_EQ(result->primitive_count(), 18u);
}

TEST(BevelSOPTest, EdgeModeSegments3Counts) {
  auto cube = make_cube();
  nodo::sop::BevelSOP bevel("bevel_edge_s3");
  bevel.set_width(0.1F);
  bevel.set_segments(3);
  bevel.set_parameter("mode", static_cast<int>(nodo::sop::BevelSOP::BevelType::Edge));
  bevel.set_input_data(0, cube);
  auto result = bevel.cook();
  ASSERT_NE(result, nullptr);
  // 24 corner points + 12 edges * 2 intermediate segment points * 2 ends = 24 + 48 = 72
  EXPECT_EQ(result->point_count(), 72u);
  // 6 original faces + 12 edges * 3 segments = 42 primitives
  EXPECT_EQ(result->primitive_count(), 42u);
}

TEST(BevelSOPTest, VertexApexFanSegments3Counts) {
  auto cube = make_cube();
  nodo::sop::BevelSOP bevel("bevel_vertex_apexfan");
  bevel.set_width(0.1F);
  bevel.set_segments(3);
  bevel.set_parameter("mode", static_cast<int>(nodo::sop::BevelSOP::BevelType::Vertex));
  bevel.set_parameter("corner_style", static_cast<int>(nodo::sop::BevelSOP::CornerStyle::ApexFan));
  bevel.set_input_data(0, cube);
  auto result = bevel.cook();
  ASSERT_NE(result, nullptr);
  // Points: 8 + 8 corners * (3*segments) = 8 + 8*9 = 80
  EXPECT_EQ(result->point_count(), 80u);
  // Primitives: 6 original + per corner (apex triangles=3 + ring
  // quads=(segments-1)*3 = 6) = 9*8=72 => 78 total
  EXPECT_EQ(result->primitive_count(), 78u);
}

TEST(BevelSOPTest, VertexRingStartSegments3Counts) {
  auto cube = make_cube();
  nodo::sop::BevelSOP bevel("bevel_vertex_ringstart");
  bevel.set_width(0.1F);
  bevel.set_segments(3);
  bevel.set_parameter("mode", static_cast<int>(nodo::sop::BevelSOP::BevelType::Vertex));
  bevel.set_parameter("corner_style", static_cast<int>(nodo::sop::BevelSOP::CornerStyle::RingStart));
  bevel.set_input_data(0, cube);
  auto result = bevel.cook();
  ASSERT_NE(result, nullptr);
  // Points same as ApexFan for now: 80
  EXPECT_EQ(result->point_count(), 80u);
  // Primitives: 6 original + per corner (ring quads=(segments-1)*3 = 6) => 6 +
  // 8*6 = 54
  EXPECT_EQ(result->primitive_count(), 54u);
}

TEST(BevelSOPTest, VertexGridSegments3Counts) {
  auto cube = make_cube();
  nodo::sop::BevelSOP bevel("bevel_vertex_grid");
  bevel.set_width(0.1F);
  bevel.set_segments(3);
  bevel.set_parameter("mode", static_cast<int>(nodo::sop::BevelSOP::BevelType::Vertex));
  bevel.set_parameter("corner_style", static_cast<int>(nodo::sop::BevelSOP::CornerStyle::Grid));
  bevel.set_input_data(0, cube);
  auto result = bevel.cook();
  ASSERT_NE(result, nullptr);
  // Points per corner: 3 * segments^2 = 27; total 8 + 8*27 = 224
  EXPECT_EQ(result->point_count(), 224u);
  // Primitives: per corner quads between rings: (segments-1) * (3*segments) =
  // 2*9=18 per corner => 6 + 8*18 = 150
  EXPECT_EQ(result->primitive_count(), 150u);
}

TEST(BevelSOPTest, CombinedEdgeVertexSegments3Counts) {
  auto cube = make_cube();
  nodo::sop::BevelSOP bevel("bevel_edgevertex_s3");
  bevel.set_width(0.1F);
  bevel.set_segments(3);
  bevel.set_parameter("mode", static_cast<int>(nodo::sop::BevelSOP::BevelType::EdgeVertex));
  bevel.set_parameter("corner_style", static_cast<int>(nodo::sop::BevelSOP::CornerStyle::RingStart));
  bevel.set_input_data(0, cube);
  auto result = bevel.cook();
  ASSERT_NE(result, nullptr);
  // Combined mode: edge bevel points + vertex patch points
  // Exact count depends on implementation; verify we have more than edge-only
  EXPECT_GT(result->point_count(), 72u);
  // Primitives: edge quads + vertex patch quads + original faces
  EXPECT_GT(result->primitive_count(), 42u);
}

TEST(BevelSOPTest, AngleLimitFiltersEdges) {
  auto cube = make_cube();
  nodo::sop::BevelSOP bevel("bevel_edge_angle_limit");
  bevel.set_width(0.1F);
  bevel.set_segments(2);
  bevel.set_parameter("angle_limit", 100.0F); // dihedral 90 < 100 => no edges
  bevel.set_parameter("mode", static_cast<int>(nodo::sop::BevelSOP::BevelType::Edge));
  bevel.set_input_data(0, cube);
  auto result = bevel.cook();
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->point_count(), cube->point_count());
  EXPECT_EQ(result->primitive_count(), cube->primitive_count());
}

TEST(BevelSOPTest, ClampWidthEdge) {
  auto cube = make_cube();
  nodo::sop::BevelSOP bevel("bevel_edge_clamp");
  bevel.set_width(1.2F); // greater than half edge length (1.0)
  bevel.set_segments(1);
  bevel.set_parameter("mode", static_cast<int>(nodo::sop::BevelSOP::BevelType::Edge));
  bevel.set_parameter("clamp_overlap", true);
  bevel.set_input_data(0, cube);
  auto result = bevel.cook();
  ASSERT_NE(result, nullptr);
  // Points should match edge mode segments=1 (clamped width doesn't change count)
  EXPECT_EQ(result->point_count(), 24u);
}
