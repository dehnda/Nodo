#include <gtest/gtest.h>
#include <nodo/core/geometry_handle.hpp>
#include <nodo/core/standard_attributes.hpp>
#include <nodo/geometry/box_generator.hpp>

using namespace nodo;
namespace attrs = core::standard_attrs;

class GeometryHandleTest : public ::testing::Test {};

// Test: Empty handle
TEST_F(GeometryHandleTest, EmptyHandle) {
  core::GeometryHandle handle;
  EXPECT_FALSE(handle.is_valid());
  EXPECT_TRUE(handle.is_empty());
  EXPECT_EQ(handle.use_count(), 0);
}

// Test: Single handle ownership
TEST_F(GeometryHandleTest, SingleOwnership) {
  auto box = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box.has_value());

  core::GeometryHandle handle(std::make_shared<core::GeometryContainer>(std::move(*box)));

  EXPECT_TRUE(handle.is_valid());
  EXPECT_FALSE(handle.is_empty());
  EXPECT_EQ(handle.use_count(), 1);
  EXPECT_TRUE(handle.is_unique());
}

// Test: Shared handles increase use_count
TEST_F(GeometryHandleTest, SharedHandles) {
  auto box = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box.has_value());

  core::GeometryHandle handle1(std::make_shared<core::GeometryContainer>(std::move(*box)));
  EXPECT_EQ(handle1.use_count(), 1);

  // Share the handle
  core::GeometryHandle handle2 = handle1;
  EXPECT_EQ(handle1.use_count(), 2);
  EXPECT_EQ(handle2.use_count(), 2);
  EXPECT_FALSE(handle1.is_unique());
  EXPECT_FALSE(handle2.is_unique());

  core::GeometryHandle handle3 = handle2;
  EXPECT_EQ(handle1.use_count(), 3);
  EXPECT_EQ(handle2.use_count(), 3);
  EXPECT_EQ(handle3.use_count(), 3);
}

// Test: Read access doesn't copy
TEST_F(GeometryHandleTest, ReadAccessNoCopy) {
  auto box = geometry::BoxGenerator::generate(2.0, 1.0, 0.5);
  ASSERT_TRUE(box.has_value());

  core::GeometryHandle handle(std::make_shared<core::GeometryContainer>(std::move(*box)));
  const size_t original_point_count = handle->point_count();

  // Multiple read accesses
  for (int i = 0; i < 10; ++i) {
    const auto& geo = handle.read();
    EXPECT_EQ(geo.point_count(), original_point_count);
  }

  // Still only one owner
  EXPECT_EQ(handle.use_count(), 1);
}

// Test: Write on unique handle doesn't copy
TEST_F(GeometryHandleTest, WriteUniqueNoCopy) {
  auto box = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box.has_value());

  core::GeometryHandle handle(std::make_shared<core::GeometryContainer>(std::move(*box)));
  EXPECT_EQ(handle.use_count(), 1);

  // Get write access (should not copy since use_count == 1)
  auto& geo = handle.write();
  const void* original_ptr = &geo;

  // Write again
  auto& geo2 = handle.write();
  const void* second_ptr = &geo2;

  // Should be the same pointer (no copy)
  EXPECT_EQ(original_ptr, second_ptr);
  EXPECT_EQ(handle.use_count(), 1);
}

// Test: Write on shared handle triggers copy (COW)
TEST_F(GeometryHandleTest, WriteSharedTriggersCOW) {
  auto box = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box.has_value());

  core::GeometryHandle handle1(std::make_shared<core::GeometryContainer>(std::move(*box)));
  core::GeometryHandle handle2 = handle1;

  EXPECT_EQ(handle1.use_count(), 2);
  EXPECT_EQ(handle2.use_count(), 2);

  // Get pointers before write
  const void* handle1_ptr_before = &handle1.read();
  const void* handle2_ptr_before = &handle2.read();
  EXPECT_EQ(handle1_ptr_before, handle2_ptr_before); // Same data

  // Write to handle2 (should trigger COW)
  auto& geo2 = handle2.write();
  const void* handle2_ptr_after = &geo2;

  // handle2 should now have different data
  EXPECT_NE(handle1_ptr_before, handle2_ptr_after);

  // Use counts updated
  EXPECT_EQ(handle1.use_count(), 1);
  EXPECT_EQ(handle2.use_count(), 1);

  // Both are now unique
  EXPECT_TRUE(handle1.is_unique());
  EXPECT_TRUE(handle2.is_unique());
}

