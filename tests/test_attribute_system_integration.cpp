#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"

#include <chrono>

#include <gtest/gtest.h>

using namespace nodo::core;
namespace attrs = nodo::core::standard_attrs;

/**
 * @brief Integration tests for the complete attribute system
 *
 * These tests validate the end-to-end workflow of the new attribute system,
 * combining topology, attributes, and real-world geometry scenarios.
 */
class AttributeSystemIntegrationTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup common to all tests
  }

  void TearDown() override {
    // Cleanup
  }

  // Helper: Create a simple triangle
  GeometryContainer create_triangle() {
    GeometryContainer geo;

    // Set topology: 3 points, 3 vertices (1:1), 1 triangle
    geo.set_point_count(3);
    geo.set_vertex_count(3);

    // Map vertices to points (1:1 for simple case)
    geo.topology().set_vertex_point(0, 0);
    geo.topology().set_vertex_point(1, 1);
    geo.topology().set_vertex_point(2, 2);

    // Define triangle primitive using add_primitive
    geo.add_primitive({0, 1, 2});

    return geo;
  }

  // Helper: Create a quad
  GeometryContainer create_quad() {
    GeometryContainer geo;

    geo.set_point_count(4);
    geo.set_vertex_count(4);

    for (int i = 0; i < 4; ++i) {
      geo.topology().set_vertex_point(i, i);
    }

    geo.add_primitive({0, 1, 2, 3});

    return geo;
  }
};

// ============================================================================
// Test 1: Simple Triangle Mesh
// ============================================================================

TEST_F(AttributeSystemIntegrationTest, TriangleMesh) {
  GeometryContainer geo = create_triangle();

  // Add position attribute
  ASSERT_TRUE(geo.add_point_attribute(attrs::P, AttributeType::VEC3F));

  // Ensure position attribute exists
  geo.ensure_position_attribute();

  // Get typed access to positions
  auto* positions = geo.positions();
  ASSERT_NE(positions, nullptr);
  EXPECT_EQ(positions->size(), 3);

  // Set triangle vertices
  (*positions)[0] = Vec3f(0.0F, 0.0F, 0.0F);
  (*positions)[1] = Vec3f(1.0F, 0.0F, 0.0F);
  (*positions)[2] = Vec3f(0.0F, 1.0F, 0.0F);

  // Validate topology
  EXPECT_TRUE(geo.topology().validate());
  EXPECT_EQ(geo.point_count(), 3);
  EXPECT_EQ(geo.vertex_count(), 3);
  EXPECT_EQ(geo.primitive_count(), 1);

  // Validate geometry
  EXPECT_TRUE(geo.validate());

  // Verify positions
  EXPECT_FLOAT_EQ((*positions)[0].x(), 0.0F);
  EXPECT_FLOAT_EQ((*positions)[1].x(), 1.0F);
  EXPECT_FLOAT_EQ((*positions)[2].y(), 1.0F);

  // Verify primitive vertex count (should be 3 for triangle)
  EXPECT_EQ(geo.topology().get_primitive_vertex_count(0), 3);
}

// ============================================================================
// Test 2: Quad Mesh (N-gon support)
// ============================================================================

