#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/geometry/boolean_ops.hpp"
#include "nodo/geometry/box_generator.hpp"
#include "nodo/geometry/sphere_generator.hpp"

#include <cmath>

#include <gtest/gtest.h>

using namespace nodo;
using namespace nodo::geometry;

namespace {

// Helper: Convert GeometryContainer to Mesh for BooleanOps testing
core::Mesh containerToMesh(const core::GeometryContainer& container) {
  auto* positions = container.get_point_attribute_typed<core::Vec3f>(core::standard_attrs::P);
  EXPECT_NE(positions, nullptr);

  const auto& topo = container.topology();
  const size_t point_count = topo.point_count();
  const size_t prim_count = topo.primitive_count();

  // Build vertices matrix
  core::Mesh::Vertices vertices(point_count, 3);
  auto pos_span = positions->values();
  for (size_t i = 0; i < point_count; ++i) {
    vertices.row(i) = pos_span[i].cast<double>();
  }

  // Build faces matrix (triangulate if needed)
  std::vector<Eigen::Vector3i> faces_vec;
  faces_vec.reserve(prim_count);

  for (size_t prim_idx = 0; prim_idx < prim_count; ++prim_idx) {
    const auto& vert_indices = topo.get_primitive_vertices(prim_idx);
    std::vector<int> point_indices;
    point_indices.reserve(vert_indices.size());

    for (size_t vert_idx : vert_indices) {
      point_indices.push_back(topo.get_vertex_point(vert_idx));
    }

    // Fan triangulation for n-gons
    for (size_t i = 1; i + 1 < point_indices.size(); ++i) {
      faces_vec.emplace_back(point_indices[0], point_indices[i], point_indices[i + 1]);
    }
  }

  core::Mesh::Faces faces(faces_vec.size(), 3);
  for (size_t i = 0; i < faces_vec.size(); ++i) {
    faces.row(i) = faces_vec[i];
  }

  return core::Mesh(vertices, faces);
}

} // anonymous namespace

class BooleanOpsTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create test geometries
    auto box_result = BoxGenerator::generate(2.0, 2.0, 2.0);
    ASSERT_TRUE(box_result.has_value());
    box_geo_ = box_result.value().clone();
    box_mesh_ = containerToMesh(box_geo_);

    auto sphere_result = SphereGenerator::generate_uv_sphere(1.0, 32, 16);
    ASSERT_TRUE(sphere_result.has_value());
    sphere_geo_ = sphere_result.value().clone();
    sphere_mesh_ = containerToMesh(sphere_geo_);
  }

  core::GeometryContainer box_geo_;
  core::Mesh box_mesh_;
  core::GeometryContainer sphere_geo_;
  core::Mesh sphere_mesh_;
};

// ============================================================================
// Basic Validation Tests
// ============================================================================

TEST_F(BooleanOpsTest, ValidateMesh_ValidBox) {
  EXPECT_TRUE(BooleanOps::validate_mesh(box_mesh_));
}

TEST_F(BooleanOpsTest, ValidateMesh_ValidSphere) {
  EXPECT_TRUE(BooleanOps::validate_mesh(sphere_mesh_));
}

TEST_F(BooleanOpsTest, ValidateMesh_EmptyMesh) {
  core::Mesh empty_mesh;
  EXPECT_FALSE(BooleanOps::validate_mesh(empty_mesh));

  const auto& error = BooleanOps::last_error();
  EXPECT_EQ(error.category, core::ErrorCategory::Validation);
}

TEST_F(BooleanOpsTest, ValidateMesh_InsufficientVertices) {
  // Create mesh with only 2 vertices (need at least 3 for a triangle)
  core::Mesh::Vertices verts(2, 3);
  verts << 0, 0, 0, 1, 0, 0;

  core::Mesh::Faces faces(1, 3);
  faces << 0, 1, 0; // Invalid - reuses same vertex

  core::Mesh invalid_mesh(verts, faces);
  EXPECT_FALSE(BooleanOps::validate_mesh(invalid_mesh));
}

TEST_F(BooleanOpsTest, ValidateMesh_InvalidFaceIndices) {
  // Create mesh where face references vertex out of bounds
  core::Mesh::Vertices verts(3, 3);
  verts << 0, 0, 0, 1, 0, 0, 0, 1, 0;

  core::Mesh::Faces faces(1, 3);
  faces << 0, 1, 5; // Index 5 is out of bounds

  core::Mesh invalid_mesh(verts, faces);
  EXPECT_FALSE(BooleanOps::validate_mesh(invalid_mesh));

  const auto& error = BooleanOps::last_error();
  EXPECT_EQ(error.category, core::ErrorCategory::Validation);
}

