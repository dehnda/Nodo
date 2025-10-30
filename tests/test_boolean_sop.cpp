#include "nodo/core/geometry_container.hpp"
#include "nodo/geometry/box_generator.hpp"
#include "nodo/geometry/sphere_generator.hpp"
#include "nodo/sop/boolean_sop.hpp"
#include <gtest/gtest.h>
#include <memory>


using namespace nodo;

class BooleanSOPTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Generate two simple box geometries for testing
    auto box1_result = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
    auto box2_result = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);

    ASSERT_TRUE(box1_result.has_value());
    ASSERT_TRUE(box2_result.has_value());

    geo1_ =
        std::make_shared<core::GeometryContainer>(box1_result.value().clone());
    geo2_ =
        std::make_shared<core::GeometryContainer>(box2_result.value().clone());
  }

  std::shared_ptr<core::GeometryContainer> geo1_;
  std::shared_ptr<core::GeometryContainer> geo2_;
};

TEST_F(BooleanSOPTest, UnionOperation) {
  auto boolean_node = std::make_shared<sop::BooleanSOP>("test_boolean");

  // Connect inputs via ports (using numeric port names)
  auto *port_a = boolean_node->get_input_ports().get_port("0");
  auto *port_b = boolean_node->get_input_ports().get_port("1");

  ASSERT_NE(port_a, nullptr);
  ASSERT_NE(port_b, nullptr);

  port_a->set_data(geo1_);
  port_b->set_data(geo2_);

  // Set operation to UNION (0)
  boolean_node->set_parameter("operation", 0);

  // Execute boolean operation
  auto result = boolean_node->cook();

  // Check result is valid
  ASSERT_NE(result, nullptr);
  EXPECT_GT(result->topology().point_count(), 0);
  EXPECT_GT(result->topology().primitive_count(), 0);
}

TEST_F(BooleanSOPTest, IntersectionOperation) {
  auto boolean_node = std::make_shared<sop::BooleanSOP>("test_intersection");

  // Connect inputs
  auto *port_a = boolean_node->get_input_ports().get_port("0");
  auto *port_b = boolean_node->get_input_ports().get_port("1");

  port_a->set_data(geo1_);
  port_b->set_data(geo2_);

  // Set operation to INTERSECTION (1)
  boolean_node->set_parameter("operation", 1);

  // Execute
  auto result = boolean_node->cook();

  // Check result is valid (intersection of two identical boxes = same box)
  ASSERT_NE(result, nullptr);
  EXPECT_GT(result->topology().point_count(), 0);
}

TEST_F(BooleanSOPTest, DifferenceOperation) {
  auto boolean_node = std::make_shared<sop::BooleanSOP>("test_difference");

  // Create two boxes, one slightly offset so difference is non-empty
  auto box1_result = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
  auto box2_result = geometry::BoxGenerator::generate(0.5, 0.5, 0.5);

  ASSERT_TRUE(box1_result.has_value());
  ASSERT_TRUE(box2_result.has_value());

  auto geo1 =
      std::make_shared<core::GeometryContainer>(box1_result.value().clone());
  auto geo2 =
      std::make_shared<core::GeometryContainer>(box2_result.value().clone());

  // Connect inputs
  auto *port_a = boolean_node->get_input_ports().get_port("0");
  auto *port_b = boolean_node->get_input_ports().get_port("1");

  port_a->set_data(geo1);
  port_b->set_data(geo2);

  // Set operation to DIFFERENCE (2)
  boolean_node->set_parameter("operation", 2);

  // Execute
  auto result = boolean_node->cook();

  // Check result is valid (large box - small box = non-empty)
  ASSERT_NE(result, nullptr);
  EXPECT_GT(result->topology().point_count(), 0);
}

TEST_F(BooleanSOPTest, MissingInputA) {
  auto boolean_node = std::make_shared<sop::BooleanSOP>("test_missing_a");

  // Try to execute with no inputs
  auto result = boolean_node->cook();

  // Should return nullptr
  EXPECT_EQ(result, nullptr);
}

TEST_F(BooleanSOPTest, MissingInputB) {
  auto boolean_node = std::make_shared<sop::BooleanSOP>("test_missing_b");

  // Only connect input A
  auto *port_a = boolean_node->get_input_ports().get_port("0");
  port_a->set_data(geo1_);

  // Try to execute
  auto result = boolean_node->cook();

  // Should return nullptr
  EXPECT_EQ(result, nullptr);
}

TEST_F(BooleanSOPTest, InvalidOperationType) {
  auto boolean_node = std::make_shared<sop::BooleanSOP>("test_invalid_op");

  // Connect both inputs
  auto *port_a = boolean_node->get_input_ports().get_port("0");
  auto *port_b = boolean_node->get_input_ports().get_port("1");

  port_a->set_data(geo1_);
  port_b->set_data(geo2_);

  // Set invalid operation type
  boolean_node->set_parameter("operation", 99);

  // Execute should fail gracefully
  auto result = boolean_node->cook();
  EXPECT_EQ(result, nullptr);
}