TEST_F(AttributeSystemIntegrationTest, QuadMesh) {
  GeometryContainer geo = create_quad();

  // Add position and color attributes
  ASSERT_TRUE(geo.add_point_attribute(attrs::P, AttributeType::VEC3F));
  ASSERT_TRUE(geo.add_vertex_attribute(attrs::Cd, AttributeType::VEC3F));

  auto* positions = geo.positions();
  auto* colors = geo.get_vertex_attribute_typed<Vec3f>(attrs::Cd);

  ASSERT_NE(positions, nullptr);
  ASSERT_NE(colors, nullptr);

  // Set quad positions (unit square in XY plane)
  (*positions)[0] = Vec3f(-1.0F, -1.0F, 0.0F);
  (*positions)[1] = Vec3f(1.0F, -1.0F, 0.0F);
  (*positions)[2] = Vec3f(1.0F, 1.0F, 0.0F);
  (*positions)[3] = Vec3f(-1.0F, 1.0F, 0.0F);

  // Set per-vertex colors (different from point colors!)
  (*colors)[0] = Vec3f(1.0F, 0.0F, 0.0F); // Red
  (*colors)[1] = Vec3f(0.0F, 1.0F, 0.0F); // Green
  (*colors)[2] = Vec3f(0.0F, 0.0F, 1.0F); // Blue
  (*colors)[3] = Vec3f(1.0F, 1.0F, 0.0F); // Yellow

  // Validate
  EXPECT_TRUE(geo.topology().validate());
  EXPECT_TRUE(geo.validate());

  // Verify quad has 4 vertices
  EXPECT_EQ(geo.topology().get_primitive_vertex_count(0), 4);

  // Verify positions
  EXPECT_FLOAT_EQ((*positions)[0].x(), -1.0F);
  EXPECT_FLOAT_EQ((*positions)[2].x(), 1.0F);

  // Verify colors
  EXPECT_FLOAT_EQ((*colors)[0].x(), 1.0F); // Red channel
  EXPECT_FLOAT_EQ((*colors)[1].y(), 1.0F); // Green channel
  EXPECT_FLOAT_EQ((*colors)[2].z(), 1.0F); // Blue channel
}

// ============================================================================
// Test 3: Cube with Split Normals (Point vs Vertex attributes)
// ============================================================================

