#include "nodo/core/geometry_container.hpp"
#include <gtest/gtest.h>

using namespace nodo::core;

class GeometryContainerTest : public ::testing::Test {};

TEST_F(GeometryContainerTest, DefaultConstruction) {
  GeometryContainer geo;

  EXPECT_EQ(geo.point_count(), 0);
  EXPECT_EQ(geo.vertex_count(), 0);
  EXPECT_EQ(geo.primitive_count(), 0);
  EXPECT_TRUE(geo.validate());
}

TEST_F(GeometryContainerTest, SetTopologyCounts) {
  GeometryContainer geo;

  geo.set_point_count(10);
  geo.set_vertex_count(20);
  geo.set_primitive_count(5);

  EXPECT_EQ(geo.point_count(), 10);
  EXPECT_EQ(geo.vertex_count(), 20);
  EXPECT_EQ(geo.primitive_count(), 5);

  // Attribute sets should automatically resize
  EXPECT_EQ(geo.point_attributes().size(), 10);
  EXPECT_EQ(geo.vertex_attributes().size(), 20);
  EXPECT_EQ(geo.primitive_attributes().size(), 5);
}

TEST_F(GeometryContainerTest, AddPointAttributes) {
  GeometryContainer geo;
  geo.set_point_count(100);

  EXPECT_TRUE(geo.add_point_attribute("P", AttributeType::VEC3F));
  EXPECT_TRUE(geo.add_point_attribute("Cd", AttributeType::VEC3F));
  EXPECT_TRUE(geo.add_point_attribute("id", AttributeType::INT));

  EXPECT_TRUE(geo.has_point_attribute("P"));
  EXPECT_TRUE(geo.has_point_attribute("Cd"));
  EXPECT_TRUE(geo.has_point_attribute("id"));
  EXPECT_FALSE(geo.has_point_attribute("N"));

  // All attributes should have 100 elements
  auto *pos = geo.get_point_attribute("P");
  EXPECT_EQ(pos->size(), 100);
}

TEST_F(GeometryContainerTest, AddVertexAttributes) {
  GeometryContainer geo;
  geo.set_vertex_count(50);

  EXPECT_TRUE(geo.add_vertex_attribute("N", AttributeType::VEC3F));
  EXPECT_TRUE(geo.add_vertex_attribute("uv", AttributeType::VEC2F));

  EXPECT_TRUE(geo.has_vertex_attribute("N"));
  EXPECT_TRUE(geo.has_vertex_attribute("uv"));

  auto *normals = geo.get_vertex_attribute("N");
  auto *uvs = geo.get_vertex_attribute("uv");

  EXPECT_EQ(normals->size(), 50);
  EXPECT_EQ(uvs->size(), 50);
}

TEST_F(GeometryContainerTest, AddPrimitiveAttributes) {
  GeometryContainer geo;
  geo.set_primitive_count(20);

  EXPECT_TRUE(geo.add_primitive_attribute("material_id", AttributeType::INT));
  EXPECT_TRUE(
      geo.add_primitive_attribute("primitive_Cd", AttributeType::VEC3F));

  EXPECT_TRUE(geo.has_primitive_attribute("material_id"));
  EXPECT_TRUE(geo.has_primitive_attribute("primitive_Cd"));
}

TEST_F(GeometryContainerTest, AddDetailAttributes) {
  GeometryContainer geo;

  EXPECT_TRUE(geo.add_detail_attribute("name", AttributeType::STRING));
  EXPECT_TRUE(geo.add_detail_attribute("frame", AttributeType::INT));

  EXPECT_TRUE(geo.has_detail_attribute("name"));
  EXPECT_TRUE(geo.has_detail_attribute("frame"));

  // Detail attributes should have size 1 (global)
  geo.detail_attributes().resize(1);
  auto *name_attr = geo.get_detail_attribute("name");
  EXPECT_EQ(name_attr->size(), 1);
}

TEST_F(GeometryContainerTest, TypedAccessPointAttributes) {
  GeometryContainer geo;
  geo.set_point_count(10);
  geo.add_point_attribute("P", AttributeType::VEC3F);

  auto *positions = geo.get_point_attribute_typed<Vec3f>("P");
  ASSERT_NE(positions, nullptr);

  (*positions)[0] = Vec3f(1.0F, 2.0F, 3.0F);
  (*positions)[1] = Vec3f(4.0F, 5.0F, 6.0F);

  EXPECT_FLOAT_EQ((*positions)[0].x(), 1.0F);
  EXPECT_FLOAT_EQ((*positions)[0].y(), 2.0F);
  EXPECT_FLOAT_EQ((*positions)[0].z(), 3.0F);

  EXPECT_FLOAT_EQ((*positions)[1].x(), 4.0F);
}

