#include "nodo/core/attribute_promotion.hpp"
#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"

#include <gtest/gtest.h>

using namespace nodo::core;

class AttributePromotionTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a simple quad: 4 points, 4 vertices, 1 primitive
    container = std::make_unique<GeometryContainer>();

    // Set up topology
    container->set_point_count(4);
    container->set_vertex_count(4);

    // Create vertex→point mapping (square)
    container->topology().set_vertex_point(0, 0);
    container->topology().set_vertex_point(1, 1);
    container->topology().set_vertex_point(2, 2);
    container->topology().set_vertex_point(3, 3);

    // Add primitive (quad)
    container->add_primitive({0, 1, 2, 3});

    // Create and set up point positions
    container->add_point_attribute(standard_attrs::P, AttributeType::VEC3F);
    auto* P = container->get_point_attribute_typed<Vec3f>(standard_attrs::P);
    auto P_span = P->values_writable();
    P_span[0] = Vec3f{0.0f, 0.0f, 0.0f};
    P_span[1] = Vec3f{1.0f, 0.0f, 0.0f};
    P_span[2] = Vec3f{1.0f, 1.0f, 0.0f};
    P_span[3] = Vec3f{0.0f, 1.0f, 0.0f};
  }

  std::unique_ptr<GeometryContainer> container;
};

// ============================================================================
// Point ↔ Vertex Tests
// ============================================================================

TEST_F(AttributePromotionTest, PromotePointToVertex_Float) {
  // Create point attribute
  container->add_point_attribute("pscale", AttributeType::FLOAT);
  auto* pscale = container->get_point_attribute_typed<float>("pscale");
  auto pscale_span = pscale->values_writable();

  pscale_span[0] = 1.0f;
  pscale_span[1] = 2.0f;
  pscale_span[2] = 3.0f;
  pscale_span[3] = 4.0f;

  // Promote to vertex
  EXPECT_TRUE(promote_point_to_vertex(*container, "pscale", "vpscale"));

  // Verify vertex attribute exists
  auto* vpscale = container->get_vertex_attribute_typed<float>("vpscale");
  ASSERT_NE(vpscale, nullptr);
  EXPECT_EQ(vpscale->size(), 4);

  // Verify values match (1:1 mapping)
  auto vpscale_span = vpscale->values();
  EXPECT_FLOAT_EQ(vpscale_span[0], 1.0f);
  EXPECT_FLOAT_EQ(vpscale_span[1], 2.0f);
  EXPECT_FLOAT_EQ(vpscale_span[2], 3.0f);
  EXPECT_FLOAT_EQ(vpscale_span[3], 4.0f);
}

TEST_F(AttributePromotionTest, PromotePointToVertex_Vec3f) {
  // Create point color attribute
  container->add_point_attribute("Cd", AttributeType::VEC3F);
  auto* Cd = container->get_point_attribute_typed<Vec3f>("Cd");
  auto Cd_span = Cd->values_writable();

  Cd_span[0] = Vec3f{1.0f, 0.0f, 0.0f}; // Red
  Cd_span[1] = Vec3f{0.0f, 1.0f, 0.0f}; // Green
  Cd_span[2] = Vec3f{0.0f, 0.0f, 1.0f}; // Blue
  Cd_span[3] = Vec3f{1.0f, 1.0f, 0.0f}; // Yellow

  // Promote to vertex
  EXPECT_TRUE(promote_point_to_vertex(*container, "Cd", "vertex_Cd"));

  // Verify vertex colors
  auto* vCd = container->get_vertex_attribute_typed<Vec3f>("vertex_Cd");
  ASSERT_NE(vCd, nullptr);

  auto vCd_span = vCd->values();
  EXPECT_EQ(vCd_span[0], Vec3f(1.0f, 0.0f, 0.0f));
  EXPECT_EQ(vCd_span[1], Vec3f(0.0f, 1.0f, 0.0f));
  EXPECT_EQ(vCd_span[2], Vec3f(0.0f, 0.0f, 1.0f));
  EXPECT_EQ(vCd_span[3], Vec3f(1.0f, 1.0f, 0.0f));
}

TEST_F(AttributePromotionTest, DemoteVertexToPoint_Average) {
  // Create vertex attribute with different values
  container->add_vertex_attribute("vnormal", AttributeType::VEC3F);
  auto* vN = container->get_vertex_attribute_typed<Vec3f>("vnormal");
  auto vN_span = vN->values_writable();

  // All normals pointing up (should average to up)
  vN_span[0] = Vec3f{0.0f, 0.0f, 1.0f};
  vN_span[1] = Vec3f{0.0f, 0.0f, 1.0f};
  vN_span[2] = Vec3f{0.0f, 0.0f, 1.0f};
  vN_span[3] = Vec3f{0.0f, 0.0f, 1.0f};

  // Demote to point
  EXPECT_TRUE(demote_vertex_to_point(*container, "vnormal", "N"));

  // Verify point normals
  auto* N = container->get_point_attribute_typed<Vec3f>("N");
  ASSERT_NE(N, nullptr);

  auto N_span = N->values();
  // Each point has one vertex, so average is the same
  EXPECT_EQ(N_span[0], Vec3f(0.0f, 0.0f, 1.0f));
  EXPECT_EQ(N_span[1], Vec3f(0.0f, 0.0f, 1.0f));
  EXPECT_EQ(N_span[2], Vec3f(0.0f, 0.0f, 1.0f));
  EXPECT_EQ(N_span[3], Vec3f(0.0f, 0.0f, 1.0f));
}

