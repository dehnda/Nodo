#include "nodo/core/standard_attributes.hpp"
#include "nodo/geometry/box_generator.hpp"
#include "nodo/geometry/sphere_generator.hpp"
#include "nodo/processing/pmp_converter.hpp"

#include <gtest/gtest.h>

using namespace nodo;
using namespace nodo::processing::detail;

namespace attrs = nodo::core::standard_attrs;

// Helper to convert GeometryContainer to Mesh for comparison
static core::Mesh container_to_mesh(const core::GeometryContainer& container) {
  core::Mesh mesh;

  // Get positions
  const auto* positions = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (!positions) {
    return mesh;
  }

  auto pos_span = positions->values();
  const size_t n_verts = pos_span.size();

  // Build vertex matrix
  Eigen::Matrix<double, Eigen::Dynamic, 3, Eigen::RowMajor> V(n_verts, 3);
  for (size_t i = 0; i < n_verts; ++i) {
    V(i, 0) = static_cast<double>(pos_span[i](0));
    V(i, 1) = static_cast<double>(pos_span[i](1));
    V(i, 2) = static_cast<double>(pos_span[i](2));
  }

  // Build face matrix
  const auto& topology = container.topology();
  const size_t n_faces = topology.primitive_count();
  Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor> F(n_faces, 3);

  for (size_t i = 0; i < n_faces; ++i) {
    const auto& prim_verts = topology.get_primitive_vertices(i);
    F(i, 0) = prim_verts[0];
    F(i, 1) = prim_verts[1];
    F(i, 2) = prim_verts[2];
  }

  mesh.vertices() = V;
  mesh.faces() = F;

  return mesh;
}

class PMPConverterTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create test sphere
    auto sphere_result = geometry::SphereGenerator::generate_icosphere(1.0, 2);
    ASSERT_TRUE(sphere_result.has_value());
    sphere_container_ = std::move(*sphere_result);

    // Convert to mesh for testing
    sphere_mesh_ = container_to_mesh(sphere_container_);

    // Create test box
    auto box_result = geometry::BoxGenerator::generate(2.0, 2.0, 2.0);
    ASSERT_TRUE(box_result.has_value());
    box_container_ = std::move(*box_result);
    box_mesh_ = container_to_mesh(box_container_);
  }

  core::GeometryContainer sphere_container_;
  core::Mesh sphere_mesh_;
  core::GeometryContainer box_container_;
  core::Mesh box_mesh_;
};

// ============================================================================
// Basic Conversion Tests
// ============================================================================

TEST_F(PMPConverterTest, MeshToPMP) {
  // Convert Nodo mesh to PMP
  auto pmp_mesh = PMPConverter::to_pmp(sphere_mesh_);

  // Verify vertex and face counts match
  EXPECT_EQ(pmp_mesh.n_vertices(), static_cast<size_t>(sphere_mesh_.vertices().rows()));
  EXPECT_EQ(pmp_mesh.n_faces(), static_cast<size_t>(sphere_mesh_.faces().rows()));

  // Verify PMP mesh is valid
  EXPECT_FALSE(pmp_mesh.is_empty());
  EXPECT_TRUE(pmp_mesh.is_triangle_mesh());
}

TEST_F(PMPConverterTest, ContainerToPMP) {
  // Convert GeometryContainer to PMP
  auto pmp_mesh = PMPConverter::to_pmp(sphere_container_);

  // Verify counts match
  EXPECT_EQ(pmp_mesh.n_vertices(), sphere_container_.topology().point_count());
  EXPECT_EQ(pmp_mesh.n_faces(), sphere_container_.topology().primitive_count());

  // Verify PMP mesh is valid
  EXPECT_FALSE(pmp_mesh.is_empty());
  EXPECT_TRUE(pmp_mesh.is_triangle_mesh());
}

TEST_F(PMPConverterTest, PMPToMesh) {
  // Nodo → PMP
  auto pmp_mesh = PMPConverter::to_pmp(sphere_mesh_);

  // PMP → Nodo
  auto result_mesh = PMPConverter::from_pmp(pmp_mesh);

  // Verify dimensions match
  EXPECT_EQ(result_mesh.vertices().rows(), sphere_mesh_.vertices().rows());
  EXPECT_EQ(result_mesh.faces().rows(), sphere_mesh_.faces().rows());
}

TEST_F(PMPConverterTest, PMPToContainer) {
  // Nodo → PMP
  auto pmp_mesh = PMPConverter::to_pmp(sphere_container_);

  // PMP → Nodo
  auto result_container = PMPConverter::from_pmp_container(pmp_mesh);

  // Verify dimensions match
  EXPECT_EQ(result_container.topology().point_count(), sphere_container_.topology().point_count());
  EXPECT_EQ(result_container.topology().primitive_count(), sphere_container_.topology().primitive_count());

  // Verify position attribute exists
  EXPECT_TRUE(result_container.has_point_attribute(attrs::P));
}

// ============================================================================
// Round-Trip Conversion Tests
// ============================================================================