TEST_F(GeometryContainerTest, TypedAccessVertexAttributes) {
  GeometryContainer geo;
  geo.set_vertex_count(5);
  geo.add_vertex_attribute("N", AttributeType::VEC3F);

  auto *normals = geo.get_vertex_attribute_typed<Vec3f>("N");
  ASSERT_NE(normals, nullptr);

  (*normals)[0] = Vec3f(0.0F, 0.0F, 1.0F);
  (*normals)[1] = Vec3f(0.0F, 1.0F, 0.0F);

  EXPECT_FLOAT_EQ((*normals)[0].z(), 1.0F);
  EXPECT_FLOAT_EQ((*normals)[1].y(), 1.0F);
}

TEST_F(GeometryContainerTest, StandardPositionAccessor) {
  GeometryContainer geo;
  geo.set_point_count(10);
  geo.add_point_attribute("P", AttributeType::VEC3F);

  auto *pos = geo.positions();
  ASSERT_NE(pos, nullptr);

  (*pos)[0] = Vec3f(1.0F, 2.0F, 3.0F);
  EXPECT_FLOAT_EQ((*pos)[0].x(), 1.0F);
}

TEST_F(GeometryContainerTest, StandardNormalAccessor) {
  GeometryContainer geo;
  geo.set_vertex_count(10);
  geo.add_vertex_attribute("N", AttributeType::VEC3F);

  auto *normals = geo.normals();
  ASSERT_NE(normals, nullptr);

  (*normals)[0] = Vec3f(0.0F, 0.0F, 1.0F);
  EXPECT_FLOAT_EQ((*normals)[0].z(), 1.0F);
}

TEST_F(GeometryContainerTest, StandardUVAccessor) {
  GeometryContainer geo;
  geo.set_vertex_count(10);
  geo.add_vertex_attribute("uv", AttributeType::VEC2F);

  auto *uvs = geo.uvs();
  ASSERT_NE(uvs, nullptr);

  (*uvs)[0] = Vec2f(0.5F, 0.5F);
  EXPECT_FLOAT_EQ((*uvs)[0].x(), 0.5F);
  EXPECT_FLOAT_EQ((*uvs)[0].y(), 0.5F);
}

TEST_F(GeometryContainerTest, StandardColorAccessor) {
  GeometryContainer geo;
  geo.set_point_count(10);
  geo.add_point_attribute("Cd", AttributeType::VEC3F);

  auto *colors = geo.colors();
  ASSERT_NE(colors, nullptr);

  (*colors)[0] = Vec3f(1.0F, 0.0F, 0.0F); // Red
  EXPECT_FLOAT_EQ((*colors)[0].x(), 1.0F);
}

TEST_F(GeometryContainerTest, EnsurePositionAttribute) {
  GeometryContainer geo;
  geo.set_point_count(10);

  EXPECT_FALSE(geo.has_point_attribute("P"));

  geo.ensure_position_attribute();

  EXPECT_TRUE(geo.has_point_attribute("P"));

  auto *pos = geo.positions();
  ASSERT_NE(pos, nullptr);
  EXPECT_EQ(pos->size(), 10);
}

TEST_F(GeometryContainerTest, EnsureNormalAttribute) {
  GeometryContainer geo;
  geo.set_vertex_count(20);

  EXPECT_FALSE(geo.has_vertex_attribute("N"));

  geo.ensure_normal_attribute();

  EXPECT_TRUE(geo.has_vertex_attribute("N"));

  auto *normals = geo.normals();
  ASSERT_NE(normals, nullptr);
  EXPECT_EQ(normals->size(), 20);
}

TEST_F(GeometryContainerTest, AddPrimitive) {
  GeometryContainer geo;
  geo.set_point_count(4);
  geo.set_vertex_count(4);

  // Set up vertex→point mapping
  for (int i = 0; i < 4; ++i) {
    geo.topology().set_vertex_point(i, i);
  }

  // Add a quad primitive
  std::vector<int> quad_verts = {0, 1, 2, 3};
  size_t prim_idx = geo.add_primitive(quad_verts);

  EXPECT_EQ(prim_idx, 0);
  EXPECT_EQ(geo.primitive_count(), 1);

  // Primitive attributes should auto-resize
  EXPECT_EQ(geo.primitive_attributes().size(), 1);
}

TEST_F(GeometryContainerTest, Validate) {
  GeometryContainer geo;
  geo.set_point_count(3);
  geo.set_vertex_count(3);

  for (int i = 0; i < 3; ++i) {
    geo.topology().set_vertex_point(i, i);
  }

  geo.add_primitive({0, 1, 2});

  geo.add_point_attribute("P", AttributeType::VEC3F);
  geo.add_vertex_attribute("N", AttributeType::VEC3F);

  EXPECT_TRUE(geo.validate());
}

