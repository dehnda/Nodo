#include "nodeflux/core/element_topology.hpp"
#include <gtest/gtest.h>

using namespace nodeflux::core;

class ElementTopologyTest : public ::testing::Test {
protected:
  ElementTopology topo_;
};

TEST_F(ElementTopologyTest, DefaultConstruction) {
  EXPECT_EQ(topo_.point_count(), 0);
  EXPECT_EQ(topo_.vertex_count(), 0);
  EXPECT_EQ(topo_.primitive_count(), 0);
}

TEST_F(ElementTopologyTest, SetCounts) {
  topo_.set_point_count(10);
  topo_.set_vertex_count(20);
  topo_.set_primitive_count(5);

  EXPECT_EQ(topo_.point_count(), 10);
  EXPECT_EQ(topo_.vertex_count(), 20);
  EXPECT_EQ(topo_.primitive_count(), 5);
}

TEST_F(ElementTopologyTest, VertexPointMapping) {
  topo_.set_point_count(4);
  topo_.set_vertex_count(6);

  // Set up vertex→point mapping
  topo_.set_vertex_point(0, 0);
  topo_.set_vertex_point(1, 1);
  topo_.set_vertex_point(2, 2);
  topo_.set_vertex_point(3, 0); // Vertex 3 shares point 0
  topo_.set_vertex_point(4, 1); // Vertex 4 shares point 1
  topo_.set_vertex_point(5, 3);

  EXPECT_EQ(topo_.get_vertex_point(0), 0);
  EXPECT_EQ(topo_.get_vertex_point(1), 1);
  EXPECT_EQ(topo_.get_vertex_point(2), 2);
  EXPECT_EQ(topo_.get_vertex_point(3), 0);
  EXPECT_EQ(topo_.get_vertex_point(4), 1);
  EXPECT_EQ(topo_.get_vertex_point(5), 3);
}

TEST_F(ElementTopologyTest, VertexPointSpan) {
  topo_.set_point_count(3);
  topo_.set_vertex_count(3);

  auto writable = topo_.get_vertex_points_writable();
  writable[0] = 0;
  writable[1] = 1;
  writable[2] = 2;

  auto readonly = topo_.get_vertex_points();
  EXPECT_EQ(readonly[0], 0);
  EXPECT_EQ(readonly[1], 1);
  EXPECT_EQ(readonly[2], 2);
  EXPECT_EQ(readonly.size(), 3);
}

TEST_F(ElementTopologyTest, PrimitiveVertexMapping) {
  topo_.set_point_count(4);
  topo_.set_vertex_count(4);
  topo_.set_primitive_count(1);

  // Set up vertex→point (1:1 for simplicity)
  for (int i = 0; i < 4; ++i) {
    topo_.set_vertex_point(i, i);
  }

  // Create a quad primitive
  std::vector<int> quad_verts = {0, 1, 2, 3};
  topo_.set_primitive_vertices(0, quad_verts);

  const auto &verts = topo_.get_primitive_vertices(0);
  EXPECT_EQ(verts.size(), 4);
  EXPECT_EQ(verts[0], 0);
  EXPECT_EQ(verts[1], 1);
  EXPECT_EQ(verts[2], 2);
  EXPECT_EQ(verts[3], 3);

  EXPECT_EQ(topo_.get_primitive_vertex_count(0), 4);
}

TEST_F(ElementTopologyTest, AddPrimitive) {
  topo_.set_point_count(3);
  topo_.set_vertex_count(3);

  for (int i = 0; i < 3; ++i) {
    topo_.set_vertex_point(i, i);
  }

  std::vector<int> tri_verts = {0, 1, 2};
  size_t prim_idx = topo_.add_primitive(tri_verts);

  EXPECT_EQ(prim_idx, 0);
  EXPECT_EQ(topo_.primitive_count(), 1);

  const auto &verts = topo_.get_primitive_vertices(0);
  EXPECT_EQ(verts.size(), 3);
  EXPECT_EQ(verts[0], 0);
  EXPECT_EQ(verts[1], 1);
  EXPECT_EQ(verts[2], 2);
}