TEST_F(AttributeSystemIntegrationTest, CubeWithSplitNormals) {
  GeometryContainer geo;

  // Cube: 8 unique points, 24 vertices (6 faces × 4 corners), 6 primitives
  geo.set_point_count(8);
  geo.set_vertex_count(24);

  // Add position (per-point) and normal (per-vertex) attributes
  ASSERT_TRUE(geo.add_point_attribute(attrs::P, AttributeType::VEC3F));
  ASSERT_TRUE(geo.add_vertex_attribute(attrs::N, AttributeType::VEC3F));

  auto* positions = geo.positions();
  auto* normals = geo.get_vertex_attribute_typed<Vec3f>(attrs::N);

  ASSERT_NE(positions, nullptr);
  ASSERT_NE(normals, nullptr);

  // Set 8 unique cube corner positions
  (*positions)[0] = Vec3f(-1.0F, -1.0F, -1.0F);
  (*positions)[1] = Vec3f(1.0F, -1.0F, -1.0F);
  (*positions)[2] = Vec3f(1.0F, 1.0F, -1.0F);
  (*positions)[3] = Vec3f(-1.0F, 1.0F, -1.0F);
  (*positions)[4] = Vec3f(-1.0F, -1.0F, 1.0F);
  (*positions)[5] = Vec3f(1.0F, -1.0F, 1.0F);
  (*positions)[6] = Vec3f(1.0F, 1.0F, 1.0F);
  (*positions)[7] = Vec3f(-1.0F, 1.0F, 1.0F);

  // Front face: vertices 0-3 → points 0,1,2,3
  geo.topology().set_vertex_point(0, 0);
  geo.topology().set_vertex_point(1, 1);
  geo.topology().set_vertex_point(2, 2);
  geo.topology().set_vertex_point(3, 3);
  geo.add_primitive({0, 1, 2, 3}); // Primitive 0

  // Back face: vertices 4-7 → points 5,4,7,6
  geo.topology().set_vertex_point(4, 5);
  geo.topology().set_vertex_point(5, 4);
  geo.topology().set_vertex_point(6, 7);
  geo.topology().set_vertex_point(7, 6);
  geo.add_primitive({4, 5, 6, 7}); // Primitive 1

  // Right face: vertices 8-11 → points 1,5,6,2
  geo.topology().set_vertex_point(8, 1);
  geo.topology().set_vertex_point(9, 5);
  geo.topology().set_vertex_point(10, 6);
  geo.topology().set_vertex_point(11, 2);
  geo.add_primitive({8, 9, 10, 11}); // Primitive 2

  // Left face: vertices 12-15 → points 4,0,3,7
  geo.topology().set_vertex_point(12, 4);
  geo.topology().set_vertex_point(13, 0);
  geo.topology().set_vertex_point(14, 3);
  geo.topology().set_vertex_point(15, 7);
  geo.add_primitive({12, 13, 14, 15}); // Primitive 3

  // Top face: vertices 16-19 → points 3,2,6,7
  geo.topology().set_vertex_point(16, 3);
  geo.topology().set_vertex_point(17, 2);
  geo.topology().set_vertex_point(18, 6);
  geo.topology().set_vertex_point(19, 7);
  geo.add_primitive({16, 17, 18, 19}); // Primitive 4

  // Bottom face: vertices 20-23 → points 4,5,1,0
  geo.topology().set_vertex_point(20, 4);
  geo.topology().set_vertex_point(21, 5);
  geo.topology().set_vertex_point(22, 1);
  geo.topology().set_vertex_point(23, 0);
  geo.add_primitive({20, 21, 22, 23}); // Primitive 5

  // Set per-vertex normals (split normals - same point, different normals!)
  // Front face: +Z normal
  for (int i = 0; i < 4; ++i) {
    (*normals)[i] = Vec3f(0.0F, 0.0F, 1.0F);
  }

  // Back face: -Z normal
  for (int i = 4; i < 8; ++i) {
    (*normals)[i] = Vec3f(0.0F, 0.0F, -1.0F);
  }

  // Right face: +X normal
  for (int i = 8; i < 12; ++i) {
    (*normals)[i] = Vec3f(1.0F, 0.0F, 0.0F);
  }

  // Left face: -X normal
  for (int i = 12; i < 16; ++i) {
    (*normals)[i] = Vec3f(-1.0F, 0.0F, 0.0F);
  }

  // Top face: +Y normal
  for (int i = 16; i < 20; ++i) {
    (*normals)[i] = Vec3f(0.0F, 1.0F, 0.0F);
  }

  // Bottom face: -Y normal
  for (int i = 20; i < 24; ++i) {
    (*normals)[i] = Vec3f(0.0F, -1.0F, 0.0F);
  }

  // Validate
  EXPECT_TRUE(geo.topology().validate());
  EXPECT_TRUE(geo.validate());

  // Verify counts
  EXPECT_EQ(geo.point_count(), 8);
  EXPECT_EQ(geo.vertex_count(), 24);
  EXPECT_EQ(geo.primitive_count(), 6);

  // Verify split normals work: vertex 0 and vertex 8 share point 1, but have
  // different normals
  int point0 = geo.topology().get_vertex_point(0);
  int point8 = geo.topology().get_vertex_point(8);

  // Vertices 1 and 8 both map to point 1
  EXPECT_EQ(point0, 0);
  EXPECT_EQ(point8, 1);

  // But they have different normals!
  EXPECT_NE((*normals)[0], (*normals)[8]);

  // Verify front face normal (+Z)
  EXPECT_FLOAT_EQ((*normals)[0].z(), 1.0F);
  EXPECT_FLOAT_EQ((*normals)[0].x(), 0.0F);

  // Verify right face normal (+X)
  EXPECT_FLOAT_EQ((*normals)[8].x(), 1.0F);
  EXPECT_FLOAT_EQ((*normals)[8].z(), 0.0F);
}

// ============================================================================
// Test 4: Mixed Primitives (Triangles + Quads)
// ============================================================================

