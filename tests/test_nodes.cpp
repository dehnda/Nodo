#include <gtest/gtest.h>
#include <nodo/geometry/box_generator.hpp>
#include <nodo/geometry/cylinder_generator.hpp>
#include <nodo/geometry/plane_generator.hpp>
#include <nodo/geometry/sphere_generator.hpp>
#include <nodo/geometry/torus_generator.hpp>


using namespace nodeflux;

// Helper to convert GeometryContainer to Mesh
// Helper to convert GeometryContainer to Mesh
static core::Mesh container_to_mesh(const core::GeometryContainer &container) {
  const auto &topology = container.topology();

  // Extract positions
  auto *p_storage =
      container.get_point_attribute_typed<core::Vec3f>(core::standard_attrs::P);
  if (!p_storage)
    return core::Mesh();

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

  return core::Mesh(vertices, faces);
}

class PrimitiveGeneratorTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Basic setup for geometry generator testing
  }
};

// Box Generator Tests
TEST_F(PrimitiveGeneratorTest, BoxGeneration) {
  auto container_result = geometry::BoxGenerator::generate(2.0, 1.5, 0.8);

  ASSERT_TRUE(container_result.has_value());
  auto mesh = container_to_mesh(*container_result);
  EXPECT_GT(mesh.vertices().rows(), 0);
  EXPECT_GT(mesh.faces().rows(), 0);
}

TEST_F(PrimitiveGeneratorTest, BoxWithDifferentDimensions) {
  auto container_result = geometry::BoxGenerator::generate(3.0, 2.0, 1.5);

  ASSERT_TRUE(container_result.has_value());
  auto mesh = container_to_mesh(*container_result);
  EXPECT_GT(mesh.vertices().rows(), 0);
  EXPECT_GT(mesh.faces().rows(), 0);
}

// Sphere Generator Tests
TEST_F(PrimitiveGeneratorTest, SphereGeneration) {
  auto container_result =
      geometry::SphereGenerator::generate_uv_sphere(1.5, 16, 8);

  ASSERT_TRUE(container_result.has_value());
  auto mesh = container_to_mesh(*container_result);
  EXPECT_GT(mesh.vertices().rows(), 0);
  EXPECT_GT(mesh.faces().rows(), 0);
}

// Cylinder Generator Tests
TEST_F(PrimitiveGeneratorTest, CylinderGeneration) {
  auto container_result = geometry::CylinderGenerator::generate(0.5, 2.0, 12);

  ASSERT_TRUE(container_result.has_value());
  auto mesh = container_to_mesh(*container_result);
  EXPECT_GT(mesh.vertices().rows(), 0);
  EXPECT_GT(mesh.faces().rows(), 0);
}

// Plane Generator Tests
TEST_F(PrimitiveGeneratorTest, PlaneGeneration) {
  auto container_result = geometry::PlaneGenerator::generate(2.0, 1.5, 4, 3);

  ASSERT_TRUE(container_result.has_value());
  auto mesh = container_to_mesh(*container_result);
  EXPECT_GT(mesh.vertices().rows(), 0);
  EXPECT_GT(mesh.faces().rows(), 0);
}

// Torus Generator Tests
TEST_F(PrimitiveGeneratorTest, TorusGeneration) {
  auto container_result = geometry::TorusGenerator::generate(1.0, 0.3, 24, 12);

  ASSERT_TRUE(container_result.has_value());
  auto mesh = container_to_mesh(*container_result);
  EXPECT_GT(mesh.vertices().rows(), 0);
  EXPECT_GT(mesh.faces().rows(), 0);
}

TEST_F(PrimitiveGeneratorTest, TorusWithDifferentParameters) {
  auto container_result = geometry::TorusGenerator::generate(2.0, 0.5, 48, 16);

  ASSERT_TRUE(container_result.has_value());
  auto mesh = container_to_mesh(*container_result);
  EXPECT_GT(mesh.vertices().rows(), 0);
  EXPECT_GT(mesh.faces().rows(), 0);
}