TEST_F(GeometryContainerTest, Clear) {
  GeometryContainer geo;
  geo.set_point_count(100);
  geo.set_vertex_count(200);
  geo.add_point_attribute("P", AttributeType::VEC3F);
  geo.add_vertex_attribute("N", AttributeType::VEC3F);

  geo.clear();

  EXPECT_EQ(geo.point_count(), 0);
  EXPECT_EQ(geo.vertex_count(), 0);
  EXPECT_EQ(geo.primitive_count(), 0);
  EXPECT_EQ(geo.point_attributes().size(), 0);
  EXPECT_EQ(geo.vertex_attributes().size(), 0);
}

TEST_F(GeometryContainerTest, Clone) {
  GeometryContainer geo;
  geo.set_point_count(3);
  geo.set_vertex_count(3);
  geo.add_point_attribute("P", AttributeType::VEC3F);

  auto *pos = geo.positions();
  (*pos)[0] = Vec3f(1.0F, 2.0F, 3.0F);
  (*pos)[1] = Vec3f(4.0F, 5.0F, 6.0F);

  auto cloned = geo.clone();

  EXPECT_EQ(cloned.point_count(), 3);
  EXPECT_EQ(cloned.vertex_count(), 3);
  EXPECT_TRUE(cloned.has_point_attribute("P"));

  auto *cloned_pos = cloned.positions();
  EXPECT_FLOAT_EQ((*cloned_pos)[0].x(), 1.0F);
  EXPECT_FLOAT_EQ((*cloned_pos)[1].x(), 4.0F);
}

TEST_F(GeometryContainerTest, ComputeStats) {
  GeometryContainer geo;
  geo.set_point_count(8);
  geo.set_vertex_count(24);
  geo.set_primitive_count(6);

  geo.add_point_attribute("P", AttributeType::VEC3F);
  geo.add_point_attribute("Cd", AttributeType::VEC3F);
  geo.add_vertex_attribute("N", AttributeType::VEC3F);
  geo.add_vertex_attribute("uv", AttributeType::VEC2F);
  geo.add_primitive_attribute("material_id", AttributeType::INT);

  auto stats = geo.compute_stats();

  EXPECT_EQ(stats.points, 8);
  EXPECT_EQ(stats.vertices, 24);
  EXPECT_EQ(stats.primitives, 6);
  EXPECT_EQ(stats.point_attributes, 2);
  EXPECT_EQ(stats.vertex_attributes, 2);
  EXPECT_EQ(stats.primitive_attributes, 1);
  EXPECT_GT(stats.total_memory_bytes, 0);
}

TEST_F(GeometryContainerTest, MemoryUsage) {
  GeometryContainer geo;
  geo.set_point_count(100);
  geo.add_point_attribute("P", AttributeType::VEC3F);
  geo.add_point_attribute("Cd", AttributeType::VEC3F);

  size_t mem = geo.memory_usage();
  EXPECT_GT(mem, 0);

  // Should be at least 100 * sizeof(Vec3f) * 2 attributes
  size_t expected_min = 100 * sizeof(Vec3f) * 2;
  EXPECT_GE(mem, expected_min);
}

TEST_F(GeometryContainerTest, RealWorldExample_Triangle) {
  // Build a simple triangle with positions, normals, and UVs
  GeometryContainer geo;

  // Set topology
  geo.set_point_count(3);   // 3 unique points
  geo.set_vertex_count(3);  // 3 vertices (no split normals for simplicity)

  // Map vertices to points (1:1 for this simple case)
  for (int i = 0; i < 3; ++i) {
    geo.topology().set_vertex_point(i, i);
  }

  // Add primitive
  geo.add_primitive({0, 1, 2});

  // Create attributes
  geo.add_point_attribute("P", AttributeType::VEC3F);
  geo.add_vertex_attribute("N", AttributeType::VEC3F);
  geo.add_vertex_attribute("uv", AttributeType::VEC2F);

  // Populate positions
  auto *pos = geo.positions();
  (*pos)[0] = Vec3f(0.0F, 0.0F, 0.0F);
  (*pos)[1] = Vec3f(1.0F, 0.0F, 0.0F);
  (*pos)[2] = Vec3f(0.5F, 1.0F, 0.0F);

  // Populate normals (all pointing up)
  auto *normals = geo.normals();
  for (int i = 0; i < 3; ++i) {
    (*normals)[i] = Vec3f(0.0F, 0.0F, 1.0F);
  }

  // Populate UVs
  auto *uvs = geo.uvs();
  (*uvs)[0] = Vec2f(0.0F, 0.0F);
  (*uvs)[1] = Vec2f(1.0F, 0.0F);
  (*uvs)[2] = Vec2f(0.5F, 1.0F);

  // Validate
  EXPECT_TRUE(geo.validate());

  // Check primitive
  EXPECT_EQ(geo.primitive_count(), 1);
  const auto &prim_verts = geo.topology().get_primitive_vertices(0);
  EXPECT_EQ(prim_verts.size(), 3);
  EXPECT_EQ(prim_verts[0], 0);
  EXPECT_EQ(prim_verts[1], 1);
  EXPECT_EQ(prim_verts[2], 2);

  // Check data integrity
  EXPECT_FLOAT_EQ((*pos)[1].x(), 1.0F);
  EXPECT_FLOAT_EQ((*normals)[0].z(), 1.0F);
  EXPECT_FLOAT_EQ((*uvs)[2].y(), 1.0F);
}