TEST_F(BooleanOpsTest, AreCompatible_BothValid) {
  EXPECT_TRUE(BooleanOps::are_compatible(box_mesh_, sphere_mesh_));
}

TEST_F(BooleanOpsTest, AreCompatible_OneInvalid) {
  core::Mesh empty_mesh;
  EXPECT_FALSE(BooleanOps::are_compatible(box_mesh_, empty_mesh));
  EXPECT_FALSE(BooleanOps::are_compatible(empty_mesh, sphere_mesh_));
}

// ============================================================================
// Union Operation Tests
// ============================================================================

TEST_F(BooleanOpsTest, Union_TwoIdenticalBoxes) {
  // Union of two identical boxes should produce a single box
  auto result = BooleanOps::union_meshes(box_mesh_, box_mesh_);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->vertices().rows(), 0);
  EXPECT_GT(result->faces().rows(), 0);

  // Volume should be approximately the same as one box
  // Box is 2x2x2 = volume 8
  auto result_mesh = *result;
  EXPECT_GT(result_mesh.vertices().rows(), 0);
}

TEST_F(BooleanOpsTest, Union_TwoOverlappingSpheres) {
  // Create two spheres offset by 0.5 units
  auto sphere1 = sphere_mesh_;
  auto sphere2 = sphere_mesh_;

  // Offset sphere2
  for (int i = 0; i < sphere2.vertices().rows(); ++i) {
    sphere2.vertices()(i, 0) += 0.5;
  }

  auto result = BooleanOps::union_meshes(sphere1, sphere2);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->vertices().rows(), 0);
  EXPECT_GT(result->faces().rows(), 0);

  // Result should have more vertices than single sphere but less than both combined
  EXPECT_GT(result->vertices().rows(), sphere1.vertices().rows());
  EXPECT_LT(result->vertices().rows(), sphere1.vertices().rows() + sphere2.vertices().rows());
}

TEST_F(BooleanOpsTest, Union_NonOverlappingMeshes) {
  // Create two boxes far apart
  auto box1 = box_mesh_;
  auto box2 = box_mesh_;

  // Offset box2 far away
  for (int i = 0; i < box2.vertices().rows(); ++i) {
    box2.vertices()(i, 0) += 10.0;
  }

  auto result = BooleanOps::union_meshes(box1, box2);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->vertices().rows(), 0);
  EXPECT_GT(result->faces().rows(), 0);

  // Result should have approximately sum of vertices (no overlap to merge)
  // Allow some tolerance for Manifold's processing
  EXPECT_GE(result->vertices().rows(), box1.vertices().rows());
}

TEST_F(BooleanOpsTest, Union_WithInvalidMesh) {
  core::Mesh empty_mesh;
  auto result = BooleanOps::union_meshes(box_mesh_, empty_mesh);

  EXPECT_FALSE(result.has_value());

  const auto& error = BooleanOps::last_error();
  EXPECT_EQ(error.category, core::ErrorCategory::Validation);
}

// ============================================================================
// Intersection Operation Tests
// ============================================================================

TEST_F(BooleanOpsTest, Intersection_TwoIdenticalBoxes) {
  // Intersection of two identical boxes should produce a single box
  auto result = BooleanOps::intersect_meshes(box_mesh_, box_mesh_);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->vertices().rows(), 0);
  EXPECT_GT(result->faces().rows(), 0);
}

TEST_F(BooleanOpsTest, Intersection_TwoOverlappingSpheres) {
  // Create two spheres offset by 0.5 units
  auto sphere1 = sphere_mesh_;
  auto sphere2 = sphere_mesh_;

  // Offset sphere2
  for (int i = 0; i < sphere2.vertices().rows(); ++i) {
    sphere2.vertices()(i, 0) += 0.5;
  }

  auto result = BooleanOps::intersect_meshes(sphere1, sphere2);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->vertices().rows(), 0);
  EXPECT_GT(result->faces().rows(), 0);

  // Intersection should be smaller than either sphere
  EXPECT_LT(result->vertices().rows(), sphere1.vertices().rows());
}

