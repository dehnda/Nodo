#include "nodeflux/core/attribute_group.hpp"
#include "nodeflux/core/geometry_container.hpp"
#include "nodeflux/core/standard_attributes.hpp"
#include <gtest/gtest.h>

using namespace nodeflux::core;

class AttributeGroupTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a simple cube geometry: 8 points
    geo = std::make_unique<GeometryContainer>();
    geo->set_point_count(8);

    // Add position attribute
    geo->add_point_attribute(standard_attrs::P, AttributeType::VEC3F);
    auto *pos = geo->get_point_attribute_typed<Vec3f>(standard_attrs::P);
    auto pos_span = pos->values_writable();

    // Cube corners
    pos_span[0] = Vec3f{0.0f, 0.0f, 0.0f};
    pos_span[1] = Vec3f{1.0f, 0.0f, 0.0f};
    pos_span[2] = Vec3f{1.0f, 1.0f, 0.0f};
    pos_span[3] = Vec3f{0.0f, 1.0f, 0.0f};
    pos_span[4] = Vec3f{0.0f, 0.0f, 1.0f};
    pos_span[5] = Vec3f{1.0f, 0.0f, 1.0f};
    pos_span[6] = Vec3f{1.0f, 1.0f, 1.0f};
    pos_span[7] = Vec3f{0.0f, 1.0f, 1.0f};

    // Add some primitives
    geo->set_vertex_count(24); // 6 quads
    geo->set_primitive_count(6);

    // Setup simple vertex mapping (each primitive is a quad face)
    for (int i = 0; i < 24; ++i) {
      geo->topology().set_vertex_point(i, i % 8);
    }
  }

  std::unique_ptr<GeometryContainer> geo;
};

// ============================================================================
// Basic Group Creation and Management
// ============================================================================

TEST_F(AttributeGroupTest, CreatePointGroup) {
  EXPECT_TRUE(create_group(*geo, "test_group", ElementClass::POINT));
  EXPECT_TRUE(has_group(*geo, "test_group", ElementClass::POINT));

  // Check that it's created as an int attribute
  auto *group_attr = geo->get_point_attribute("group_test_group");
  ASSERT_NE(group_attr, nullptr);
  EXPECT_EQ(group_attr->descriptor().type(), AttributeType::INT);
}

TEST_F(AttributeGroupTest, CreatePrimitiveGroup) {
  EXPECT_TRUE(create_group(*geo, "prim_group", ElementClass::PRIMITIVE));
  EXPECT_TRUE(has_group(*geo, "prim_group", ElementClass::PRIMITIVE));

  auto *group_attr = geo->get_primitive_attribute("group_prim_group");
  ASSERT_NE(group_attr, nullptr);
}

TEST_F(AttributeGroupTest, DeleteGroup) {
  create_group(*geo, "temp_group", ElementClass::POINT);
  EXPECT_TRUE(has_group(*geo, "temp_group", ElementClass::POINT));

  EXPECT_TRUE(delete_group(*geo, "temp_group", ElementClass::POINT));
  EXPECT_FALSE(has_group(*geo, "temp_group", ElementClass::POINT));
}

TEST_F(AttributeGroupTest, AddSingleElementToGroup) {
  create_group(*geo, "selection", ElementClass::POINT);

  EXPECT_TRUE(add_to_group(*geo, "selection", ElementClass::POINT, 0));
  EXPECT_TRUE(add_to_group(*geo, "selection", ElementClass::POINT, 3));
  EXPECT_TRUE(add_to_group(*geo, "selection", ElementClass::POINT, 7));

  EXPECT_TRUE(is_in_group(*geo, "selection", ElementClass::POINT, 0));
  EXPECT_TRUE(is_in_group(*geo, "selection", ElementClass::POINT, 3));
  EXPECT_TRUE(is_in_group(*geo, "selection", ElementClass::POINT, 7));

  EXPECT_FALSE(is_in_group(*geo, "selection", ElementClass::POINT, 1));
  EXPECT_FALSE(is_in_group(*geo, "selection", ElementClass::POINT, 2));
}

