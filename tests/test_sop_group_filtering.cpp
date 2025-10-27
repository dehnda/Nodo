#include "nodeflux/core/attribute_group.hpp"
#include "nodeflux/core/standard_attributes.hpp"
#include "nodeflux/sop/group_sop.hpp"
#include "nodeflux/sop/sphere_sop.hpp"
#include "nodeflux/sop/wrangle_sop.hpp"
#include <gtest/gtest.h>

using namespace nodeflux;
using namespace nodeflux::sop;
using namespace nodeflux::core;

/**
 * Test that the universal group parameter in SOPNode works correctly
 * across all SOP nodes
 */
class SOPGroupFilteringTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

/**
 * Test WrangleSOP with group filtering
 * Create a sphere, select top half with GroupSOP, then modify only those points
 */
TEST_F(SOPGroupFilteringTest, WrangleWithGroupFilter) {
  // Create a sphere
  auto sphere = std::make_shared<SphereSOP>("sphere1");
  sphere->set_parameter("radius", 1.0f);
  sphere->set_parameter("divisions", 10);
  auto sphere_result = sphere->cook();
  ASSERT_NE(sphere_result, nullptr);

  size_t total_points = sphere_result->point_count();
  ASSERT_GT(total_points, 0);

  // Create a group of points where y > 0 (top half)
  auto group = std::make_shared<GroupSOP>("group1");
  group->get_input_ports().get_port("0")->set_data(sphere_result);
  group->set_parameter("group_name", std::string("top_half"));
  group->set_parameter("element_type", 0); // Points
  group->set_parameter("group_type", 2);   // Expression-based (if available)

  // For now, manually create the group since we need expression support
  auto group_result = group->cook();
  ASSERT_NE(group_result, nullptr);

  // Manually create a group for the top half
  create_group(*group_result, "top_half", ElementClass::POINT);
  auto *pos_storage = group_result->get_point_attribute_typed<Vec3f>("P");
  ASSERT_NE(pos_storage, nullptr);

  auto pos_span = pos_storage->values();
  size_t top_half_count = 0;
  for (size_t i = 0; i < group_result->point_count(); ++i) {
    const auto &P = pos_span[i];
    if (P.y() > 0.0f) {
      add_to_group(*group_result, "top_half", ElementClass::POINT, i);
      top_half_count++;
    }
  }

  ASSERT_GT(top_half_count, 0);
  ASSERT_LT(top_half_count, total_points); // Should be less than total

  // Now use WrangleSOP with group filter to move only top half points up
  auto wrangle = std::make_shared<WrangleSOP>("wrangle1");
  wrangle->get_input_ports().get_port("0")->set_data(group_result);
  wrangle->set_parameter("expression", std::string("Py := Py + 0.5"));
  wrangle->set_parameter(
      "group", std::string("top_half")); // Use inherited group parameter!

  auto wrangle_result = wrangle->cook();
  ASSERT_NE(wrangle_result, nullptr);

  // Verify that only top half points were modified
  auto *result_pos = wrangle_result->get_point_attribute_typed<Vec3f>("P");
  ASSERT_NE(result_pos, nullptr);

  auto result_span = result_pos->values();

  size_t modified_count = 0;
  size_t unmodified_count = 0;

  for (size_t i = 0; i < wrangle_result->point_count(); ++i) {
    const auto &original_P = pos_span[i];
    const auto &result_P = result_span[i];

    bool was_in_group =
        is_in_group(*group_result, "top_half", ElementClass::POINT, i);

    if (was_in_group) {
      // Should be modified (moved up by 0.5)
      EXPECT_NEAR(result_P.y(), original_P.y() + 0.5f, 1e-5f)
          << "Point " << i << " was in group but not modified";
      modified_count++;
    } else {
      // Should be unchanged
      EXPECT_NEAR(result_P.y(), original_P.y(), 1e-5f)
          << "Point " << i << " was NOT in group but was modified";
      unmodified_count++;
    }
  }

  EXPECT_EQ(modified_count, top_half_count);
  EXPECT_EQ(unmodified_count, total_points - top_half_count);
}

/**
 * Test that when no group is specified, all elements are processed
 */
TEST_F(SOPGroupFilteringTest, WrangleWithoutGroupFilter) {
  // Create a sphere
  auto sphere = std::make_shared<SphereSOP>("sphere1");
  sphere->set_parameter("radius", 1.0f);
  sphere->set_parameter("divisions", 8);
  auto sphere_result = sphere->cook();
  ASSERT_NE(sphere_result, nullptr);

  size_t total_points = sphere_result->point_count();

  // Use WrangleSOP without group filter
  auto wrangle = std::make_shared<WrangleSOP>("wrangle1");
  wrangle->get_input_ports().get_port("0")->set_data(sphere_result);
  wrangle->set_parameter("expression", std::string("Py := Py + 1.0"));
  // Don't set group parameter - should process all points

  auto wrangle_result = wrangle->cook();
  ASSERT_NE(wrangle_result, nullptr);

  // Verify all points were modified
  auto *original_pos = sphere_result->get_point_attribute_typed<Vec3f>("P");
  auto *result_pos = wrangle_result->get_point_attribute_typed<Vec3f>("P");
  ASSERT_NE(original_pos, nullptr);
  ASSERT_NE(result_pos, nullptr);

  auto original_span = original_pos->values();
  auto result_span = result_pos->values();

  for (size_t i = 0; i < total_points; ++i) {
    const auto &original_P = original_span[i];
    const auto &result_P = result_span[i];

    EXPECT_NEAR(result_P.y(), original_P.y() + 1.0f, 1e-5f)
        << "Point " << i << " should have been modified but wasn't";
  }
}