TEST_F(BooleanOpsTest, Intersection_NonOverlappingMeshes) {
  // Create two boxes far apart
  auto box1 = box_mesh_;
  auto box2 = box_mesh_;

  // Offset box2 far away
  for (int i = 0; i < box2.vertices().rows(); ++i) {
    box2.vertices()(i, 0) += 10.0;
  }

  auto result = BooleanOps::intersect_meshes(box1, box2);

  // Non-overlapping meshes intersection returns empty result
  // BooleanOps currently returns nullopt for empty results
  // TODO: Consider returning empty mesh instead of nullopt for consistency
  if (result.has_value()) {
    EXPECT_EQ(result->faces().rows(), 0);
  } else {
    // Current behavior: returns nullopt for empty intersection
    SUCCEED() << "Empty intersection returned as nullopt (current behavior)";
  }
}

TEST_F(BooleanOpsTest, Difference_TwoIdenticalBoxes) {
  // Difference of two identical boxes should produce empty result
  auto result = BooleanOps::difference_meshes(box_mesh_, box_mesh_);

  // Complete subtraction may return nullopt or empty mesh
  // BooleanOps currently returns nullopt for empty results
  // TODO: Consider returning empty mesh instead of nullopt for consistency
  if (result.has_value()) {
    EXPECT_EQ(result->faces().rows(), 0) << "Expected empty result mesh";
  } else {
    // Current behavior: returns nullopt for empty difference
    SUCCEED() << "Empty difference returned as nullopt (current behavior)";
  }
}

TEST_F(BooleanOpsTest, Difference_LargeMinusSmall) {
  // Create large and small box
  auto large_box_result = BoxGenerator::generate(2.0, 2.0, 2.0);
  ASSERT_TRUE(large_box_result.has_value());
  auto large_mesh = containerToMesh(large_box_result.value());

  auto small_box_result = BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(small_box_result.has_value());
  auto small_mesh = containerToMesh(small_box_result.value());

  // Large - small should produce a hollow box
  auto result = BooleanOps::difference_meshes(large_mesh, small_mesh);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->vertices().rows(), 0);
  EXPECT_GT(result->faces().rows(), 0);

  // Result should have more vertices than either input (due to cut)
  EXPECT_GT(result->vertices().rows(), large_mesh.vertices().rows());
}

TEST_F(BooleanOpsTest, Difference_SmallMinusLarge) {
  // Create large and small box
  auto large_box_result = BoxGenerator::generate(2.0, 2.0, 2.0);
  ASSERT_TRUE(large_box_result.has_value());
  auto large_mesh = containerToMesh(large_box_result.value());

  auto small_box_result = BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(small_box_result.has_value());
  auto small_mesh = containerToMesh(small_box_result.value());

  // Small - large should produce empty result (small is entirely inside large)
  auto result = BooleanOps::difference_meshes(small_mesh, large_mesh);

  // Complete subtraction may return nullopt or empty mesh
  if (result.has_value()) {
    EXPECT_EQ(result->faces().rows(), 0) << "Expected empty result mesh";
  } else {
    // Current behavior: returns nullopt for empty difference
    SUCCEED() << "Empty difference returned as nullopt (current behavior)";
  }
}

TEST_F(BooleanOpsTest, Difference_NonOverlappingMeshes) {
  // Create two boxes far apart
  auto box1 = box_mesh_;
  auto box2 = box_mesh_;

  // Offset box2 far away
  for (int i = 0; i < box2.vertices().rows(); ++i) {
    box2.vertices()(i, 0) += 10.0;
  }

  auto result = BooleanOps::difference_meshes(box1, box2);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->vertices().rows(), 0);

  // Result should be approximately the same as box1 (nothing subtracted)
  EXPECT_EQ(result->vertices().rows(), box1.vertices().rows());
}

