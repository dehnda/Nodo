#include "nodeflux/geometry/sphere_generator.hpp"
#include "nodeflux/io/obj_exporter.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>


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

class ObjExportTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a simple sphere for testing
    auto container_result = geometry::SphereGenerator::generate_uv_sphere(1.0, 8, 8);
    ASSERT_TRUE(container_result.has_value());
    test_mesh_ = container_to_mesh(*container_result);
  }

  core::Mesh test_mesh_;
};

TEST_F(ObjExportTest, ExportToString) {
  auto obj_string = io::ObjExporter::mesh_to_obj_string(test_mesh_);

  ASSERT_TRUE(obj_string.has_value());
  EXPECT_FALSE(obj_string->empty());
}

TEST_F(ObjExportTest, ContainsVertexPositions) {
  auto obj_string = io::ObjExporter::mesh_to_obj_string(test_mesh_);

  ASSERT_TRUE(obj_string.has_value());

  // Check that it contains vertex lines (v x y z)
  EXPECT_NE(obj_string->find("v "), std::string::npos);

  // Count vertex lines
  std::istringstream stream(*obj_string);
  std::string line;
  int vertex_count = 0;

  while (std::getline(stream, line)) {
    if (line.length() >= 2 && line[0] == 'v' && line[1] == ' ') {
      vertex_count++;
    }
  }

  EXPECT_EQ(vertex_count, test_mesh_.vertex_count());
}

TEST_F(ObjExportTest, ContainsVertexNormals) {
  auto obj_string = io::ObjExporter::mesh_to_obj_string(test_mesh_);

  ASSERT_TRUE(obj_string.has_value());

  // Check that it contains normal lines (vn x y z)
  EXPECT_NE(obj_string->find("vn "), std::string::npos);

  // Count normal lines
  std::istringstream stream(*obj_string);
  std::string line;
  int normal_count = 0;

  while (std::getline(stream, line)) {
    if (line.length() >= 3 && line[0] == 'v' && line[1] == 'n' &&
        line[2] == ' ') {
      normal_count++;
    }
  }

  EXPECT_EQ(normal_count, test_mesh_.vertex_count());
}

TEST_F(ObjExportTest, ContainsFacesWithNormals) {
  auto obj_string = io::ObjExporter::mesh_to_obj_string(test_mesh_);

  ASSERT_TRUE(obj_string.has_value());

  // Check that faces use the format: f v//vn v//vn v//vn
  EXPECT_NE(obj_string->find("f "), std::string::npos);
  EXPECT_NE(obj_string->find("//"), std::string::npos);

  // Count face lines
  std::istringstream stream(*obj_string);
  std::string line;
  int face_count = 0;

  while (std::getline(stream, line)) {
    if (line.length() >= 2 && line[0] == 'f' && line[1] == ' ') {
      face_count++;
      // Check that the face line contains // (position//normal format)
      EXPECT_NE(line.find("//"), std::string::npos);
    }
  }

  EXPECT_EQ(face_count, test_mesh_.face_count());
}

TEST_F(ObjExportTest, ExportToFile) {
  const auto temp_path =
      std::filesystem::temp_directory_path() / "nodeflux_test_export.obj";

  bool success = io::ObjExporter::export_mesh(test_mesh_, temp_path.string());
  EXPECT_TRUE(success);

  // Verify file exists and is not empty
  std::ifstream file(temp_path);
  ASSERT_TRUE(file.is_open());

  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  EXPECT_FALSE(content.empty());

  // Verify content has vertices, normals, and faces
  EXPECT_NE(content.find("v "), std::string::npos);
  EXPECT_NE(content.find("vn "), std::string::npos);
  EXPECT_NE(content.find("f "), std::string::npos);
  EXPECT_NE(content.find("//"), std::string::npos);

  file.close();

  // Clean up
  std::error_code ec;
  std::filesystem::remove(temp_path, ec);
}

TEST_F(ObjExportTest, EmptyMeshReturnsNullopt) {
  core::Mesh empty_mesh;

  auto obj_string = io::ObjExporter::mesh_to_obj_string(empty_mesh);
  EXPECT_FALSE(obj_string.has_value());
}

TEST_F(ObjExportTest, VerifyOneBasedIndexing) {
  auto obj_string = io::ObjExporter::mesh_to_obj_string(test_mesh_);

  ASSERT_TRUE(obj_string.has_value());

  // OBJ format uses 1-based indexing
  // Check that no face references index 0
  std::istringstream stream(*obj_string);
  std::string line;
  bool found_valid_index = false;

  while (std::getline(stream, line)) {
    if (line.length() >= 2 && line[0] == 'f' && line[1] == ' ') {
      // Should not contain " 0 " or "//0 " (zero index)
      EXPECT_EQ(line.find(" 0 "), std::string::npos);
      EXPECT_EQ(line.find("//0 "), std::string::npos);
      EXPECT_EQ(line.find(" 0//"), std::string::npos);
      EXPECT_EQ(line.find("f 0"), std::string::npos);

      // At least one face should contain valid 1-based indices
      if (line.find(" 1") != std::string::npos ||
          line.find("f 1") != std::string::npos) {
        found_valid_index = true;
      }
    }
  }

  // Verify that we found at least some valid 1-based indices
  EXPECT_TRUE(found_valid_index);
}

TEST_F(ObjExportTest, VerifyNormalMagnitude) {
  auto obj_string = io::ObjExporter::mesh_to_obj_string(test_mesh_);

  ASSERT_TRUE(obj_string.has_value());

  // Parse normals and verify they are unit length (approximately)
  std::istringstream stream(*obj_string);
  std::string line;

  while (std::getline(stream, line)) {
    if (line.length() >= 3 && line[0] == 'v' && line[1] == 'n' &&
        line[2] == ' ') {
      std::istringstream normal_stream(line.substr(3));
      double x = 0.0;
      double y = 0.0;
      double z = 0.0;
      normal_stream >> x >> y >> z;

      double magnitude = std::sqrt(x * x + y * y + z * z);
      EXPECT_NEAR(magnitude, 1.0, 0.01) << "Normal should be unit length";
    }
  }
}