TEST_F(PMPConverterTest, RoundTripMesh) {
  // Nodo → PMP → Nodo
  auto pmp_mesh = PMPConverter::to_pmp(sphere_mesh_);
  auto result_mesh = PMPConverter::from_pmp(pmp_mesh);

  // Should have same dimensions
  EXPECT_EQ(result_mesh.vertices().rows(), sphere_mesh_.vertices().rows());
  EXPECT_EQ(result_mesh.faces().rows(), sphere_mesh_.faces().rows());

  // Positions should be very close (accounting for float precision)
  const double tolerance = 1e-5;
  for (int i = 0; i < sphere_mesh_.vertices().rows(); ++i) {
    for (int j = 0; j < 3; ++j) {
      EXPECT_NEAR(result_mesh.vertices()(i, j), sphere_mesh_.vertices()(i, j), tolerance)
          << "Mismatch at vertex " << i << " coordinate " << j;
    }
  }

  // Face indices should match exactly
  for (int i = 0; i < sphere_mesh_.faces().rows(); ++i) {
    for (int j = 0; j < 3; ++j) {
      EXPECT_EQ(result_mesh.faces()(i, j), sphere_mesh_.faces()(i, j)) << "Mismatch at face " << i << " vertex " << j;
    }
  }
}

TEST_F(PMPConverterTest, RoundTripContainer) {
  // Nodo → PMP → Nodo
  auto pmp_mesh = PMPConverter::to_pmp(sphere_container_);
  auto result_container = PMPConverter::from_pmp_container(pmp_mesh);

  // Verify counts
  EXPECT_EQ(result_container.topology().point_count(), sphere_container_.topology().point_count());
  EXPECT_EQ(result_container.topology().primitive_count(), sphere_container_.topology().primitive_count());

  // Verify position attribute
  ASSERT_TRUE(result_container.has_point_attribute(attrs::P));

  const auto* orig_pos = sphere_container_.get_point_attribute_typed<core::Vec3f>(attrs::P);
  const auto* result_pos = result_container.get_point_attribute_typed<core::Vec3f>(attrs::P);

  ASSERT_NE(orig_pos, nullptr);
  ASSERT_NE(result_pos, nullptr);

  auto orig_span = orig_pos->values();
  auto result_span = result_pos->values();

  EXPECT_EQ(result_span.size(), orig_span.size());

  // Check positions are close
  const float tolerance = 1e-5f;
  for (size_t i = 0; i < orig_span.size(); ++i) {
    for (int j = 0; j < 3; ++j) {
      EXPECT_NEAR(result_span[i](j), orig_span[i](j), tolerance)
          << "Position mismatch at point " << i << " coordinate " << j;
    }
  }
}

TEST_F(PMPConverterTest, RoundTripMultipleTimes) {
  // Test that multiple conversions don't accumulate error
  auto current = sphere_mesh_;

  const int num_rounds = 5;
  for (int round = 0; round < num_rounds; ++round) {
    auto pmp_mesh = PMPConverter::to_pmp(current);
    current = PMPConverter::from_pmp(pmp_mesh);
  }

  // After multiple round trips, should still be close
  const double tolerance = 1e-4; // Slightly larger tolerance
  for (int i = 0; i < sphere_mesh_.vertices().rows(); ++i) {
    for (int j = 0; j < 3; ++j) {
      EXPECT_NEAR(current.vertices()(i, j), sphere_mesh_.vertices()(i, j), tolerance);
    }
  }
}

// ============================================================================
// Attribute Preservation Tests
// ============================================================================

TEST_F(PMPConverterTest, PreservesNormals) {
  // Convert to PMP (which computes normals)
  auto pmp_mesh = PMPConverter::to_pmp(sphere_container_);

  // Convert back with attribute preservation
  auto result = PMPConverter::from_pmp_container(pmp_mesh, true);

  // Should have normals attribute
  EXPECT_TRUE(result.has_point_attribute(attrs::N));

  // Verify normals are unit length
  const auto* normals = result.get_point_attribute_typed<core::Vec3f>(attrs::N);
  ASSERT_NE(normals, nullptr);

  auto norm_span = normals->values();
  for (size_t i = 0; i < norm_span.size(); ++i) {
    float length = std::sqrt(norm_span[i](0) * norm_span[i](0) + norm_span[i](1) * norm_span[i](1) +
                             norm_span[i](2) * norm_span[i](2));
    EXPECT_NEAR(length, 1.0f, 1e-5f) << "Normal at vertex " << i << " is not unit length";
  }
}

TEST_F(PMPConverterTest, WithoutAttributePreservation) {
  auto pmp_mesh = PMPConverter::to_pmp(sphere_container_);

  // Convert back WITHOUT attribute preservation
  auto result = PMPConverter::from_pmp_container(pmp_mesh, false);

  // Should still have positions
  EXPECT_TRUE(result.has_point_attribute(attrs::P));

  // Should NOT have normals (since we disabled preservation)
  EXPECT_FALSE(result.has_point_attribute(attrs::N));
}

// ============================================================================
// Validation Tests
// ============================================================================