// ============================================================================
// Point ↔ Primitive Tests
// ============================================================================

TEST_F(AttributePromotionTest, PromotePointToPrimitive_Average) {
  // Create point density attribute
  container->add_point_attribute("density", AttributeType::FLOAT);
  auto* density = container->get_point_attribute_typed<float>("density");
  auto density_span = density->values_writable();

  density_span[0] = 1.0f;
  density_span[1] = 2.0f;
  density_span[2] = 3.0f;
  density_span[3] = 4.0f;

  // Promote to primitive (should average: (1+2+3+4)/4 = 2.5)
  EXPECT_TRUE(promote_point_to_primitive(*container, "density", "prim_density"));

  // Verify primitive attribute
  auto* prim_density = container->get_primitive_attribute_typed<float>("prim_density");
  ASSERT_NE(prim_density, nullptr);
  EXPECT_EQ(prim_density->size(), 1);

  auto prim_density_span = prim_density->values();
  EXPECT_FLOAT_EQ(prim_density_span[0], 2.5f);
}

TEST_F(AttributePromotionTest, PromotePointToPrimitive_Vec3f) {
  // Create point color attribute
  container->add_point_attribute("Cd", AttributeType::VEC3F);
  auto* Cd = container->get_point_attribute_typed<Vec3f>("Cd");
  auto Cd_span = Cd->values_writable();

  // All corners are white, average should be white
  Cd_span[0] = Vec3f{1.0f, 1.0f, 1.0f};
  Cd_span[1] = Vec3f{1.0f, 1.0f, 1.0f};
  Cd_span[2] = Vec3f{1.0f, 1.0f, 1.0f};
  Cd_span[3] = Vec3f{1.0f, 1.0f, 1.0f};

  // Promote to primitive
  EXPECT_TRUE(promote_point_to_primitive(*container, "Cd", "prim_Cd"));

  // Verify
  auto* prim_Cd = container->get_primitive_attribute_typed<Vec3f>("prim_Cd");
  ASSERT_NE(prim_Cd, nullptr);

  auto prim_Cd_span = prim_Cd->values();
  EXPECT_EQ(prim_Cd_span[0], Vec3f(1.0f, 1.0f, 1.0f));
}

TEST_F(AttributePromotionTest, DemotePrimitiveToPoint_Distribute) {
  // Create primitive material ID
  container->add_primitive_attribute("material_id", AttributeType::INT);
  auto* mat = container->get_primitive_attribute_typed<int>("material_id");
  auto mat_span = mat->values_writable();

  mat_span[0] = 42;

  // Demote to point (all points should get value 42)
  EXPECT_TRUE(demote_primitive_to_point(*container, "material_id", "point_mat"));

  // Verify all points got the value
  auto* point_mat = container->get_point_attribute_typed<int>("point_mat");
  ASSERT_NE(point_mat, nullptr);

  auto point_mat_span = point_mat->values();
  EXPECT_EQ(point_mat_span[0], 42);
  EXPECT_EQ(point_mat_span[1], 42);
  EXPECT_EQ(point_mat_span[2], 42);
  EXPECT_EQ(point_mat_span[3], 42);
}

// ============================================================================
// Vertex ↔ Primitive Tests
// ============================================================================

TEST_F(AttributePromotionTest, PromoteVertexToPrimitive_Average) {
  // Create vertex attribute
  container->add_vertex_attribute("vweight", AttributeType::FLOAT);
  auto* vweight = container->get_vertex_attribute_typed<float>("vweight");
  auto vweight_span = vweight->values_writable();

  vweight_span[0] = 1.0f;
  vweight_span[1] = 2.0f;
  vweight_span[2] = 3.0f;
  vweight_span[3] = 4.0f;

  // Promote to primitive (average = 2.5)
  EXPECT_TRUE(promote_vertex_to_primitive(*container, "vweight", "prim_weight"));

  // Verify
  auto* prim_weight = container->get_primitive_attribute_typed<float>("prim_weight");
  ASSERT_NE(prim_weight, nullptr);

  auto prim_weight_span = prim_weight->values();
  EXPECT_FLOAT_EQ(prim_weight_span[0], 2.5f);
}