TEST_F(ElementTopologyTest, NGonSupport) {
  topo_.set_point_count(10);
  topo_.set_vertex_count(10);

  for (int i = 0; i < 10; ++i) {
    topo_.set_vertex_point(i, i);
  }

  // Triangle
  topo_.add_primitive({0, 1, 2});

  // Quad
  topo_.add_primitive({3, 4, 5, 6});

  // Pentagon
  topo_.add_primitive({7, 8, 9, 0, 1});

  EXPECT_EQ(topo_.primitive_count(), 3);
  EXPECT_EQ(topo_.get_primitive_vertex_count(0), 3);
  EXPECT_EQ(topo_.get_primitive_vertex_count(1), 4);
  EXPECT_EQ(topo_.get_primitive_vertex_count(2), 5);
}

TEST_F(ElementTopologyTest, Validation_Valid) {
  topo_.set_point_count(3);
  topo_.set_vertex_count(3);

  for (int i = 0; i < 3; ++i) {
    topo_.set_vertex_point(i, i);
  }

  topo_.add_primitive({0, 1, 2});

  EXPECT_TRUE(topo_.validate());
}

TEST_F(ElementTopologyTest, Validation_UnassignedVertexPoint) {
  topo_.set_point_count(2);
  topo_.set_vertex_count(3);

  topo_.set_vertex_point(0, 0);
  topo_.set_vertex_point(1, 1);
  // Vertex 2 left unassigned (value = -1)

  // Validation passes even with unassigned vertices (they're allowed)
  EXPECT_TRUE(topo_.validate());
}

TEST_F(ElementTopologyTest, Validation_ValidWithAllAssigned) {
  topo_.set_point_count(3);
  topo_.set_vertex_count(3);

  for (int i = 0; i < 3; ++i) {
    topo_.set_vertex_point(i, i);
  }

  topo_.set_primitive_count(1);
  topo_.set_primitive_vertices(0, {0, 1, 2});

  // All vertices assigned, primitive valid
  EXPECT_TRUE(topo_.validate());
}

TEST_F(ElementTopologyTest, Validation_EmptyPrimitive) {
  topo_.set_point_count(3);
  topo_.set_vertex_count(3);
  topo_.set_primitive_count(1);

  std::vector<int> empty_verts;
  topo_.set_primitive_vertices(0, empty_verts);

  EXPECT_FALSE(topo_.validate());
}

TEST_F(ElementTopologyTest, Clear) {
  topo_.set_point_count(10);
  topo_.set_vertex_count(20);
  topo_.add_primitive({0, 1, 2});

  topo_.clear();

  EXPECT_EQ(topo_.point_count(), 0);
  EXPECT_EQ(topo_.vertex_count(), 0);
  EXPECT_EQ(topo_.primitive_count(), 0);
}

TEST_F(ElementTopologyTest, ComputeStats) {
  topo_.set_point_count(8);
  topo_.set_vertex_count(12);

  for (int i = 0; i < 12; ++i) {
    topo_.set_vertex_point(i, i % 8);
  }

  // 2 triangles + 2 quads
  topo_.add_primitive({0, 1, 2});
  topo_.add_primitive({3, 4, 5});
  topo_.add_primitive({6, 7, 8, 9});
  topo_.add_primitive({10, 11, 0, 1});

  auto stats = topo_.compute_stats();

  EXPECT_EQ(stats.points, 8);
  EXPECT_EQ(stats.vertices, 12);
  EXPECT_EQ(stats.primitives, 4);
  EXPECT_EQ(stats.min_prim_verts, 3);
  EXPECT_EQ(stats.max_prim_verts, 4);
  EXPECT_DOUBLE_EQ(stats.avg_prim_verts, (3.0 + 3.0 + 4.0 + 4.0) / 4.0);
}