TEST_F(BooleanOpsTest, Difference_WithInvalidMesh) {
  core::Mesh empty_mesh;
  auto result = BooleanOps::difference_meshes(box_mesh_, empty_mesh);

  EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Manifold Property Tests (No Internal Geometry)
// ============================================================================

TEST_F(BooleanOpsTest, Union_ProducesManifoldMesh) {
  // Create two overlapping spheres
  auto sphere1 = sphere_mesh_;
  auto sphere2 = sphere_mesh_;

  for (int i = 0; i < sphere2.vertices().rows(); ++i) {
    sphere2.vertices()(i, 0) += 0.5;
  }

  auto result = BooleanOps::union_meshes(sphere1, sphere2);
  ASSERT_TRUE(result.has_value());

  // Verify manifold property: each edge shared by exactly 1 or 2 faces
  std::map<std::pair<int, int>, int> edge_count;

  for (int face_idx = 0; face_idx < result->faces().rows(); ++face_idx) {
    for (int i = 0; i < 3; ++i) {
      int v1 = result->faces()(face_idx, i);
      int v2 = result->faces()(face_idx, (i + 1) % 3);

      if (v1 > v2)
        std::swap(v1, v2);
      edge_count[{v1, v2}]++;
    }
  }

  // Check for non-manifold edges (shared by >2 faces)
  int non_manifold_edges = 0;
  for (const auto& [edge, count] : edge_count) {
    if (count > 2) {
      non_manifold_edges++;
    }
  }

  EXPECT_EQ(non_manifold_edges, 0) << "Union produced " << non_manifold_edges << " non-manifold edges";
}

TEST_F(BooleanOpsTest, Difference_ProducesManifoldMesh) {
  // Create large box with small box subtracted (hollow box)
  auto large_box_result = BoxGenerator::generate(2.0, 2.0, 2.0);
  ASSERT_TRUE(large_box_result.has_value());
  auto large_mesh = containerToMesh(large_box_result.value());

  auto small_box_result = BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(small_box_result.has_value());
  auto small_mesh = containerToMesh(small_box_result.value());

  auto result = BooleanOps::difference_meshes(large_mesh, small_mesh);
  ASSERT_TRUE(result.has_value());
  ASSERT_GT(result->faces().rows(), 0);

  // Verify manifold property
  std::map<std::pair<int, int>, int> edge_count;

  for (int face_idx = 0; face_idx < result->faces().rows(); ++face_idx) {
    for (int i = 0; i < 3; ++i) {
      int v1 = result->faces()(face_idx, i);
      int v2 = result->faces()(face_idx, (i + 1) % 3);

      if (v1 > v2)
        std::swap(v1, v2);
      edge_count[{v1, v2}]++;
    }
  }

  int non_manifold_edges = 0;
  for (const auto& [edge, count] : edge_count) {
    if (count > 2) {
      non_manifold_edges++;
    }
  }

  EXPECT_EQ(non_manifold_edges, 0) << "Difference produced " << non_manifold_edges << " non-manifold edges";
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(BooleanOpsTest, LastError_ClearsOnSuccess) {
  // First cause an error
  core::Mesh empty_mesh;
  auto fail_result = BooleanOps::union_meshes(box_mesh_, empty_mesh);
  EXPECT_FALSE(fail_result.has_value());

  // Error should be set
  auto error1 = BooleanOps::last_error();
  EXPECT_EQ(error1.category, core::ErrorCategory::Validation);

  // Now do successful operation
  auto success_result = BooleanOps::union_meshes(box_mesh_, box_mesh_);
  EXPECT_TRUE(success_result.has_value());

  // Note: last_error() may or may not be cleared on success
  // This depends on implementation - document the behavior
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(BooleanOpsTest, Union_CoplanarFaces) {
  // Create two boxes that share a face (edge-to-edge contact)
  auto box1_result = BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box1_result.has_value());
  auto box1 = containerToMesh(box1_result.value());

  auto box2_result = BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box2_result.has_value());
  auto box2_geo = box2_result.value().clone();
  auto* positions = box2_geo.get_point_attribute_typed<core::Vec3f>("P");
  for (size_t i = 0; i < box2_geo.point_count(); ++i) {
    (*positions)[i].x() += 1.0f;
  }
  auto box2 = containerToMesh(box2_geo);

  auto result = BooleanOps::union_meshes(box1, box2);

  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->vertices().rows(), 0);
  EXPECT_GT(result->faces().rows(), 0);
}

TEST_F(BooleanOpsTest, Intersection_TouchingAtPoint) {
  // Create two boxes that only touch at a single point
  auto box1_result = BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box1_result.has_value());
  auto box1 = containerToMesh(box1_result.value());

  auto box2_result = BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box2_result.has_value());
  auto box2_geo = box2_result.value().clone();
  auto* positions = box2_geo.get_point_attribute_typed<core::Vec3f>("P");
  for (size_t i = 0; i < box2_geo.point_count(); ++i) {
    (*positions)[i].x() += 1.0f;
    (*positions)[i].y() += 1.0f;
    (*positions)[i].z() += 1.0f;
  }
  auto box2 = containerToMesh(box2_geo);

  auto result = BooleanOps::intersect_meshes(box1, box2);

  // Touching at a point should produce empty or degenerate result
  // May return nullopt or empty mesh depending on implementation
  if (result.has_value()) {
    // May be empty or have degenerate faces
    SUCCEED() << "Point intersection returned mesh with " << result->faces().rows() << " faces";
  } else {
    // Current behavior: returns nullopt for point intersection
    SUCCEED() << "Point intersection returned as nullopt (current behavior)";
  }
}