TEST_F(AttributeGroupTest, AddMultipleElementsToGroup) {
  create_group(*geo, "corners", ElementClass::POINT);

  std::vector<size_t> corner_indices = {0, 2, 5, 7};
  EXPECT_TRUE(
      add_to_group(*geo, "corners", ElementClass::POINT, corner_indices));

  for (size_t idx : corner_indices) {
    EXPECT_TRUE(is_in_group(*geo, "corners", ElementClass::POINT, idx));
  }

  EXPECT_FALSE(is_in_group(*geo, "corners", ElementClass::POINT, 1));
  EXPECT_FALSE(is_in_group(*geo, "corners", ElementClass::POINT, 3));
}

TEST_F(AttributeGroupTest, RemoveFromGroup) {
  create_group(*geo, "group1", ElementClass::POINT);

  add_to_group(*geo, "group1", ElementClass::POINT, {0, 1, 2, 3});
  EXPECT_TRUE(is_in_group(*geo, "group1", ElementClass::POINT, 1));

  remove_from_group(*geo, "group1", ElementClass::POINT, 1);
  EXPECT_FALSE(is_in_group(*geo, "group1", ElementClass::POINT, 1));
  EXPECT_TRUE(is_in_group(*geo, "group1", ElementClass::POINT, 0));
  EXPECT_TRUE(is_in_group(*geo, "group1", ElementClass::POINT, 2));
}

TEST_F(AttributeGroupTest, GetGroupElements) {
  create_group(*geo, "evens", ElementClass::POINT);
  add_to_group(*geo, "evens", ElementClass::POINT, {0, 2, 4, 6});

  auto elements = get_group_elements(*geo, "evens", ElementClass::POINT);
  EXPECT_EQ(elements.size(), 4);

  std::sort(elements.begin(), elements.end());
  EXPECT_EQ(elements[0], 0);
  EXPECT_EQ(elements[1], 2);
  EXPECT_EQ(elements[2], 4);
  EXPECT_EQ(elements[3], 6);
}

TEST_F(AttributeGroupTest, GetGroupSize) {
  create_group(*geo, "test", ElementClass::POINT);
  EXPECT_EQ(get_group_size(*geo, "test", ElementClass::POINT), 0);

  add_to_group(*geo, "test", ElementClass::POINT, {0, 1, 2});
  EXPECT_EQ(get_group_size(*geo, "test", ElementClass::POINT), 3);
}

TEST_F(AttributeGroupTest, ClearGroup) {
  create_group(*geo, "temp", ElementClass::POINT);
  add_to_group(*geo, "temp", ElementClass::POINT, {0, 1, 2, 3, 4});
  EXPECT_EQ(get_group_size(*geo, "temp", ElementClass::POINT), 5);

  EXPECT_TRUE(clear_group(*geo, "temp", ElementClass::POINT));
  EXPECT_EQ(get_group_size(*geo, "temp", ElementClass::POINT), 0);

  // Group still exists, just empty
  EXPECT_TRUE(has_group(*geo, "temp", ElementClass::POINT));
}

// ============================================================================
// Group Boolean Operations
// ============================================================================

TEST_F(AttributeGroupTest, GroupUnion) {
  create_group(*geo, "group_a", ElementClass::POINT);
  create_group(*geo, "group_b", ElementClass::POINT);

  add_to_group(*geo, "group_a", ElementClass::POINT, {0, 1, 2});
  add_to_group(*geo, "group_b", ElementClass::POINT, {2, 3, 4});

  EXPECT_TRUE(
      group_union(*geo, "group_a", "group_b", "result", ElementClass::POINT));

  auto result = get_group_elements(*geo, "result", ElementClass::POINT);
  std::sort(result.begin(), result.end());

  EXPECT_EQ(result.size(), 5);
  EXPECT_EQ(result[0], 0);
  EXPECT_EQ(result[1], 1);
  EXPECT_EQ(result[2], 2);
  EXPECT_EQ(result[3], 3);
  EXPECT_EQ(result[4], 4);
}