TEST_F(PMPConverterTest, ValidationValidMesh) {
  auto error = PMPConverter::validate_for_pmp(sphere_mesh_);
  EXPECT_TRUE(error.empty()) << "Error: " << error;

  auto error2 = PMPConverter::validate_for_pmp(sphere_container_);
  EXPECT_TRUE(error2.empty()) << "Error: " << error2;
}

TEST_F(PMPConverterTest, ValidationEmptyMesh) {
  core::Mesh empty_mesh;
  auto error = PMPConverter::validate_for_pmp(empty_mesh);
  EXPECT_FALSE(error.empty());
  EXPECT_NE(error.find("vertices"), std::string::npos);
}

TEST_F(PMPConverterTest, ValidationInvalidIndices) {
  core::Mesh invalid_mesh;
  invalid_mesh.vertices().resize(3, 3);
  invalid_mesh.vertices() << 0, 0, 0, 1, 0, 0, 0, 1, 0;

  invalid_mesh.faces().resize(1, 3);
  invalid_mesh.faces() << 0, 1, 10; // Invalid index 10

  auto error = PMPConverter::validate_for_pmp(invalid_mesh);
  EXPECT_FALSE(error.empty());
  EXPECT_NE(error.find("invalid"), std::string::npos);
}

TEST_F(PMPConverterTest, ValidationMissingPositions) {
  core::GeometryContainer container;
  container.topology().set_point_count(3);
  container.topology().set_vertex_count(3);

  // Initialize vertex→point mapping
  for (int i = 0; i < 3; ++i) {
    container.topology().set_vertex_point(i, i);
  }

  // Add a primitive (but no position attribute)
  container.topology().add_primitive({0, 1, 2});

  auto error = PMPConverter::validate_for_pmp(container);
  EXPECT_FALSE(error.empty());
  EXPECT_NE(error.find("position"), std::string::npos);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(PMPConverterTest, ThrowsOnEmptyMesh) {
  core::Mesh empty_mesh;
  EXPECT_THROW(PMPConverter::to_pmp(empty_mesh), std::runtime_error);
}

TEST_F(PMPConverterTest, ThrowsOnMissingPositions) {
  core::GeometryContainer container;
  container.topology().set_point_count(3);
  // Missing position attribute

  EXPECT_THROW(PMPConverter::to_pmp(container), std::runtime_error);
}

TEST_F(PMPConverterTest, ThrowsOnNonTriangles) {
  core::GeometryContainer container;
  container.topology().set_point_count(4);
  container.topology().set_vertex_count(4);

  // Initialize vertex→point mapping
  for (int i = 0; i < 4; ++i) {
    container.topology().set_vertex_point(i, i);
  }

  // Add positions
  std::vector<core::Vec3f> positions = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto* pos_attr = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  pos_attr->resize(4);
  auto pos_span = pos_attr->values_writable();
  for (size_t i = 0; i < 4; ++i) {
    pos_span[i] = positions[i];
  }

  // Add quad
  container.topology().add_primitive({0, 1, 2, 3});

  // to_pmp() should accept quads (they can be triangulated later if needed)
  EXPECT_NO_THROW(PMPConverter::to_pmp(container));
}

// ============================================================================
// Different Mesh Types Tests
// ============================================================================

TEST_F(PMPConverterTest, LargeMeshConversion) {
  // Create a larger sphere for stress testing
  auto large_sphere = geometry::SphereGenerator::generate_icosphere(1.0, 4);
  ASSERT_TRUE(large_sphere.has_value());

  auto large_mesh = container_to_mesh(*large_sphere);

  // Convert
  auto pmp_mesh = PMPConverter::to_pmp(large_mesh);
  auto result_mesh = PMPConverter::from_pmp(pmp_mesh);

  // Verify
  EXPECT_EQ(result_mesh.vertices().rows(), large_mesh.vertices().rows());
  EXPECT_EQ(result_mesh.faces().rows(), large_mesh.faces().rows());

  // Spot check a few vertices
  const double tolerance = 1e-5;
  for (int i = 0; i < std::min(10, static_cast<int>(large_mesh.vertices().rows())); ++i) {
    for (int j = 0; j < 3; ++j) {
      EXPECT_NEAR(result_mesh.vertices()(i, j), large_mesh.vertices()(i, j), tolerance);
    }
  }
}

// ============================================================================
// Performance Tests (Optional)
// ============================================================================

TEST_F(PMPConverterTest, ConversionPerformance) {
  // This is more of a sanity check than a strict benchmark
  auto start = std::chrono::high_resolution_clock::now();

  const int iterations = 100;
  for (int i = 0; i < iterations; ++i) {
    auto pmp_mesh = PMPConverter::to_pmp(sphere_mesh_);
    auto result_mesh = PMPConverter::from_pmp(pmp_mesh);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Should be reasonably fast (< 1 second for 100 iterations of small mesh)
  EXPECT_LT(duration.count(), 1000) << "Conversion is too slow: " << duration.count() << "ms for " << iterations
                                    << " iterations";
}