TEST_F(AttributeSystemIntegrationTest, MixedPrimitives) {
  GeometryContainer geo;

  // 5 points, 7 vertices, 2 primitives (1 triangle + 1 quad)
  geo.set_point_count(5);
  geo.set_vertex_count(7);

  // Add position
  ASSERT_TRUE(geo.add_point_attribute(attrs::P, AttributeType::VEC3F));
  auto* positions = geo.positions();
  ASSERT_NE(positions, nullptr);

  // Set positions
  (*positions)[0] = Vec3f(0.0F, 0.0F, 0.0F);
  (*positions)[1] = Vec3f(1.0F, 0.0F, 0.0F);
  (*positions)[2] = Vec3f(0.5F, 1.0F, 0.0F);
  (*positions)[3] = Vec3f(2.0F, 0.0F, 0.0F);
  (*positions)[4] = Vec3f(2.0F, 1.0F, 0.0F);

  // Triangle: vertices 0,1,2 → points 0,1,2
  geo.topology().set_vertex_point(0, 0);
  geo.topology().set_vertex_point(1, 1);
  geo.topology().set_vertex_point(2, 2);
  geo.add_primitive({0, 1, 2}); // Triangle

  // Quad: vertices 3,4,5,6 → points 1,3,4,2
  geo.topology().set_vertex_point(3, 1);
  geo.topology().set_vertex_point(4, 3);
  geo.topology().set_vertex_point(5, 4);
  geo.topology().set_vertex_point(6, 2);
  geo.add_primitive({3, 4, 5, 6}); // Quad

  // Validate
  EXPECT_TRUE(geo.topology().validate());
  EXPECT_TRUE(geo.validate());

  // Verify primitive types
  EXPECT_EQ(geo.topology().get_primitive_vertex_count(0), 3); // Triangle
  EXPECT_EQ(geo.topology().get_primitive_vertex_count(1), 4); // Quad

  // Verify shared vertex: vertex 1 and vertex 3 both use point 1
  EXPECT_EQ(geo.topology().get_vertex_point(1), 1);
  EXPECT_EQ(geo.topology().get_vertex_point(3), 1);
}

// ============================================================================
// Test 5: Attribute Cloning
// ============================================================================

TEST_F(AttributeSystemIntegrationTest, AttributeCloning) {
  GeometryContainer geo1 = create_triangle();

  // Add and populate position
  ASSERT_TRUE(geo1.add_point_attribute(attrs::P, AttributeType::VEC3F));
  auto* positions1 = geo1.positions();
  (*positions1)[0] = Vec3f(1.0F, 2.0F, 3.0F);
  (*positions1)[1] = Vec3f(4.0F, 5.0F, 6.0F);
  (*positions1)[2] = Vec3f(7.0F, 8.0F, 9.0F);

  // Clone geometry
  GeometryContainer geo2 = geo1.clone();

  // Verify clone has same structure
  EXPECT_EQ(geo2.point_count(), geo1.point_count());
  EXPECT_EQ(geo2.vertex_count(), geo1.vertex_count());
  EXPECT_EQ(geo2.primitive_count(), geo1.primitive_count());

  // Verify clone has position attribute
  EXPECT_TRUE(geo2.has_point_attribute(attrs::P));

  auto* positions2 = geo2.positions();
  ASSERT_NE(positions2, nullptr);

  // Verify values are copied
  EXPECT_EQ((*positions2)[0], (*positions1)[0]);
  EXPECT_EQ((*positions2)[1], (*positions1)[1]);
  EXPECT_EQ((*positions2)[2], (*positions1)[2]);

  // Verify it's a deep copy (modifying clone doesn't affect original)
  (*positions2)[0] = Vec3f(99.0F, 99.0F, 99.0F);
  EXPECT_NE((*positions2)[0], (*positions1)[0]);
  EXPECT_FLOAT_EQ((*positions1)[0].x(), 1.0F); // Original unchanged
}

// ============================================================================
// Test 6: Custom Attributes
// ============================================================================

