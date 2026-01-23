#include "nodo/core/attribute_group.hpp"
#include "nodo/sop/box_sop.hpp"
#include "nodo/sop/delete_sop.hpp"

#include <gtest/gtest.h>

using namespace nodo;

class DeleteSOPTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a simple box for testing
    auto box_sop = std::make_shared<sop::BoxSOP>("test_box");
    box_sop->set_dimensions(2.0F, 2.0F, 2.0F);
    box_sop->set_segments(1, 1, 1);
    test_geo_ = box_sop->cook();
    ASSERT_NE(test_geo_, nullptr);
  }

  std::shared_ptr<core::GeometryContainer> test_geo_;
};

TEST_F(DeleteSOPTest, BasicConstruction) {
  auto delete_node = std::make_shared<sop::DeleteSOP>("test_delete");
  EXPECT_TRUE(delete_node != nullptr);

  // Check parameters exist
  EXPECT_TRUE(delete_node->has_parameter("class"));
  EXPECT_TRUE(delete_node->has_parameter("operation"));
  EXPECT_TRUE(delete_node->has_parameter("group"));
  EXPECT_TRUE(delete_node->has_parameter("pattern_mode"));
  EXPECT_TRUE(delete_node->has_parameter("cleanup"));
}

TEST_F(DeleteSOPTest, DeleteAllPrimitives) {
  auto delete_node = std::make_shared<sop::DeleteSOP>("test_delete");

  // Connect input
  delete_node->set_input_data(0, test_geo_);

  // Set to delete all primitives
  delete_node->set_parameter("class", 1);     // Primitives
  delete_node->set_parameter("operation", 2); // Delete All

  auto output = delete_node->cook();
  ASSERT_NE(output, nullptr);

  EXPECT_EQ(output->primitive_count(), 0);
  EXPECT_GT(output->point_count(), 0); // Points still exist
}

TEST_F(DeleteSOPTest, DeleteByRange) {
  auto delete_node = std::make_shared<sop::DeleteSOP>("test_delete");

  // Connect input
  delete_node->set_input_data(0, test_geo_);

  size_t initial_prim_count = test_geo_->primitive_count();

  // Delete primitives by range
  delete_node->set_parameter("class", 1);        // Primitives
  delete_node->set_parameter("operation", 0);    // Delete Selected
  delete_node->set_parameter("pattern_mode", 1); // Range
  delete_node->set_parameter("range_start", 0);
  delete_node->set_parameter("range_end", 2); // Delete first 3 prims

  auto output = delete_node->cook();
  ASSERT_NE(output, nullptr);

  EXPECT_EQ(output->primitive_count(), initial_prim_count - 3);
}

TEST_F(DeleteSOPTest, DeleteEveryNth) {
  auto delete_node = std::make_shared<sop::DeleteSOP>("test_delete");

  // Connect input
  delete_node->set_input_data(0, test_geo_);

  // Delete every 2nd primitive
  delete_node->set_parameter("class", 1);        // Primitives
  delete_node->set_parameter("operation", 0);    // Delete Selected
  delete_node->set_parameter("pattern_mode", 2); // Every Nth
  delete_node->set_parameter("nth_step", 2);
  delete_node->set_parameter("nth_offset", 0);

  auto output = delete_node->cook();
  ASSERT_NE(output, nullptr);

  // Should delete roughly half the primitives
  EXPECT_LT(output->primitive_count(), test_geo_->primitive_count());
}

TEST_F(DeleteSOPTest, DeleteByGroup) {
  // Create a group first
  core::create_group(*test_geo_, "test_group", core::ElementClass::PRIMITIVE);
  core::add_to_group(*test_geo_, "test_group", core::ElementClass::PRIMITIVE, 0);
  core::add_to_group(*test_geo_, "test_group", core::ElementClass::PRIMITIVE, 1);

  auto delete_node = std::make_shared<sop::DeleteSOP>("test_delete");
  delete_node->set_input_data(0, test_geo_);

  size_t initial_count = test_geo_->primitive_count();

  // Delete primitives in group
  delete_node->set_parameter("class", 1);     // Primitives
  delete_node->set_parameter("operation", 0); // Delete Selected
  delete_node->set_parameter("group", "test_group");

  auto output = delete_node->cook();
  ASSERT_NE(output, nullptr);

  EXPECT_EQ(output->primitive_count(), initial_count - 2);
}

TEST_F(DeleteSOPTest, DeleteNonSelected) {
  // Create a group
  core::create_group(*test_geo_, "keep_group", core::ElementClass::PRIMITIVE);
  core::add_to_group(*test_geo_, "keep_group", core::ElementClass::PRIMITIVE, 0);

  auto delete_node = std::make_shared<sop::DeleteSOP>("test_delete");
  delete_node->set_input_data(0, test_geo_);

  // Delete everything NOT in the group (keep only element 0)
  delete_node->set_parameter("class", 1);     // Primitives
  delete_node->set_parameter("operation", 1); // Delete Non-Selected
  delete_node->set_parameter("group", "keep_group");

  auto output = delete_node->cook();
  ASSERT_NE(output, nullptr);

  EXPECT_EQ(output->primitive_count(), 1);
}