TEST_F(AttributeGroupTest, GroupIntersection) {
  create_group(*geo, "group_a", ElementClass::POINT);
  create_group(*geo, "group_b", ElementClass::POINT);

  add_to_group(*geo, "group_a", ElementClass::POINT, {0, 1, 2, 3});
  add_to_group(*geo, "group_b", ElementClass::POINT, {2, 3, 4, 5});

  EXPECT_TRUE(group_intersection(*geo, "group_a", "group_b", "result",
                                 ElementClass::POINT));

  auto result = get_group_elements(*geo, "result", ElementClass::POINT);
  std::sort(result.begin(), result.end());

  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 2);
  EXPECT_EQ(result[1], 3);
}

TEST_F(AttributeGroupTest, GroupDifference) {
  create_group(*geo, "group_a", ElementClass::POINT);
  create_group(*geo, "group_b", ElementClass::POINT);

  add_to_group(*geo, "group_a", ElementClass::POINT, {0, 1, 2, 3, 4});
  add_to_group(*geo, "group_b", ElementClass::POINT, {2, 3});

  EXPECT_TRUE(group_difference(*geo, "group_a", "group_b", "result",
                               ElementClass::POINT));

  auto result = get_group_elements(*geo, "result", ElementClass::POINT);
  std::sort(result.begin(), result.end());

  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 0);
  EXPECT_EQ(result[1], 1);
  EXPECT_EQ(result[2], 4);
}

TEST_F(AttributeGroupTest, GroupInvert) {
  create_group(*geo, "odds", ElementClass::POINT);
  add_to_group(*geo, "odds", ElementClass::POINT, {1, 3, 5, 7});

  EXPECT_TRUE(group_invert(*geo, "odds", "evens", ElementClass::POINT));

  auto evens = get_group_elements(*geo, "evens", ElementClass::POINT);
  std::sort(evens.begin(), evens.end());

  EXPECT_EQ(evens.size(), 4);
  EXPECT_EQ(evens[0], 0);
  EXPECT_EQ(evens[1], 2);
  EXPECT_EQ(evens[2], 4);
  EXPECT_EQ(evens[3], 6);
}

// ============================================================================
// Pattern-Based Selection
// ============================================================================

TEST_F(AttributeGroupTest, SelectPattern_EverySecond) {
  EXPECT_TRUE(select_pattern(*geo, "pattern", ElementClass::POINT, 2, 0));

  auto elements = get_group_elements(*geo, "pattern", ElementClass::POINT);
  std::sort(elements.begin(), elements.end());

  // Should select 0, 2, 4, 6
  EXPECT_EQ(elements.size(), 4);
  EXPECT_EQ(elements[0], 0);
  EXPECT_EQ(elements[1], 2);
  EXPECT_EQ(elements[2], 4);
  EXPECT_EQ(elements[3], 6);
}

TEST_F(AttributeGroupTest, SelectPattern_WithOffset) {
  EXPECT_TRUE(select_pattern(*geo, "pattern", ElementClass::POINT, 2, 1));

  auto elements = get_group_elements(*geo, "pattern", ElementClass::POINT);
  std::sort(elements.begin(), elements.end());

  // Should select 1, 3, 5, 7
  EXPECT_EQ(elements.size(), 4);
  EXPECT_EQ(elements[0], 1);
  EXPECT_EQ(elements[1], 3);
  EXPECT_EQ(elements[2], 5);
  EXPECT_EQ(elements[3], 7);
}

TEST_F(AttributeGroupTest, SelectPattern_EveryThird) {
  EXPECT_TRUE(select_pattern(*geo, "pattern", ElementClass::POINT, 3, 0));

  auto elements = get_group_elements(*geo, "pattern", ElementClass::POINT);
  std::sort(elements.begin(), elements.end());

  // Should select 0, 3, 6
  EXPECT_EQ(elements.size(), 3);
  EXPECT_EQ(elements[0], 0);
  EXPECT_EQ(elements[1], 3);
  EXPECT_EQ(elements[2], 6);
}