// Test: Modifications after COW don't affect original
TEST_F(GeometryHandleTest, ModificationsAfterCOW) {
  auto box = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box.has_value());

  core::GeometryHandle handle1(std::make_shared<core::GeometryContainer>(std::move(*box)));
  core::GeometryHandle handle2 = handle1;

  const size_t original_point_count = handle1->point_count();
  EXPECT_EQ(handle2->point_count(), original_point_count);

  // Modify handle2's geometry
  auto& geo2 = handle2.write(); // Triggers COW
  auto* pos_attr = geo2.get_point_attribute_typed<core::Vec3f>(attrs::P);
  ASSERT_NE(pos_attr, nullptr);

  // Scale all positions by 2
  for (size_t i = 0; i < pos_attr->size(); ++i) {
    (*pos_attr)[i] *= 2.0F;
  }

  // handle1's data should be unchanged
  const auto& geo1 = handle1.read();
  auto* pos1 = geo1.get_point_attribute_typed<core::Vec3f>(attrs::P);
  ASSERT_NE(pos1, nullptr);

  // Original positions should be in range [-0.5, 0.5] (1x1x1 box)
  for (size_t i = 0; i < pos1->size(); ++i) {
    EXPECT_LE(std::abs((*pos1)[i].x()), 0.51F);
    EXPECT_LE(std::abs((*pos1)[i].y()), 0.51F);
    EXPECT_LE(std::abs((*pos1)[i].z()), 0.51F);
  }

  // handle2's positions should be scaled (in range [-1.0, 1.0])
  for (size_t i = 0; i < pos_attr->size(); ++i) {
    EXPECT_LE(std::abs((*pos_attr)[i].x()), 1.01F);
    EXPECT_LE(std::abs((*pos_attr)[i].y()), 1.01F);
    EXPECT_LE(std::abs((*pos_attr)[i].z()), 1.01F);
  }
}

// Test: make_unique() forces copy when shared
TEST_F(GeometryHandleTest, MakeUniqueForcesCopy) {
  auto box = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box.has_value());

  core::GeometryHandle handle1(std::make_shared<core::GeometryContainer>(std::move(*box)));
  core::GeometryHandle handle2 = handle1;

  EXPECT_EQ(handle2.use_count(), 2);
  EXPECT_FALSE(handle2.is_unique());

  const void* ptr_before = &handle2.read();

  // Force copy
  handle2.make_unique();

  const void* ptr_after = &handle2.read();

  EXPECT_NE(ptr_before, ptr_after);
  EXPECT_EQ(handle2.use_count(), 1);
  EXPECT_TRUE(handle2.is_unique());
}

// Test: Clone creates independent copy
TEST_F(GeometryHandleTest, CloneCreatesIndependentCopy) {
  auto box = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box.has_value());

  core::GeometryHandle handle1(std::make_shared<core::GeometryContainer>(std::move(*box)));
  core::GeometryHandle handle2 = handle1.clone();

  // Different data
  EXPECT_NE(&handle1.read(), &handle2.read());

  // Both unique
  EXPECT_EQ(handle1.use_count(), 1);
  EXPECT_EQ(handle2.use_count(), 1);
  EXPECT_TRUE(handle1.is_unique());
  EXPECT_TRUE(handle2.is_unique());
}

// Test: Move semantics
TEST_F(GeometryHandleTest, MoveSemantics) {
  auto box = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box.has_value());

  core::GeometryHandle handle1(std::make_shared<core::GeometryContainer>(std::move(*box)));
  const void* original_ptr = &handle1.read();

  // Move construct
  core::GeometryHandle handle2(std::move(handle1));

  EXPECT_FALSE(handle1.is_valid()); // Moved-from handle is empty
  EXPECT_TRUE(handle2.is_valid());
  EXPECT_EQ(&handle2.read(), original_ptr); // Same data, no copy
  EXPECT_EQ(handle2.use_count(), 1);
}

// Test: Branching scenario (realistic use case)
TEST_F(GeometryHandleTest, BranchingScenario) {
  // Create box
  auto box = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box.has_value());
  core::GeometryHandle box_handle(std::make_shared<core::GeometryContainer>(std::move(*box)));

  // Branch A and B both receive box's output
  core::GeometryHandle branch_a = box_handle;
  core::GeometryHandle branch_b = box_handle;

  EXPECT_EQ(box_handle.use_count(), 3);

  // Branch A modifies (triggers COW)
  auto& geo_a = branch_a.write();
  EXPECT_EQ(branch_a.use_count(), 1);   // Now unique
  EXPECT_EQ(box_handle.use_count(), 2); // box and branch_b still share

  // Branch B modifies (triggers COW)
  auto& geo_b = branch_b.write();
  EXPECT_EQ(branch_b.use_count(), 1);   // Now unique
  EXPECT_EQ(box_handle.use_count(), 1); // box is now unique

  // All three have independent data
  EXPECT_NE(&box_handle.read(), &branch_a.read());
  EXPECT_NE(&box_handle.read(), &branch_b.read());
  EXPECT_NE(&branch_a.read(), &branch_b.read());
}

// Test: Linear chain scenario (zero-copy optimization)
TEST_F(GeometryHandleTest, LinearChainZeroCopy) {
  // Create box
  auto box = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
  ASSERT_TRUE(box.has_value());
  core::GeometryHandle handle(std::make_shared<core::GeometryContainer>(std::move(*box)));

  const void* original_ptr = &handle.read();

  // Simulate: Box releases, Transform receives (use_count stays 1)
  core::GeometryHandle transform_handle = std::move(handle);
  EXPECT_FALSE(handle.is_valid());
  EXPECT_EQ(transform_handle.use_count(), 1);

  // Transform writes - no copy since unique
  auto& geo = transform_handle.write();
  EXPECT_EQ(&geo, original_ptr); // Same pointer!

  // Simulate: Transform releases, Subdivide receives
  core::GeometryHandle subdivide_handle = std::move(transform_handle);
  EXPECT_EQ(subdivide_handle.use_count(), 1);

  // Subdivide writes - still no copy
  auto& geo2 = subdivide_handle.write();
  EXPECT_EQ(&geo2, original_ptr); // Still same pointer!
}
