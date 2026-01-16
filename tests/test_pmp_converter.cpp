#include "nodo/core/standard_attributes.hpp"
#include "nodo/geometry/box_generator.hpp"
#include "nodo/geometry/sphere_generator.hpp"
#include "nodo/processing/pmp_converter.hpp"

#include <gtest/gtest.h>

using namespace nodo;
using namespace nodo::processing::detail;

namespace attrs = nodo::core::standard_attrs;

class PMPConverterTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create test sphere
    auto sphere_result = geometry::SphereGenerator::generate_icosphere(1.0, 2);
    ASSERT_TRUE(sphere_result.has_value());
    sphere_container_ = std::move(*sphere_result);

    // Create test box
    auto box_result = geometry::BoxGenerator::generate(2.0, 2.0, 2.0);
    ASSERT_TRUE(box_result.has_value());
    box_container_ = std::move(*box_result);
  }

  core::GeometryContainer sphere_container_;
  core::GeometryContainer box_container_;
};

// ============================================================================
// Basic Conversion Tests
// ============================================================================

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

TEST_F(PMPConverterTest, PMPToContainer) {
  // Nodo → PMP
  auto pmp_mesh = PMPConverter::to_pmp(sphere_container_);

  // PMP → Nodo
  auto result_container = PMPConverter::from_pmp(pmp_mesh);

  // Verify dimensions match
  EXPECT_EQ(result_container.topology().point_count(), sphere_container_.topology().point_count());
  EXPECT_EQ(result_container.topology().primitive_count(), sphere_container_.topology().primitive_count());

  // Verify position attribute exists
  EXPECT_TRUE(result_container.has_point_attribute(attrs::P));
}

// ============================================================================
// Round-Trip Conversion Tests
// ============================================================================

TEST_F(PMPConverterTest, RoundTripContainer) {
  // Nodo → PMP → Nodo
  auto pmp_mesh = PMPConverter::to_pmp(sphere_container_);
  auto result_container = PMPConverter::from_pmp(pmp_mesh);

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
  // Start with a copy of the sphere (need to recreate it since GeometryContainer is move-only)
  auto sphere_copy = geometry::SphereGenerator::generate_icosphere(1.0, 2);
  ASSERT_TRUE(sphere_copy.has_value());
  auto current = std::move(*sphere_copy);

  const int num_rounds = 5;
  for (int round = 0; round < num_rounds; ++round) {
    auto pmp_mesh = PMPConverter::to_pmp(current);
    current = PMPConverter::from_pmp(pmp_mesh);
  }

  // After multiple round trips, should still be close
  const auto* original_pos = sphere_container_.get_point_attribute_typed<core::Vec3f>(attrs::P);
  const auto* current_pos = current.get_point_attribute_typed<core::Vec3f>(attrs::P);
  ASSERT_NE(original_pos, nullptr);
  ASSERT_NE(current_pos, nullptr);

  auto orig_span = original_pos->values();
  auto curr_span = current_pos->values();
  EXPECT_EQ(curr_span.size(), orig_span.size());

  const float tolerance = 1e-4f; // Slightly larger tolerance
  for (size_t i = 0; i < orig_span.size(); ++i) {
    for (int j = 0; j < 3; ++j) {
      EXPECT_NEAR(curr_span[i](j), orig_span[i](j), tolerance)
          << "Position mismatch after " << num_rounds << " round trips at point " << i << " coordinate " << j;
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
  auto result = PMPConverter::from_pmp(pmp_mesh, true);

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
  auto result = PMPConverter::from_pmp(pmp_mesh, false);

  // Should still have positions
  EXPECT_TRUE(result.has_point_attribute(attrs::P));

  // Should NOT have normals (since we disabled preservation)
  EXPECT_FALSE(result.has_point_attribute(attrs::N));
}

// ============================================================================
// Validation Tests
// ============================================================================

TEST_F(PMPConverterTest, ValidationValidMesh) {
  auto error = PMPConverter::validate_for_pmp(sphere_container_);
  EXPECT_TRUE(error.empty()) << "Error: " << error;
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

  // Convert
  auto pmp_mesh = PMPConverter::to_pmp(*large_sphere);
  auto result_container = PMPConverter::from_pmp(pmp_mesh);

  // Verify counts
  EXPECT_EQ(result_container.topology().point_count(), large_sphere->topology().point_count());
  EXPECT_EQ(result_container.topology().primitive_count(), large_sphere->topology().primitive_count());

  // Spot check a few vertices
  const auto* orig_pos = large_sphere->get_point_attribute_typed<core::Vec3f>(attrs::P);
  const auto* result_pos = result_container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  ASSERT_NE(orig_pos, nullptr);
  ASSERT_NE(result_pos, nullptr);

  auto orig_span = orig_pos->values();
  auto result_span = result_pos->values();

  const float tolerance = 1e-5f;
  for (size_t i = 0; i < std::min(size_t(10), orig_span.size()); ++i) {
    for (int j = 0; j < 3; ++j) {
      EXPECT_NEAR(result_span[i](j), orig_span[i](j), tolerance)
          << "Position mismatch at point " << i << " coordinate " << j;
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
    auto pmp_mesh = PMPConverter::to_pmp(sphere_container_);
    auto result_container = PMPConverter::from_pmp(pmp_mesh);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Should be reasonably fast (< 1 second for 100 iterations of small mesh)
  EXPECT_LT(duration.count(), 1000) << "Conversion is too slow: " << duration.count() << "ms for " << iterations
                                    << " iterations";
}
