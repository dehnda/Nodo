#include <gtest/gtest.h>
#include <nodo/geometry/box_generator.hpp>
#include <nodo/geometry/sphere_generator.hpp>
#include <nodo/spatial/bvh.hpp>


using namespace nodeflux;

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

class BVHTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a simple sphere for testing
    auto sphere_result = geometry::SphereGenerator::generate_icosphere(1.0, 2);
    ASSERT_TRUE(sphere_result.has_value());
    sphere_mesh_ = container_to_mesh(*sphere_result);

    // Create a simple box for testing
    auto box_result = geometry::BoxGenerator::generate(2.0, 2.0, 2.0);
    ASSERT_TRUE(box_result.has_value());
    box_mesh_ = container_to_mesh(*box_result);
  }

  core::Mesh sphere_mesh_;
  core::Mesh box_mesh_;
};

TEST_F(BVHTest, AABBConstruction) {
  // Test AABB from mesh
  auto aabb = spatial::AABB::from_mesh(sphere_mesh_);

  EXPECT_TRUE(aabb.is_valid());
  EXPECT_GT(aabb.volume(), 0.0);
  EXPECT_GT(aabb.surface_area(), 0.0);

  // Check that sphere is roughly contained
  auto center = aabb.center();
  auto size = aabb.size();

  // For a unit sphere, AABB should be approximately 2x2x2
  EXPECT_NEAR(size.x(), 2.0, 0.5);
  EXPECT_NEAR(size.y(), 2.0, 0.5);
  EXPECT_NEAR(size.z(), 2.0, 0.5);
}

TEST_F(BVHTest, AABBIntersection) {
  auto sphere_aabb = spatial::AABB::from_mesh(sphere_mesh_);
  auto box_aabb = spatial::AABB::from_mesh(box_mesh_);

  // Both centered at origin, should intersect
  EXPECT_TRUE(sphere_aabb.intersects(box_aabb));
  EXPECT_TRUE(box_aabb.intersects(sphere_aabb));

  // Test point containment
  EXPECT_TRUE(sphere_aabb.contains(Eigen::Vector3d(0, 0, 0)));
  EXPECT_TRUE(box_aabb.contains(Eigen::Vector3d(0, 0, 0)));
}

TEST_F(BVHTest, BVHConstruction) {
  spatial::BVH bvh;
  spatial::BVH::BuildParams params;

  bool success = bvh.build(sphere_mesh_, params);
  EXPECT_TRUE(success);
  EXPECT_TRUE(bvh.is_built());

  // Check statistics
  const auto &stats = bvh.stats();
  EXPECT_GT(stats.total_nodes, 0);
  EXPECT_GT(stats.leaf_nodes, 0);
  EXPECT_GE(stats.max_depth, 0);
  EXPECT_GT(stats.build_time_ms, 0.0);
}

TEST_F(BVHTest, BVHRayIntersection) {
  spatial::BVH bvh;
  spatial::BVH::BuildParams params;

  bool success = bvh.build(sphere_mesh_, params);
  ASSERT_TRUE(success);

  // Ray through center should hit
  spatial::BVH::Ray ray(Eigen::Vector3d(-2, 0, 0), Eigen::Vector3d(1, 0, 0));
  auto hit = bvh.intersect_ray(ray);

  EXPECT_TRUE(hit.has_value());
  if (hit) {
    EXPECT_GT(hit->t, 0.0);
    EXPECT_GE(hit->triangle_index, 0);
    EXPECT_LT(hit->triangle_index, sphere_mesh_.faces().rows());
  }

  // Ray missing sphere should not hit
  spatial::BVH::Ray miss_ray(Eigen::Vector3d(-2, 10, 0),
                             Eigen::Vector3d(1, 0, 0));
  auto miss_hit = bvh.intersect_ray(miss_ray);
  EXPECT_FALSE(miss_hit.has_value());
}

TEST_F(BVHTest, BVHAABBQuery) {
  spatial::BVH bvh;
  spatial::BVH::BuildParams params;

  bool success = bvh.build(sphere_mesh_, params);
  ASSERT_TRUE(success);

  // Query with sphere's bounding box should return triangles
  auto sphere_aabb = spatial::AABB::from_mesh(sphere_mesh_);
  auto triangles = bvh.query_aabb(sphere_aabb);

  EXPECT_GT(triangles.size(), 0);

  // All returned triangle indices should be valid
  for (int tri_idx : triangles) {
    EXPECT_GE(tri_idx, 0);
    EXPECT_LT(tri_idx, sphere_mesh_.faces().rows());
  }

  // Query with empty AABB should return fewer or no triangles
  spatial::AABB empty_aabb(Eigen::Vector3d(10, 10, 10),
                           Eigen::Vector3d(11, 11, 11));
  auto empty_triangles = bvh.query_aabb(empty_aabb);
  EXPECT_LE(empty_triangles.size(), triangles.size());
}

TEST_F(BVHTest, BVHClosestPoint) {
  spatial::BVH bvh;
  spatial::BVH::BuildParams params;

  bool success = bvh.build(sphere_mesh_, params);
  ASSERT_TRUE(success);

  // Find closest point to origin
  Eigen::Vector3d query_point(0, 0, 0);
  auto closest = bvh.closest_point(query_point);

  EXPECT_TRUE(closest.has_value());
  if (closest) {
    EXPECT_GE(closest->second, 0); // Valid triangle index
    EXPECT_LT(closest->second, sphere_mesh_.faces().rows());
  }
}

TEST_F(BVHTest, EmptyMeshHandling) {
  spatial::BVH bvh;
  spatial::BVH::BuildParams params;

  core::Mesh empty_mesh;
  bool success = bvh.build(empty_mesh, params);
  EXPECT_FALSE(success);
  EXPECT_FALSE(bvh.is_built());
}

TEST_F(BVHTest, BuildParameters) {
  spatial::BVH bvh;

  // Test with different parameters
  spatial::BVH::BuildParams params;
  params.max_triangles_per_leaf = 5;
  params.max_depth = 10;
  params.use_sah = false;

  bool success = bvh.build(sphere_mesh_, params);
  EXPECT_TRUE(success);

  const auto &stats = bvh.stats();
  EXPECT_LE(stats.max_depth, 10);
}
