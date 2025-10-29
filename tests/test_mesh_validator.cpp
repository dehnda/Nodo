#include <gtest/gtest.h>
#include <nodo/geometry/box_generator.hpp>
#include <nodo/geometry/mesh_validator.hpp>
#include <nodo/geometry/sphere_generator.hpp>

using namespace nodo;

// Helper to convert GeometryContainer to Mesh
// Handles both triangles and quads
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
  // Mesh class requires triangles, so triangulate quads if needed
  std::vector<Eigen::Vector3i> triangle_list;
  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &vert_indices = topology.get_primitive_vertices(prim_idx);

    // Convert vertex indices to point indices
    std::vector<int> point_indices;
    for (int vert_idx : vert_indices) {
      point_indices.push_back(topology.get_vertex_point(vert_idx));
    }

    if (point_indices.size() == 3) {
      // Triangle - add directly
      triangle_list.emplace_back(point_indices[0], point_indices[1],
                                 point_indices[2]);
    } else if (point_indices.size() == 4) {
      // Quad - triangulate (fan from first vertex)
      triangle_list.emplace_back(point_indices[0], point_indices[1],
                                 point_indices[2]);
      triangle_list.emplace_back(point_indices[0], point_indices[2],
                                 point_indices[3]);
    }
  }

  // Convert to Eigen matrix
  Eigen::MatrixXi faces(triangle_list.size(), 3);
  for (size_t i = 0; i < triangle_list.size(); ++i) {
    faces.row(i) = triangle_list[i];
  }

  return core::Mesh(vertices, faces);
}

class MeshValidatorTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a clean box mesh
    auto box_result = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
    ASSERT_TRUE(box_result.has_value());
    clean_mesh_ = container_to_mesh(*box_result);

    // Create a problematic mesh
    create_problematic_mesh();
  }

  void create_problematic_mesh() {
    Eigen::MatrixXd vertices(6, 3);
    vertices << 0.0, 0.0, 0.0, // v0
        1.0, 0.0, 0.0,         // v1
        0.5, 1.0, 0.0,         // v2
        0.0, 0.0, 0.0,         // v3 (duplicate of v0)
        2.0, 0.0, 0.0,         // v4 (unreferenced)
        1.0, 0.0, 0.0;         // v5 (duplicate of v1)

    Eigen::MatrixXi faces(3, 3);
    faces << 0, 1, 2, // valid triangle
        0, 1, 1,      // degenerate triangle (duplicate vertex)
        3, 5, 2;      // triangle using duplicate vertices

    problematic_mesh_.vertices() = vertices;
    problematic_mesh_.faces() = faces;
  }

  core::Mesh clean_mesh_;
  core::Mesh problematic_mesh_;
};

TEST_F(MeshValidatorTest, ValidMeshValidation) {
  auto report = geometry::MeshValidator::validate(clean_mesh_);

  EXPECT_GT(report.num_vertices, 0);
  EXPECT_GT(report.num_faces, 0);
  EXPECT_GT(report.num_edges, 0);
  EXPECT_TRUE(report.is_manifold);
  EXPECT_FALSE(report.has_degenerate_faces);
}

TEST_F(MeshValidatorTest, ProblematicMeshValidation) {
  auto report = geometry::MeshValidator::validate(problematic_mesh_);

  EXPECT_FALSE(report.is_valid);
  EXPECT_TRUE(report.has_degenerate_faces);
  EXPECT_TRUE(report.has_duplicate_vertices);
  EXPECT_TRUE(report.has_unreferenced_vertices);
  EXPECT_FALSE(report.is_manifold);

  EXPECT_EQ(report.degenerate_face_indices.size(), 1);
  EXPECT_EQ(report.duplicate_vertex_indices.size(), 2);
  EXPECT_EQ(report.unreferenced_vertex_indices.size(), 1);
}

TEST_F(MeshValidatorTest, DegenerateFaceDetection) {
  auto degenerate_faces =
      geometry::MeshValidator::find_degenerate_faces(problematic_mesh_);

  ASSERT_EQ(degenerate_faces.size(), 1);
  EXPECT_EQ(degenerate_faces[0], 1); // Face with indices [0, 1, 1]
}

TEST_F(MeshValidatorTest, DuplicateVertexDetection) {
  auto duplicates =
      geometry::MeshValidator::find_duplicate_vertices(problematic_mesh_);

  ASSERT_EQ(duplicates.size(), 2);
  // Should find vertices 3 and 5 as duplicates
  EXPECT_EQ(duplicates[0], 3);
  EXPECT_EQ(duplicates[1], 5);
}

TEST_F(MeshValidatorTest, UnreferencedVertexDetection) {
  auto unreferenced =
      geometry::MeshValidator::find_unreferenced_vertices(problematic_mesh_);

  ASSERT_EQ(unreferenced.size(), 1);
  EXPECT_EQ(unreferenced[0], 4); // Vertex 4 is not used in any face
}

TEST_F(MeshValidatorTest, ManifoldChecking) {
  // Clean mesh should be manifold
  EXPECT_TRUE(geometry::MeshValidator::is_manifold(clean_mesh_));

  // Problematic mesh should not be manifold
  EXPECT_FALSE(geometry::MeshValidator::is_manifold(problematic_mesh_));
}

TEST_F(MeshValidatorTest, ClosedMeshChecking) {
  // Generate a sphere which should be closed
  auto sphere_result = geometry::SphereGenerator::generate_uv_sphere(1.0, 8, 8);
  ASSERT_TRUE(sphere_result.has_value());
  auto sphere_mesh = container_to_mesh(*sphere_result);

  EXPECT_TRUE(geometry::MeshValidator::is_closed(sphere_mesh));
}

TEST_F(MeshValidatorTest, EmptyMeshValidation) {
  core::Mesh empty_mesh;
  auto report = geometry::MeshValidator::validate(empty_mesh);

  EXPECT_FALSE(report.is_valid);
  EXPECT_EQ(report.num_vertices, 0);
  EXPECT_EQ(report.num_faces, 0);
  EXPECT_EQ(report.num_edges, 0);
}

TEST_F(MeshValidatorTest, ValidationReportSummary) {
  auto report = geometry::MeshValidator::validate(problematic_mesh_);
  std::string summary = report.summary();

  EXPECT_FALSE(summary.empty());
  EXPECT_NE(summary.find("Valid: NO"), std::string::npos);
  EXPECT_NE(summary.find("Has Degenerate Faces: YES"), std::string::npos);
  EXPECT_NE(summary.find("Has Duplicate Vertices: YES"), std::string::npos);
}

TEST_F(MeshValidatorTest, ToleranceSettings) {
  // Test duplicate detection with different tolerances
  auto duplicates_strict = geometry::MeshValidator::find_duplicate_vertices(
      problematic_mesh_, 1e-12);
  auto duplicates_loose =
      geometry::MeshValidator::find_duplicate_vertices(problematic_mesh_, 1e-6);

  // Should get same results for exact duplicates regardless of tolerance
  EXPECT_EQ(duplicates_strict.size(), duplicates_loose.size());
}