// Test that boolean operations produce manifold geometry with no internal faces
TEST_F(BooleanSOPTest, NoInternalGeometry) {
  // Create two overlapping spheres to ensure we have interesting boolean
  // topology
  constexpr int u_segments = 32;
  constexpr int v_segments = 16;
  auto sphere1_result = geometry::SphereGenerator::generate_uv_sphere(
      1.0, u_segments, v_segments);
  auto sphere2_result = geometry::SphereGenerator::generate_uv_sphere(
      1.0, u_segments, v_segments);

  ASSERT_TRUE(sphere1_result.has_value());
  ASSERT_TRUE(sphere2_result.has_value());

  // Create geometry containers
  auto geo1 =
      std::make_shared<core::GeometryContainer>(sphere1_result.value().clone());
  auto geo2 =
      std::make_shared<core::GeometryContainer>(sphere2_result.value().clone());

  // Offset second sphere by 0.5 units in X to create overlap
  constexpr float offset = 0.5F;
  auto *pos2 = geo2->get_point_attribute_typed<core::Vec3f>("P");
  ASSERT_NE(pos2, nullptr);
  for (size_t i = 0; i < geo2->topology().point_count(); ++i) {
    (*pos2)[i].x() += offset;
  }

  // Test UNION operation
  {
    auto boolean_node =
        std::make_shared<sop::BooleanSOP>("test_union_manifold");
    auto *port_a = boolean_node->get_input_ports().get_port("0");
    auto *port_b = boolean_node->get_input_ports().get_port("1");

    port_a->set_data(geo1);
    port_b->set_data(geo2);
    boolean_node->set_parameter("operation", 0); // UNION

    auto result = boolean_node->cook();
    ASSERT_NE(result, nullptr);

    // Verify result is manifold:
    // 1. Each edge should be shared by exactly 2 faces
    // 2. No duplicate vertices at the same position
    // 3. All faces should reference valid vertices

    const auto &topo = result->topology();
    ASSERT_GT(topo.point_count(), 0);
    ASSERT_GT(topo.primitive_count(), 0);

    // Build edge-to-face map to verify manifold property
    std::map<std::pair<int, int>, int> edge_count;

    for (size_t prim_idx = 0; prim_idx < topo.primitive_count(); ++prim_idx) {
      const auto &vert_indices = topo.get_primitive_vertices(prim_idx);
      std::vector<int> point_indices;
      point_indices.reserve(vert_indices.size());

      for (size_t vert_idx : vert_indices) {
        point_indices.push_back(topo.get_vertex_point(vert_idx));
      }

      // For each edge in this primitive (assuming triangles)
      for (size_t i = 0; i < point_indices.size(); ++i) {
        int point_a = point_indices[i];
        int point_b = point_indices[(i + 1) % point_indices.size()];

        // Normalize edge (smaller index first)
        if (point_a > point_b) {
          std::swap(point_a, point_b);
        }

        edge_count[{point_a, point_b}]++;
      }
    }

    // Verify all edges are manifold (shared by exactly 1 or 2 faces)
    // Edge count of 1 means it's a boundary edge (valid for open meshes)
    // Edge count of 2 means it's an internal manifold edge (ideal)
    // Edge count > 2 means non-manifold (BAD - indicates internal geometry)
    for (const auto &[edge, count] : edge_count) {
      EXPECT_LE(count, 2) << "Non-manifold edge detected between vertices "
                          << edge.first << " and " << edge.second
                          << " (shared by " << count << " faces). "
                          << "This indicates internal geometry!";
    }

    // Additional check: For closed meshes (like sphere unions),
    // all edges should be shared by exactly 2 faces
    // We'll just verify no edge has more than 2 faces (non-manifold condition)
    int non_manifold_edges = 0;
    for (const auto &[edge, count] : edge_count) {
      if (count > 2) {
        non_manifold_edges++;
      }
    }

    EXPECT_EQ(non_manifold_edges, 0)
        << "Found " << non_manifold_edges << " non-manifold edges, "
        << "indicating internal faces/geometry that should have been removed!";
  }

  // Test DIFFERENCE operation (more likely to create complex topology)
  {
    auto boolean_node = std::make_shared<sop::BooleanSOP>("test_diff_manifold");
    auto *port_a = boolean_node->get_input_ports().get_port("0");
    auto *port_b = boolean_node->get_input_ports().get_port("1");

    port_a->set_data(geo1);
    port_b->set_data(geo2);
    boolean_node->set_parameter("operation", 2); // DIFFERENCE

    auto result = boolean_node->cook();
    ASSERT_NE(result, nullptr);

    // Same manifold verification
    const auto &topo = result->topology();
    std::map<std::pair<int, int>, int> edge_count;

    for (size_t prim_idx = 0; prim_idx < topo.primitive_count(); ++prim_idx) {
      const auto &vert_indices = topo.get_primitive_vertices(prim_idx);
      std::vector<int> point_indices;
      point_indices.reserve(vert_indices.size());

      for (size_t vert_idx : vert_indices) {
        point_indices.push_back(topo.get_vertex_point(vert_idx));
      }

      for (size_t i = 0; i < point_indices.size(); ++i) {
        int point_a = point_indices[i];
        int point_b = point_indices[(i + 1) % point_indices.size()];
        if (point_a > point_b) {
          std::swap(point_a, point_b);
        }
        edge_count[{point_a, point_b}]++;
      }
    }

    // Verify manifold property
    int non_manifold_edges = 0;
    for (const auto &[edge, count] : edge_count) {
      if (count > 2) {
        non_manifold_edges++;
      }
    }

    EXPECT_EQ(non_manifold_edges, 0)
        << "DIFFERENCE operation produced " << non_manifold_edges
        << " non-manifold edges!";
  }
}