TEST_F(ElementTopologyTest, ComputeStats_Empty) {
  auto stats = topo_.compute_stats();

  EXPECT_EQ(stats.points, 0);
  EXPECT_EQ(stats.vertices, 0);
  EXPECT_EQ(stats.primitives, 0);
  EXPECT_EQ(stats.min_prim_verts, 0);
  EXPECT_EQ(stats.max_prim_verts, 0);
  EXPECT_DOUBLE_EQ(stats.avg_prim_verts, 0.0);
}

TEST_F(ElementTopologyTest, SplitNormals_Example) {
  // Example: Cube with split normals
  // 8 unique points, but 24 vertices (4 per face × 6 faces)
  topo_.set_point_count(8);
  topo_.set_vertex_count(24);

  // Each vertex references one of the 8 points
  // Vertices 0-3 → face 1 (all reference different points)
  topo_.set_vertex_point(0, 0);
  topo_.set_vertex_point(1, 1);
  topo_.set_vertex_point(2, 2);
  topo_.set_vertex_point(3, 3);

  // Vertices 4-7 → face 2 (shares some points with face 1)
  topo_.set_vertex_point(4, 4);
  topo_.set_vertex_point(5, 5);
  topo_.set_vertex_point(6, 6);
  topo_.set_vertex_point(7, 7);

  // Continue for remaining vertices...
  for (int i = 8; i < 24; ++i) {
    topo_.set_vertex_point(i, i % 8);
  }

  // Add 6 quad faces
  topo_.add_primitive({0, 1, 2, 3});
  topo_.add_primitive({4, 5, 6, 7});
  topo_.add_primitive({8, 9, 10, 11});
  topo_.add_primitive({12, 13, 14, 15});
  topo_.add_primitive({16, 17, 18, 19});
  topo_.add_primitive({20, 21, 22, 23});

  EXPECT_EQ(topo_.point_count(), 8);
  EXPECT_EQ(topo_.vertex_count(), 24);
  EXPECT_EQ(topo_.primitive_count(), 6);
  EXPECT_TRUE(topo_.validate());

  // This topology allows each vertex to have unique normals/UVs
  // even though multiple vertices share the same point position
}

TEST_F(ElementTopologyTest, OutOfRangeExceptions) {
  topo_.set_point_count(2);
  topo_.set_vertex_count(3);
  topo_.set_primitive_count(1);

  // Out of range vertex index
  EXPECT_THROW(topo_.get_vertex_point(10), std::out_of_range);
  EXPECT_THROW(topo_.set_vertex_point(10, 0), std::out_of_range);

  // Out of range point index
  EXPECT_THROW(topo_.set_vertex_point(0, 10), std::out_of_range);

  // Out of range primitive index
  EXPECT_THROW(topo_.get_primitive_vertices(10), std::out_of_range);
  EXPECT_THROW(topo_.set_primitive_vertices(10, {0, 1, 2}),
               std::out_of_range);

  // Invalid vertex in primitive
  EXPECT_THROW(topo_.set_primitive_vertices(0, {0, 1, 10}),
               std::out_of_range);
  EXPECT_THROW(topo_.add_primitive({0, 1, 10}), std::out_of_range);
}

TEST_F(ElementTopologyTest, Reserve) {
  topo_.reserve_vertices(100);
  topo_.reserve_primitives(50);

  // Should not throw or crash
  topo_.set_point_count(10);
  topo_.set_vertex_count(100);

  for (int i = 0; i < 100; ++i) {
    topo_.set_vertex_point(i, i % 10);
  }

  for (int i = 0; i < 50; ++i) {
    topo_.add_primitive({i % 100, (i + 1) % 100, (i + 2) % 100});
  }

  EXPECT_EQ(topo_.primitive_count(), 50);
  EXPECT_TRUE(topo_.validate());
}