TEST_F(AttributeGroupTest, SelectRange) {
  EXPECT_TRUE(select_range(*geo, "middle", ElementClass::POINT, 2, 6));

  auto elements = get_group_elements(*geo, "middle", ElementClass::POINT);
  std::sort(elements.begin(), elements.end());

  EXPECT_EQ(elements.size(), 4);
  EXPECT_EQ(elements[0], 2);
  EXPECT_EQ(elements[1], 3);
  EXPECT_EQ(elements[2], 4);
  EXPECT_EQ(elements[3], 5);
}

TEST_F(AttributeGroupTest, SelectRandom) {
  // Select 3 random points with a fixed seed
  EXPECT_TRUE(select_random(*geo, "random", ElementClass::POINT, 3, 42));

  auto elements = get_group_elements(*geo, "random", ElementClass::POINT);
  EXPECT_EQ(elements.size(), 3);

  // With same seed, should get same results
  EXPECT_TRUE(select_random(*geo, "random2", ElementClass::POINT, 3, 42));
  auto elements2 = get_group_elements(*geo, "random2", ElementClass::POINT);

  std::sort(elements.begin(), elements.end());
  std::sort(elements2.begin(), elements2.end());
  EXPECT_EQ(elements, elements2);
}

TEST_F(AttributeGroupTest, SelectByAttribute_Float) {
  // Add a height attribute
  geo->add_point_attribute("height", AttributeType::FLOAT);
  auto *height = geo->get_point_attribute_typed<float>("height");
  auto height_span = height->values_writable();

  for (size_t i = 0; i < 8; ++i) {
    height_span[i] = static_cast<float>(i);
  }

  // Select points with height > 3.5
  EXPECT_TRUE(
      select_by_attribute<float>(*geo, "tall", ElementClass::POINT, "height",
                                 [](const float &h) { return h > 3.5f; }));

  auto tall_points = get_group_elements(*geo, "tall", ElementClass::POINT);
  std::sort(tall_points.begin(), tall_points.end());

  EXPECT_EQ(tall_points.size(), 4);
  EXPECT_EQ(tall_points[0], 4);
  EXPECT_EQ(tall_points[1], 5);
  EXPECT_EQ(tall_points[2], 6);
  EXPECT_EQ(tall_points[3], 7);
}

TEST_F(AttributeGroupTest, SelectByAttribute_Vec3f) {
  // Select points on the top face (y > 0.5)
  EXPECT_TRUE(select_by_attribute<Vec3f>(
      *geo, "top_face", ElementClass::POINT, standard_attrs::P,
      [](const Vec3f &p) { return p.y() > 0.5f; }));

  auto top_points = get_group_elements(*geo, "top_face", ElementClass::POINT);
  std::sort(top_points.begin(), top_points.end());

  // Points 2, 3, 6, 7 have y = 1.0
  EXPECT_EQ(top_points.size(), 4);
  EXPECT_EQ(top_points[0], 2);
  EXPECT_EQ(top_points[1], 3);
  EXPECT_EQ(top_points[2], 6);
  EXPECT_EQ(top_points[3], 7);
}

// ============================================================================
// Primitive Groups
// ============================================================================

TEST_F(AttributeGroupTest, PrimitiveGroups) {
  create_group(*geo, "half", ElementClass::PRIMITIVE);

  // Add first 3 primitives to group
  add_to_group(*geo, "half", ElementClass::PRIMITIVE, {0, 1, 2});

  EXPECT_TRUE(is_in_group(*geo, "half", ElementClass::PRIMITIVE, 0));
  EXPECT_TRUE(is_in_group(*geo, "half", ElementClass::PRIMITIVE, 1));
  EXPECT_TRUE(is_in_group(*geo, "half", ElementClass::PRIMITIVE, 2));
  EXPECT_FALSE(is_in_group(*geo, "half", ElementClass::PRIMITIVE, 3));

  EXPECT_EQ(get_group_size(*geo, "half", ElementClass::PRIMITIVE), 3);
}

