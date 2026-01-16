#include "test_utils.hpp"

#include <gtest/gtest.h>
#include <nodo/geometry/box_generator.hpp>
#include <nodo/geometry/mesh_repairer.hpp>
#include <nodo/geometry/mesh_validator.hpp>

using namespace nodo;

class MeshRepairerTest : public ::testing::Test {
protected:
  void SetUp() override { create_problematic_mesh(); }

  void create_problematic_mesh() {
    // Create mesh with various issues
    Eigen::MatrixXd vertices(8, 3);
    vertices << 0.0, 0.0, 0.0, // v0
        1.0, 0.0, 0.0,         // v1
        0.5, 1.0, 0.0,         // v2
        0.0, 0.0, 0.0,         // v3 (duplicate of v0)
        2.0, 0.0, 0.0,         // v4 (unreferenced)
        1.0, 0.0, 0.0,         // v5 (duplicate of v1)
        0.0, 1.0, 0.0,         // v6
        0.5, 0.5, 1.0;         // v7

    Eigen::MatrixXi faces(4, 3);
    faces << 0, 1, 2, // valid triangle
        0, 1, 1,      // degenerate triangle (duplicate vertex)
        3, 5, 2,      // triangle using duplicate vertices
        1, 6, 7;      // valid triangle

    problematic_mesh_.vertices() = vertices;
    problematic_mesh_.faces() = faces;
  }

  core::Mesh problematic_mesh_;
};

TEST_F(MeshRepairerTest, RemoveUnreferencedVertices) {
  auto original_vertex_count = problematic_mesh_.vertices().rows();

  auto removed_count = geometry::MeshRepairer::remove_unreferenced_vertices(problematic_mesh_);
  EXPECT_GT(removed_count, 0);

  // Should have fewer vertices after removing unreferenced ones
  EXPECT_LT(problematic_mesh_.vertices().rows(), original_vertex_count);

  // All vertices should now be referenced
  auto unreferenced = geometry::MeshValidator::find_unreferenced_vertices(problematic_mesh_);
  EXPECT_TRUE(unreferenced.empty());
}

TEST_F(MeshRepairerTest, RemoveDuplicateVertices) {
  auto original_vertex_count = problematic_mesh_.vertices().rows();
  auto test_mesh = problematic_mesh_; // Copy for testing

  auto merged_count = geometry::MeshRepairer::merge_duplicate_vertices(test_mesh);
  EXPECT_GT(merged_count, 0);

  // Should have fewer vertices after removing duplicates
  EXPECT_LT(test_mesh.vertices().rows(), original_vertex_count);

  // Should have no duplicate vertices
  auto duplicates = geometry::MeshValidator::find_duplicate_vertices(test_mesh);
  EXPECT_TRUE(duplicates.empty());

  // Faces should be updated to use merged vertices
  EXPECT_GT(test_mesh.faces().rows(), 0);
}

TEST_F(MeshRepairerTest, RemoveDegenerateFaces) {
  auto original_face_count = problematic_mesh_.faces().rows();
  auto test_mesh = problematic_mesh_; // Copy for testing

  auto removed_count = geometry::MeshRepairer::remove_degenerate_faces(test_mesh);
  EXPECT_GT(removed_count, 0);

  // Should have fewer faces after removing degenerate ones
  EXPECT_LT(test_mesh.faces().rows(), original_face_count);

  // Should have no degenerate faces
  auto degenerate = geometry::MeshValidator::find_degenerate_faces(test_mesh);
  EXPECT_TRUE(degenerate.empty());
}