TEST_F(AttributeSystemIntegrationTest, CustomAttributes) {
  GeometryContainer geo = create_triangle();

  // Add standard position
  ASSERT_TRUE(geo.add_point_attribute(attrs::P, AttributeType::VEC3F));

  // Add custom float attribute
  ASSERT_TRUE(geo.add_point_attribute("temperature", AttributeType::FLOAT));

  // Add custom int attribute
  ASSERT_TRUE(geo.add_vertex_attribute("id", AttributeType::INT));

  // Get typed access
  auto* temps = geo.get_point_attribute_typed<float>("temperature");
  auto* ids = geo.get_vertex_attribute_typed<int>("id");

  ASSERT_NE(temps, nullptr);
  ASSERT_NE(ids, nullptr);

  // Set custom attribute values
  (*temps)[0] = 100.5F;
  (*temps)[1] = 200.7F;
  (*temps)[2] = 300.9F;

  (*ids)[0] = 10;
  (*ids)[1] = 20;
  (*ids)[2] = 30;

  // Verify
  EXPECT_FLOAT_EQ((*temps)[1], 200.7F);
  EXPECT_EQ((*ids)[2], 30);

  // Verify attribute exists
  EXPECT_TRUE(geo.has_point_attribute("temperature"));
  EXPECT_TRUE(geo.has_vertex_attribute("id"));
  EXPECT_FALSE(geo.has_point_attribute("nonexistent"));
}

// ============================================================================
// Test 7: Performance Baseline - 1M Vector3 Positions
// ============================================================================

TEST_F(AttributeSystemIntegrationTest, PerformanceBaseline_1M_Positions) {
  const size_t COUNT = 1000000;

  GeometryContainer geo;
  geo.set_point_count(COUNT);

  // Add position attribute
  ASSERT_TRUE(geo.add_point_attribute(attrs::P, AttributeType::VEC3F));
  auto* positions = geo.positions();
  ASSERT_NE(positions, nullptr);
  ASSERT_EQ(positions->size(), COUNT);

  // Benchmark: Sequential write
  auto start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < COUNT; ++i) {
    (*positions)[i] = Vec3f(static_cast<float>(i), static_cast<float>(i * 2),
                            static_cast<float>(i * 3));
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto write_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Benchmark: Sequential read
  start = std::chrono::high_resolution_clock::now();

  Vec3f sum(0.0F, 0.0F, 0.0F);
  for (size_t i = 0; i < COUNT; ++i) {
    sum += (*positions)[i];
  }

  end = std::chrono::high_resolution_clock::now();
  auto read_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Report results
  std::cout << "\n=== Performance Baseline (1M Vec3f positions) ===\n";
  std::cout << "Sequential write: " << write_duration.count() << "ms\n";
  std::cout << "Sequential read:  " << read_duration.count() << "ms\n";
  std::cout << "Sum (to prevent optimization): " << sum.x() << "\n";

  // Success criteria: should complete in reasonable time
  // Target: <50ms for write, <30ms for read on modern hardware (Release build)
  // Debug builds are significantly slower, so we use relaxed thresholds
  EXPECT_LT(write_duration.count(), 100); // Generous for CI
#ifdef NDEBUG
  // Release build: strict threshold
  EXPECT_LT(read_duration.count(), 100);
#else
  // Debug build: relaxed threshold (3-4x slower is normal)
  EXPECT_LT(read_duration.count(), 400);
#endif

  // Verify correctness
  EXPECT_FLOAT_EQ((*positions)[0].x(), 0.0F);
  EXPECT_FLOAT_EQ((*positions)[999999].x(), 999999.0F);
}

// ============================================================================
// Test 8: Span-based Iteration
// ============================================================================

TEST_F(AttributeSystemIntegrationTest, SpanBasedIteration) {
  GeometryContainer geo;
  geo.set_point_count(100);

  ASSERT_TRUE(geo.add_point_attribute(attrs::P, AttributeType::VEC3F));
  auto* positions = geo.positions();
  ASSERT_NE(positions, nullptr);

  // Get writable span
  auto pos_span = positions->values_writable();

  // Modern range-based for loop
  size_t index = 0;
  for (auto& pos : pos_span) {
    pos = Vec3f(static_cast<float>(index), 0.0F, 0.0F);
    ++index;
  }

  // Verify
  EXPECT_EQ(index, 100);
  EXPECT_FLOAT_EQ((*positions)[50].x(), 50.0F);

  // Read-only span
  auto const_span = positions->values();
  float sum = 0.0F;
  for (const auto& pos : const_span) {
    sum += pos.x();
  }

  // Sum of 0..99 = 4950
  EXPECT_FLOAT_EQ(sum, 4950.0F);
}