TEST_F(GeometryContainerTest, RealWorldExample_CubeWithSplitNormals) {
  // Build a cube with split normals (24 vertices for 8 points)
  GeometryContainer geo;

  geo.set_point_count(8);   // 8 unique corner positions
  geo.set_vertex_count(24); // 24 vertices (4 per face * 6 faces)

  // Create attributes
  geo.add_point_attribute("P", AttributeType::VEC3F);
  geo.add_vertex_attribute("N", AttributeType::VEC3F);

  // Set cube corner positions
  auto *pos = geo.positions();
  (*pos)[0] = Vec3f(0.0F, 0.0F, 0.0F);
  (*pos)[1] = Vec3f(1.0F, 0.0F, 0.0F);
  (*pos)[2] = Vec3f(1.0F, 1.0F, 0.0F);
  (*pos)[3] = Vec3f(0.0F, 1.0F, 0.0F);
  (*pos)[4] = Vec3f(0.0F, 0.0F, 1.0F);
  (*pos)[5] = Vec3f(1.0F, 0.0F, 1.0F);
  (*pos)[6] = Vec3f(1.0F, 1.0F, 1.0F);
  (*pos)[7] = Vec3f(0.0F, 1.0F, 1.0F);

  // Map vertices to points (multiple vertices can share same point)
  // Front face (vertices 0-3)
  geo.topology().set_vertex_point(0, 0);
  geo.topology().set_vertex_point(1, 1);
  geo.topology().set_vertex_point(2, 2);
  geo.topology().set_vertex_point(3, 3);

  // Back face (vertices 4-7)
  geo.topology().set_vertex_point(4, 5);
  geo.topology().set_vertex_point(5, 4);
  geo.topology().set_vertex_point(6, 7);
  geo.topology().set_vertex_point(7, 6);

  // (Continue for other faces...)
  // For simplicity, just set up first 8 vertices

  // Set normals (each face has unique normal)
  auto *normals = geo.normals();
  // Front face normal
  (*normals)[0] = Vec3f(0.0F, 0.0F, -1.0F);
  (*normals)[1] = Vec3f(0.0F, 0.0F, -1.0F);
  (*normals)[2] = Vec3f(0.0F, 0.0F, -1.0F);
  (*normals)[3] = Vec3f(0.0F, 0.0F, -1.0F);

  // Back face normal
  (*normals)[4] = Vec3f(0.0F, 0.0F, 1.0F);
  (*normals)[5] = Vec3f(0.0F, 0.0F, 1.0F);
  (*normals)[6] = Vec3f(0.0F, 0.0F, 1.0F);
  (*normals)[7] = Vec3f(0.0F, 0.0F, 1.0F);

  // Add primitives
  geo.add_primitive({0, 1, 2, 3}); // Front face
  geo.add_primitive({4, 5, 6, 7}); // Back face

  EXPECT_EQ(geo.primitive_count(), 2);
  EXPECT_EQ(geo.point_count(), 8);
  EXPECT_EQ(geo.vertex_count(), 24);

  // This demonstrates point vs vertex separation:
  // - 8 unique points (positions)
  // - 24 vertices (corners with unique normals)
  // - Multiple vertices reference the same point but have different normals
}

TEST_F(GeometryContainerTest, RemoveAttributes) {
  GeometryContainer geo;
  geo.set_point_count(10);
  geo.add_point_attribute("P", AttributeType::VEC3F);
  geo.add_point_attribute("Cd", AttributeType::VEC3F);

  EXPECT_TRUE(geo.has_point_attribute("P"));
  EXPECT_TRUE(geo.has_point_attribute("Cd"));

  EXPECT_TRUE(geo.remove_point_attribute("Cd"));
  EXPECT_FALSE(geo.has_point_attribute("Cd"));
  EXPECT_TRUE(geo.has_point_attribute("P"));
}