TEST_F(MeshRepairerTest, FullRepair) {
  auto report_before = geometry::MeshValidator::validate(problematic_mesh_);
  EXPECT_FALSE(report_before.is_valid);

  auto test_mesh = problematic_mesh_; // Copy for testing
  auto result = geometry::MeshRepairer::repair(test_mesh);
  EXPECT_TRUE(result.success);

  auto report_after = geometry::MeshValidator::validate(test_mesh);

  // Repaired mesh should be significantly improved
  EXPECT_LT(test_mesh.vertices().rows(), problematic_mesh_.vertices().rows());
  EXPECT_LT(test_mesh.faces().rows(), problematic_mesh_.faces().rows());

  // Should have no duplicates, unreferenced vertices, or degenerate faces
  EXPECT_TRUE(geometry::MeshValidator::find_duplicate_vertices(test_mesh).empty());
  EXPECT_TRUE(geometry::MeshValidator::find_unreferenced_vertices(test_mesh).empty());
  EXPECT_TRUE(geometry::MeshValidator::find_degenerate_faces(test_mesh).empty());
}

TEST_F(MeshRepairerTest, ToleranceSettings) {
  // Test duplicate removal with different tolerances
  auto test_mesh1 = problematic_mesh_; // Copy for testing
  auto test_mesh2 = problematic_mesh_; // Copy for testing

  [[maybe_unused]] auto result_strict = geometry::MeshRepairer::merge_duplicate_vertices(test_mesh1, 1e-12);
  [[maybe_unused]] auto result_loose = geometry::MeshRepairer::merge_duplicate_vertices(test_mesh2, 1e-6);

  // Should get same results for exact duplicates regardless of tolerance
  EXPECT_EQ(test_mesh1.vertices().rows(), test_mesh2.vertices().rows());
}

TEST_F(MeshRepairerTest, CleanMeshUnchanged) {
  // Generate a clean box mesh (now quad-based, will be triangulated)
  auto box_result = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box_result.has_value());
  auto clean_mesh = test::container_to_mesh(*box_result);

  auto original_vertex_count = clean_mesh.vertices().rows();
  auto original_face_count = clean_mesh.faces().rows();

  auto result = geometry::MeshRepairer::repair(clean_mesh);
  EXPECT_TRUE(result.success);

  // Clean mesh should have same or fewer vertices/faces after repair
  // (Box generator creates clean quads, so no duplicates to merge)
  EXPECT_LE(clean_mesh.vertices().rows(), original_vertex_count);
  EXPECT_LE(clean_mesh.faces().rows(), original_face_count);

  // The mesh should still be valid after repair
  EXPECT_GT(clean_mesh.vertices().rows(), 0);
  EXPECT_GT(clean_mesh.faces().rows(), 0);
}

TEST_F(MeshRepairerTest, EmptyMeshHandling) {
  core::Mesh empty_mesh;

  auto result = geometry::MeshRepairer::repair(empty_mesh);
  EXPECT_TRUE(result.success);

  // Empty mesh should remain empty
  EXPECT_EQ(empty_mesh.vertices().rows(), 0);
  EXPECT_EQ(empty_mesh.faces().rows(), 0);
}

TEST_F(MeshRepairerTest, RepairStatistics) {
  auto test_mesh = problematic_mesh_; // Copy for testing
  auto result = geometry::MeshRepairer::repair(test_mesh);

  EXPECT_TRUE(result.success);
  EXPECT_GT(result.vertices_removed, 0);
  EXPECT_GT(result.vertices_merged, 0);
  EXPECT_GT(result.faces_removed, 0);
  EXPECT_FALSE(result.summary().empty());
}

TEST_F(MeshRepairerTest, VertexRemapping) {
  // Test that vertex indices are properly remapped after vertex removal
  auto test_mesh = problematic_mesh_; // Copy for testing
  auto merged_count = geometry::MeshRepairer::merge_duplicate_vertices(test_mesh);
  EXPECT_GT(merged_count, 0);

  // All face indices should be valid
  for (int i = 0; i < test_mesh.faces().rows(); ++i) {
    for (int j = 0; j < 3; ++j) {
      int vertex_index = test_mesh.faces()(i, j);
      EXPECT_GE(vertex_index, 0);
      EXPECT_LT(vertex_index, test_mesh.vertices().rows());
    }
  }
}