TEST_F(AttributeGroupTest, PrimitiveGroupPattern) {
  // Select every other primitive
  EXPECT_TRUE(
      select_pattern(*geo, "alternating", ElementClass::PRIMITIVE, 2, 0));

  auto prims = get_group_elements(*geo, "alternating", ElementClass::PRIMITIVE);
  std::sort(prims.begin(), prims.end());

  EXPECT_EQ(prims.size(), 3); // primitives 0, 2, 4
  EXPECT_EQ(prims[0], 0);
  EXPECT_EQ(prims[1], 2);
  EXPECT_EQ(prims[2], 4);
}

// ============================================================================
// Error Handling
// ============================================================================

TEST_F(AttributeGroupTest, ErrorHandling_GroupDoesNotExist) {
  EXPECT_FALSE(has_group(*geo, "nonexistent", ElementClass::POINT));
  EXPECT_FALSE(is_in_group(*geo, "nonexistent", ElementClass::POINT, 0));

  auto elements = get_group_elements(*geo, "nonexistent", ElementClass::POINT);
  EXPECT_TRUE(elements.empty());
}

TEST_F(AttributeGroupTest, ErrorHandling_InvalidElementIndex) {
  create_group(*geo, "test", ElementClass::POINT);

  // Trying to add out-of-bounds index should return false
  EXPECT_FALSE(add_to_group(*geo, "test", ElementClass::POINT, 999));
  EXPECT_FALSE(is_in_group(*geo, "test", ElementClass::POINT, 999));
}

TEST_F(AttributeGroupTest, ErrorHandling_CreateDuplicateGroup) {
  EXPECT_TRUE(create_group(*geo, "dup", ElementClass::POINT));

  // Creating same group again should fail
  EXPECT_FALSE(create_group(*geo, "dup", ElementClass::POINT));
}

// ============================================================================
// Complex Workflow Tests
// ============================================================================

TEST_F(AttributeGroupTest, ComplexWorkflow_SelectAndModify) {
  // 1. Create a group of top points
  EXPECT_TRUE(select_by_attribute<Vec3f>(
      *geo, "top", ElementClass::POINT, standard_attrs::P,
      [](const Vec3f &p) { return p.y() > 0.5f; }));

  // 2. Create a group of right points
  EXPECT_TRUE(select_by_attribute<Vec3f>(
      *geo, "right", ElementClass::POINT, standard_attrs::P,
      [](const Vec3f &p) { return p.x() > 0.5f; }));

  // 3. Find intersection (top-right corner points)
  EXPECT_TRUE(group_intersection(*geo, "top", "right", "top_right",
                                 ElementClass::POINT));

  auto corner_points =
      get_group_elements(*geo, "top_right", ElementClass::POINT);
  std::sort(corner_points.begin(), corner_points.end());

  // Points 2 and 6 have both y>0.5 and x>0.5
  EXPECT_EQ(corner_points.size(), 2);
  EXPECT_EQ(corner_points[0], 2);
  EXPECT_EQ(corner_points[1], 6);
}

TEST_F(AttributeGroupTest, ComplexWorkflow_MultipleOperations) {
  // Select all points
  create_group(*geo, "all", ElementClass::POINT);
  add_to_group(*geo, "all", ElementClass::POINT, {0, 1, 2, 3, 4, 5, 6, 7});

  // Select some to remove
  create_group(*geo, "remove", ElementClass::POINT);
  add_to_group(*geo, "remove", ElementClass::POINT, {1, 3, 5});

  // Difference: all - remove
  EXPECT_TRUE(
      group_difference(*geo, "all", "remove", "keep", ElementClass::POINT));

  auto keep = get_group_elements(*geo, "keep", ElementClass::POINT);
  std::sort(keep.begin(), keep.end());

  EXPECT_EQ(keep.size(), 5);
  EXPECT_EQ(keep[0], 0);
  EXPECT_EQ(keep[1], 2);
  EXPECT_EQ(keep[2], 4);
  EXPECT_EQ(keep[3], 6);
  EXPECT_EQ(keep[4], 7);
}