TEST_F(AttributePromotionTest, DemotePrimitivToVertex_Replicate) {
  // Create primitive normal (face normal)
  container->add_primitive_attribute("prim_N", AttributeType::VEC3F);
  auto* prim_N = container->get_primitive_attribute_typed<Vec3f>("prim_N");
  auto prim_N_span = prim_N->values_writable();

  prim_N_span[0] = Vec3f{0.0f, 0.0f, 1.0f}; // Face pointing up

  // Demote to vertex (all vertices should get same normal)
  EXPECT_TRUE(demote_primitive_to_vertex(*container, "prim_N", "vertex_N"));

  // Verify all vertices got the face normal
  auto* vertex_N = container->get_vertex_attribute_typed<Vec3f>("vertex_N");
  ASSERT_NE(vertex_N, nullptr);

  auto vertex_N_span = vertex_N->values();
  EXPECT_EQ(vertex_N_span[0], Vec3f(0.0f, 0.0f, 1.0f));
  EXPECT_EQ(vertex_N_span[1], Vec3f(0.0f, 0.0f, 1.0f));
  EXPECT_EQ(vertex_N_span[2], Vec3f(0.0f, 0.0f, 1.0f));
  EXPECT_EQ(vertex_N_span[3], Vec3f(0.0f, 0.0f, 1.0f));
}

// ============================================================================
// Error Cases
// ============================================================================

TEST_F(AttributePromotionTest, ErrorHandling_AttributeNotFound) {
  // Try to promote non-existent attribute
  EXPECT_FALSE(promote_point_to_vertex(*container, "nonexistent"));
  EXPECT_FALSE(demote_vertex_to_point(*container, "nonexistent"));
  EXPECT_FALSE(promote_point_to_primitive(*container, "nonexistent"));
  EXPECT_FALSE(demote_primitive_to_point(*container, "nonexistent"));
}

TEST_F(AttributePromotionTest, ErrorHandling_AttributeAlreadyExists) {
  // Create point attribute
  container->add_point_attribute("test", AttributeType::FLOAT);

  // Create vertex attribute with same name
  container->add_vertex_attribute("test", AttributeType::FLOAT);

  // Try to promote with same name (should fail - already exists)
  EXPECT_FALSE(promote_point_to_vertex(*container, "test", "test"));
}

TEST_F(AttributePromotionTest, DefaultOutputName_UsesSameName) {
  // Create point attribute
  container->add_point_attribute("value", AttributeType::FLOAT);
  auto* value = container->get_point_attribute_typed<float>("value");
  value->values_writable()[0] = 123.0f;

  // Promote without specifying output name (should create vertex attr with same
  // name)
  EXPECT_TRUE(promote_point_to_vertex(*container, "value"));

  // Verify vertex attribute exists with same name
  auto* vertex_value = container->get_vertex_attribute_typed<float>("value");
  ASSERT_NE(vertex_value, nullptr);
  EXPECT_FLOAT_EQ(vertex_value->values()[0], 123.0f);
}

// ============================================================================
// Complex Geometry Test
// ============================================================================

TEST_F(AttributePromotionTest, ComplexGeometry_TwoTriangles) {
  // Create two triangles sharing an edge (4 points, 6 vertices, 2 primitives)
  auto complex_geo = std::make_unique<GeometryContainer>();

  complex_geo->set_point_count(4);
  complex_geo->set_vertex_count(6);

  // Triangle 1: points 0,1,2
  complex_geo->topology().set_vertex_point(0, 0);
  complex_geo->topology().set_vertex_point(1, 1);
  complex_geo->topology().set_vertex_point(2, 2);

  // Triangle 2: points 1,2,3 (shares edge with triangle 1)
  complex_geo->topology().set_vertex_point(3, 1);
  complex_geo->topology().set_vertex_point(4, 2);
  complex_geo->topology().set_vertex_point(5, 3);

  complex_geo->add_primitive({0, 1, 2});
  complex_geo->add_primitive({3, 4, 5});

  // Create point density
  complex_geo->add_point_attribute("density", AttributeType::FLOAT);
  auto* density = complex_geo->get_point_attribute_typed<float>("density");
  auto density_span = density->values_writable();

  density_span[0] = 1.0f;
  density_span[1] = 2.0f;
  density_span[2] = 3.0f;
  density_span[3] = 4.0f;

  // Promote to primitive
  EXPECT_TRUE(promote_point_to_primitive(*complex_geo, "density", "prim_density"));

  auto* prim_density = complex_geo->get_primitive_attribute_typed<float>("prim_density");
  ASSERT_NE(prim_density, nullptr);
  EXPECT_EQ(prim_density->size(), 2);

  auto prim_density_span = prim_density->values();

  // Triangle 1: average of points 0,1,2 = (1+2+3)/3 = 2.0
  EXPECT_FLOAT_EQ(prim_density_span[0], 2.0f);

  // Triangle 2: average of points 1,2,3 = (2+3+4)/3 = 3.0
  EXPECT_FLOAT_EQ(prim_density_span[1], 3.0f);
}