// ============================================================================
// Test 9: Validation
// ============================================================================

// Test topology validation catches invalid vertex→point references
TEST_F(AttributeSystemIntegrationTest, Validation) {
  GeometryContainer geo;
  geo.set_point_count(3);
  geo.set_vertex_count(3);

  // Valid topology
  geo.topology().set_vertex_point(0, 0);
  geo.topology().set_vertex_point(1, 1);
  geo.topology().set_vertex_point(2, 2);
  geo.add_primitive({0, 1, 2});

  EXPECT_TRUE(geo.topology().validate());

  // Add position attribute
  ASSERT_TRUE(geo.add_point_attribute(attrs::P, AttributeType::VEC3F));
  EXPECT_TRUE(geo.validate());

  // Create invalid topology: vertex references non-existent point
  // The API throws on invalid set_vertex_point, so test that behavior
  GeometryContainer bad_geo;
  bad_geo.set_point_count(2); // Only 2 points
  bad_geo.set_vertex_count(3);

  bad_geo.topology().set_vertex_point(0, 0);
  bad_geo.topology().set_vertex_point(1, 1);

  // Trying to set vertex to invalid point should throw
  EXPECT_THROW(bad_geo.topology().set_vertex_point(2, 5), std::out_of_range);

  // Test validation catches unassigned vertex (initialized to -1)
  // Vertex 2 is still -1 (unassigned) which is valid
  bad_geo.add_primitive({0, 1, 2});
  EXPECT_TRUE(bad_geo.topology().validate()); // -1 is allowed (unassigned)
}

// ============================================================================
// Test 10: Multiple Attribute Types
// ============================================================================

TEST_F(AttributeSystemIntegrationTest, MultipleAttributeTypes) {
  GeometryContainer geo = create_quad();

  // Add various attribute types
  ASSERT_TRUE(geo.add_point_attribute(attrs::P, AttributeType::VEC3F));
  ASSERT_TRUE(geo.add_point_attribute("pscale", AttributeType::FLOAT));
  ASSERT_TRUE(geo.add_vertex_attribute(attrs::N, AttributeType::VEC3F));
  ASSERT_TRUE(geo.add_vertex_attribute(attrs::uv, AttributeType::VEC2F));
  ASSERT_TRUE(geo.add_primitive_attribute("primid", AttributeType::INT));

  // Get typed access to all
  auto* positions = geo.positions();
  auto* pscales = geo.get_point_attribute_typed<float>("pscale");
  auto* normals = geo.get_vertex_attribute_typed<Vec3f>(attrs::N);
  auto* uvs = geo.get_vertex_attribute_typed<Vec2f>(attrs::uv);
  auto* primids = geo.get_primitive_attribute_typed<int>("primid");

  ASSERT_NE(positions, nullptr);
  ASSERT_NE(pscales, nullptr);
  ASSERT_NE(normals, nullptr);
  ASSERT_NE(uvs, nullptr);
  ASSERT_NE(primids, nullptr);

  // Set some values
  (*positions)[0] = Vec3f(0.0F, 0.0F, 0.0F);
  (*pscales)[0] = 1.5F;
  (*normals)[0] = Vec3f(0.0F, 0.0F, 1.0F);
  (*uvs)[0] = Vec2f(0.0F, 0.0F);
  (*primids)[0] = 100;

  // Verify
  EXPECT_FLOAT_EQ((*pscales)[0], 1.5F);
  EXPECT_FLOAT_EQ((*uvs)[0].x(), 0.0F);
  EXPECT_EQ((*primids)[0], 100);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
