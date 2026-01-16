#include "nodo/core/attribute_descriptor.hpp"
#include "nodo/core/attribute_set.hpp"
#include "nodo/core/attribute_storage.hpp"

#include <gtest/gtest.h>

using namespace nodo::core;

// ============================================================================
// AttributeDescriptor Tests
// ============================================================================

class AttributeDescriptorTest : public ::testing::Test {};

TEST_F(AttributeDescriptorTest, BasicConstruction) {
  AttributeDescriptor desc("P", AttributeType::VEC3F, ElementClass::POINT);

  EXPECT_EQ(desc.name(), "P");
  EXPECT_EQ(desc.type(), AttributeType::VEC3F);
  EXPECT_EQ(desc.owner(), ElementClass::POINT);
  EXPECT_EQ(desc.interpolation(), InterpolationMode::LINEAR);
  EXPECT_EQ(desc.version(), 0);
}

TEST_F(AttributeDescriptorTest, DefaultInterpolation) {
  // INT should default to DISCRETE
  AttributeDescriptor id_desc("id", AttributeType::INT, ElementClass::POINT);
  EXPECT_EQ(id_desc.interpolation(), InterpolationMode::DISCRETE);

  // QUATERNION should default to QUATERNION_SLERP
  AttributeDescriptor orient_desc("orient", AttributeType::QUATERNION, ElementClass::POINT);
  EXPECT_EQ(orient_desc.interpolation(), InterpolationMode::QUATERNION_SLERP);

  // VEC3F should default to LINEAR
  AttributeDescriptor normal_desc("N", AttributeType::VEC3F, ElementClass::VERTEX);
  EXPECT_EQ(normal_desc.interpolation(), InterpolationMode::LINEAR);
}

TEST_F(AttributeDescriptorTest, ElementSize) {
  AttributeDescriptor float_desc("f", AttributeType::FLOAT, ElementClass::POINT);
  EXPECT_EQ(float_desc.element_size(), sizeof(float));

  AttributeDescriptor vec3_desc("P", AttributeType::VEC3F, ElementClass::POINT);
  EXPECT_EQ(vec3_desc.element_size(), sizeof(Vec3f));

  AttributeDescriptor mat4_desc("transform", AttributeType::MATRIX4, ElementClass::POINT);
  EXPECT_EQ(mat4_desc.element_size(), sizeof(Matrix4f));
}

TEST_F(AttributeDescriptorTest, ComponentCount) {
  AttributeDescriptor float_desc("f", AttributeType::FLOAT, ElementClass::POINT);
  EXPECT_EQ(float_desc.component_count(), 1);

  AttributeDescriptor vec3_desc("P", AttributeType::VEC3F, ElementClass::POINT);
  EXPECT_EQ(vec3_desc.component_count(), 3);

  AttributeDescriptor vec4_desc("color", AttributeType::VEC4F, ElementClass::POINT);
  EXPECT_EQ(vec4_desc.component_count(), 4);
}

TEST_F(AttributeDescriptorTest, DefaultValue) {
  AttributeDescriptor desc("N", AttributeType::VEC3F, ElementClass::VERTEX);

  EXPECT_FALSE(desc.has_default());

  Vec3f default_normal(0.0F, 0.0F, 1.0F);
  desc.set_default(default_normal);

  EXPECT_TRUE(desc.has_default());

  auto retrieved = desc.get_default<Vec3f>();
  ASSERT_TRUE(retrieved.has_value());
  EXPECT_FLOAT_EQ((*retrieved).x(), 0.0F);
  EXPECT_FLOAT_EQ((*retrieved).y(), 0.0F);
  EXPECT_FLOAT_EQ((*retrieved).z(), 1.0F);
}

TEST_F(AttributeDescriptorTest, Builder) {
  auto desc = AttributeDescriptorBuilder("Cd", AttributeType::VEC3F, ElementClass::POINT)
                  .interpolation(InterpolationMode::LINEAR)
                  .default_value(Vec3f(1.0F, 1.0F, 1.0F))
                  .build();

  EXPECT_EQ(desc.name(), "Cd");
  EXPECT_EQ(desc.type(), AttributeType::VEC3F);
  EXPECT_EQ(desc.owner(), ElementClass::POINT);
  EXPECT_EQ(desc.interpolation(), InterpolationMode::LINEAR);
  EXPECT_TRUE(desc.has_default());

  auto default_val = desc.get_default<Vec3f>();
  ASSERT_TRUE(default_val.has_value());
  EXPECT_FLOAT_EQ((*default_val).x(), 1.0F);
  EXPECT_FLOAT_EQ((*default_val).y(), 1.0F);
  EXPECT_FLOAT_EQ((*default_val).z(), 1.0F);
}

TEST_F(AttributeDescriptorTest, Versioning) {
  AttributeDescriptor desc("P", AttributeType::VEC3F, ElementClass::POINT);

  EXPECT_EQ(desc.version(), 0);

  desc.increment_version();
  EXPECT_EQ(desc.version(), 1);

  desc.increment_version();
  EXPECT_EQ(desc.version(), 2);
}

TEST_F(AttributeDescriptorTest, Equality) {
  AttributeDescriptor desc1("P", AttributeType::VEC3F, ElementClass::POINT);
  AttributeDescriptor desc2("P", AttributeType::VEC3F, ElementClass::POINT);
  AttributeDescriptor desc3("N", AttributeType::VEC3F, ElementClass::VERTEX);

  // operator== checks name only
  EXPECT_TRUE(desc1 == desc2);
  EXPECT_FALSE(desc1 == desc3);

  // equals() checks all fields
  EXPECT_TRUE(desc1.equals(desc2));
  EXPECT_FALSE(desc1.equals(desc3));
}

// ============================================================================
// AttributeStorage Tests
// ============================================================================

class AttributeStorageTest : public ::testing::Test {};

TEST_F(AttributeStorageTest, FloatStorage) {
  AttributeDescriptor desc("temperature", AttributeType::FLOAT, ElementClass::POINT);
  AttributeStorage<float> storage(desc);

  EXPECT_EQ(storage.size(), 0);

  storage.resize(10);
  EXPECT_EQ(storage.size(), 10);

  storage[0] = 25.5F;
  storage[1] = 30.0F;

  EXPECT_FLOAT_EQ(storage[0], 25.5F);
  EXPECT_FLOAT_EQ(storage[1], 30.0F);
}

TEST_F(AttributeStorageTest, Vec3fStorage) {
  AttributeDescriptor desc("P", AttributeType::VEC3F, ElementClass::POINT);
  AttributeStorage<Vec3f> storage(desc);

  storage.resize(5);
  EXPECT_EQ(storage.size(), 5);

  storage[0] = Vec3f(1.0F, 2.0F, 3.0F);
  storage[1] = Vec3f(4.0F, 5.0F, 6.0F);

  EXPECT_FLOAT_EQ(storage[0].x(), 1.0F);
  EXPECT_FLOAT_EQ(storage[0].y(), 2.0F);
  EXPECT_FLOAT_EQ(storage[0].z(), 3.0F);

  EXPECT_FLOAT_EQ(storage[1].x(), 4.0F);
  EXPECT_FLOAT_EQ(storage[1].y(), 5.0F);
  EXPECT_FLOAT_EQ(storage[1].z(), 6.0F);
}

TEST_F(AttributeStorageTest, StringStorage) {
  AttributeDescriptor desc("name", AttributeType::STRING, ElementClass::DETAIL);
  AttributeStorage<std::string> storage(desc);

  storage.resize(3);
  storage[0] = "box";
  storage[1] = "sphere";
  storage[2] = "cylinder";

  EXPECT_EQ(storage[0], "box");
  EXPECT_EQ(storage[1], "sphere");
  EXPECT_EQ(storage[2], "cylinder");
}

TEST_F(AttributeStorageTest, DefaultValue) {
  AttributeDescriptor desc("N", AttributeType::VEC3F, ElementClass::VERTEX);
  desc.set_default(Vec3f(0.0F, 0.0F, 1.0F));

  AttributeStorage<Vec3f> storage(desc);
  storage.resize(10);

  // All elements should be initialized to default
  for (size_t i = 0; i < 10; ++i) {
    EXPECT_FLOAT_EQ(storage[i].x(), 0.0F);
    EXPECT_FLOAT_EQ(storage[i].y(), 0.0F);
    EXPECT_FLOAT_EQ(storage[i].z(), 1.0F);
  }
}

TEST_F(AttributeStorageTest, SpanAccess) {
  AttributeDescriptor desc("P", AttributeType::VEC3F, ElementClass::POINT);
  AttributeStorage<Vec3f> storage(desc);

  storage.resize(5);
  auto writable = storage.values_writable();
  writable[0] = Vec3f(1.0F, 2.0F, 3.0F);
  writable[1] = Vec3f(4.0F, 5.0F, 6.0F);

  auto readonly = storage.values();
  EXPECT_EQ(readonly.size(), 5);
  EXPECT_FLOAT_EQ(readonly[0].x(), 1.0F);
  EXPECT_FLOAT_EQ(readonly[1].x(), 4.0F);
}

TEST_F(AttributeStorageTest, PushBack) {
  AttributeDescriptor desc("P", AttributeType::VEC3F, ElementClass::POINT);
  AttributeStorage<Vec3f> storage(desc);

  storage.push_back(Vec3f(1.0F, 2.0F, 3.0F));
  storage.push_back(Vec3f(4.0F, 5.0F, 6.0F));

  EXPECT_EQ(storage.size(), 2);
  EXPECT_FLOAT_EQ(storage[0].x(), 1.0F);
  EXPECT_FLOAT_EQ(storage[1].x(), 4.0F);
}

TEST_F(AttributeStorageTest, Clone) {
  AttributeDescriptor desc("P", AttributeType::VEC3F, ElementClass::POINT);
  AttributeStorage<Vec3f> storage(desc);

  storage.resize(3);
  storage[0] = Vec3f(1.0F, 2.0F, 3.0F);
  storage[1] = Vec3f(4.0F, 5.0F, 6.0F);
  storage[2] = Vec3f(7.0F, 8.0F, 9.0F);

  auto cloned = storage.clone();
  ASSERT_NE(cloned, nullptr);

  auto* typed_clone = dynamic_cast<AttributeStorage<Vec3f>*>(cloned.get());
  ASSERT_NE(typed_clone, nullptr);

  EXPECT_EQ(typed_clone->size(), 3);
  EXPECT_FLOAT_EQ((*typed_clone)[0].x(), 1.0F);
  EXPECT_FLOAT_EQ((*typed_clone)[1].x(), 4.0F);
  EXPECT_FLOAT_EQ((*typed_clone)[2].x(), 7.0F);
}

TEST_F(AttributeStorageTest, CopyElement) {
  AttributeDescriptor desc("P", AttributeType::VEC3F, ElementClass::POINT);
  AttributeStorage<Vec3f> src(desc);
  AttributeStorage<Vec3f> dst(desc);

  src.resize(3);
  dst.resize(3);

  src[0] = Vec3f(1.0F, 2.0F, 3.0F);
  src[1] = Vec3f(4.0F, 5.0F, 6.0F);
  src[2] = Vec3f(7.0F, 8.0F, 9.0F);

  dst.copy_element(1, 0, src); // Copy src[1] to dst[0]

  EXPECT_FLOAT_EQ(dst[0].x(), 4.0F);
  EXPECT_FLOAT_EQ(dst[0].y(), 5.0F);
  EXPECT_FLOAT_EQ(dst[0].z(), 6.0F);
}

TEST_F(AttributeStorageTest, SwapElements) {
  AttributeDescriptor desc("P", AttributeType::VEC3F, ElementClass::POINT);
  AttributeStorage<Vec3f> storage(desc);

  storage.resize(3);
  storage[0] = Vec3f(1.0F, 2.0F, 3.0F);
  storage[1] = Vec3f(4.0F, 5.0F, 6.0F);

  storage.swap_elements(0, 1);

  EXPECT_FLOAT_EQ(storage[0].x(), 4.0F);
  EXPECT_FLOAT_EQ(storage[1].x(), 1.0F);
}

TEST_F(AttributeStorageTest, FactoryCreation) {
  AttributeDescriptor float_desc("f", AttributeType::FLOAT, ElementClass::POINT);
  auto float_storage = create_attribute_storage(float_desc);
  EXPECT_NE(float_storage, nullptr);
  EXPECT_EQ(float_storage->descriptor().type(), AttributeType::FLOAT);

  AttributeDescriptor vec3_desc("P", AttributeType::VEC3F, ElementClass::POINT);
  auto vec3_storage = create_attribute_storage(vec3_desc);
  EXPECT_NE(vec3_storage, nullptr);
  EXPECT_EQ(vec3_storage->descriptor().type(), AttributeType::VEC3F);

  AttributeDescriptor string_desc("name", AttributeType::STRING, ElementClass::DETAIL);
  auto string_storage = create_attribute_storage(string_desc);
  EXPECT_NE(string_storage, nullptr);
  EXPECT_EQ(string_storage->descriptor().type(), AttributeType::STRING);
}

// ============================================================================
// AttributeSet Tests
// ============================================================================

class AttributeSetTest : public ::testing::Test {};

TEST_F(AttributeSetTest, BasicConstruction) {
  AttributeSet point_attrs(ElementClass::POINT);

  EXPECT_EQ(point_attrs.element_class(), ElementClass::POINT);
  EXPECT_EQ(point_attrs.size(), 0);
  EXPECT_EQ(point_attrs.attribute_count(), 0);
}

TEST_F(AttributeSetTest, AddAttribute) {
  AttributeSet point_attrs(ElementClass::POINT);

  EXPECT_TRUE(point_attrs.add_attribute("P", AttributeType::VEC3F));
  EXPECT_TRUE(point_attrs.add_attribute("Cd", AttributeType::VEC3F));
  EXPECT_TRUE(point_attrs.add_attribute("id", AttributeType::INT));

  EXPECT_EQ(point_attrs.attribute_count(), 3);
  EXPECT_TRUE(point_attrs.has_attribute("P"));
  EXPECT_TRUE(point_attrs.has_attribute("Cd"));
  EXPECT_TRUE(point_attrs.has_attribute("id"));
  EXPECT_FALSE(point_attrs.has_attribute("N"));
}

TEST_F(AttributeSetTest, AddDuplicateAttribute) {
  AttributeSet point_attrs(ElementClass::POINT);

  EXPECT_TRUE(point_attrs.add_attribute("P", AttributeType::VEC3F));
  EXPECT_FALSE(point_attrs.add_attribute("P", AttributeType::VEC3F)); // Duplicate

  EXPECT_EQ(point_attrs.attribute_count(), 1);
}

TEST_F(AttributeSetTest, RemoveAttribute) {
  AttributeSet point_attrs(ElementClass::POINT);

  point_attrs.add_attribute("P", AttributeType::VEC3F);
  point_attrs.add_attribute("Cd", AttributeType::VEC3F);

  EXPECT_TRUE(point_attrs.remove_attribute("P"));
  EXPECT_FALSE(point_attrs.has_attribute("P"));
  EXPECT_TRUE(point_attrs.has_attribute("Cd"));

  EXPECT_FALSE(point_attrs.remove_attribute("P")); // Already removed
}

TEST_F(AttributeSetTest, ResizeAllAttributes) {
  AttributeSet point_attrs(ElementClass::POINT);

  point_attrs.add_attribute("P", AttributeType::VEC3F);
  point_attrs.add_attribute("Cd", AttributeType::VEC3F);
  point_attrs.add_attribute("id", AttributeType::INT);

  point_attrs.resize(100);

  EXPECT_EQ(point_attrs.size(), 100);

  // All attributes should have the same size
  auto* pos_storage = point_attrs.get_storage("P");
  auto* color_storage = point_attrs.get_storage("Cd");
  auto* id_storage = point_attrs.get_storage("id");

  EXPECT_EQ(pos_storage->size(), 100);
  EXPECT_EQ(color_storage->size(), 100);
  EXPECT_EQ(id_storage->size(), 100);
}

TEST_F(AttributeSetTest, TypedAccess) {
  AttributeSet point_attrs(ElementClass::POINT);

  point_attrs.add_attribute("P", AttributeType::VEC3F);
  point_attrs.resize(10);

  auto* positions = point_attrs.get_storage_typed<Vec3f>("P");
  ASSERT_NE(positions, nullptr);

  (*positions)[0] = Vec3f(1.0F, 2.0F, 3.0F);
  (*positions)[1] = Vec3f(4.0F, 5.0F, 6.0F);

  EXPECT_FLOAT_EQ((*positions)[0].x(), 1.0F);
  EXPECT_FLOAT_EQ((*positions)[1].x(), 4.0F);
}

TEST_F(AttributeSetTest, TypedAccessWrongType) {
  AttributeSet point_attrs(ElementClass::POINT);

  point_attrs.add_attribute("P", AttributeType::VEC3F);

  // Try to access as wrong type
  auto* wrong_type = point_attrs.get_storage_typed<float>("P");
  EXPECT_EQ(wrong_type, nullptr);
}

TEST_F(AttributeSetTest, GetDescriptor) {
  AttributeSet point_attrs(ElementClass::POINT);

  point_attrs.add_attribute("P", AttributeType::VEC3F);

  auto desc = point_attrs.get_descriptor("P");
  ASSERT_TRUE(desc.has_value());
  EXPECT_EQ(desc->name(), "P");
  EXPECT_EQ(desc->type(), AttributeType::VEC3F);
  EXPECT_EQ(desc->owner(), ElementClass::POINT);
}

TEST_F(AttributeSetTest, AttributeNames) {
  AttributeSet point_attrs(ElementClass::POINT);

  point_attrs.add_attribute("P", AttributeType::VEC3F);
  point_attrs.add_attribute("Cd", AttributeType::VEC3F);
  point_attrs.add_attribute("id", AttributeType::INT);

  auto names = point_attrs.attribute_names();
  EXPECT_EQ(names.size(), 3);

  // Should be sorted alphabetically
  EXPECT_EQ(names[0], "Cd");
  EXPECT_EQ(names[1], "P");
  EXPECT_EQ(names[2], "id");
}

TEST_F(AttributeSetTest, Clone) {
  AttributeSet point_attrs(ElementClass::POINT);

  point_attrs.add_attribute("P", AttributeType::VEC3F);
  point_attrs.resize(3);

  auto* positions = point_attrs.get_storage_typed<Vec3f>("P");
  (*positions)[0] = Vec3f(1.0F, 2.0F, 3.0F);
  (*positions)[1] = Vec3f(4.0F, 5.0F, 6.0F);

  auto cloned = point_attrs.clone();

  EXPECT_EQ(cloned.size(), 3);
  EXPECT_EQ(cloned.attribute_count(), 1);
  EXPECT_TRUE(cloned.has_attribute("P"));

  auto* cloned_positions = cloned.get_storage_typed<Vec3f>("P");
  EXPECT_FLOAT_EQ((*cloned_positions)[0].x(), 1.0F);
  EXPECT_FLOAT_EQ((*cloned_positions)[1].x(), 4.0F);
}

TEST_F(AttributeSetTest, Merge) {
  AttributeSet set1(ElementClass::POINT);
  AttributeSet set2(ElementClass::POINT);

  set1.add_attribute("P", AttributeType::VEC3F);
  set2.add_attribute("Cd", AttributeType::VEC3F);
  set2.add_attribute("id", AttributeType::INT);

  set1.merge(set2, false);

  EXPECT_TRUE(set1.has_attribute("P"));
  EXPECT_TRUE(set1.has_attribute("Cd"));
  EXPECT_TRUE(set1.has_attribute("id"));
  EXPECT_EQ(set1.attribute_count(), 3);
}

TEST_F(AttributeSetTest, Validate) {
  AttributeSet point_attrs(ElementClass::POINT);

  point_attrs.add_attribute("P", AttributeType::VEC3F);
  point_attrs.add_attribute("Cd", AttributeType::VEC3F);
  point_attrs.resize(10);

  EXPECT_TRUE(point_attrs.validate());
}

TEST_F(AttributeSetTest, MemoryUsage) {
  AttributeSet point_attrs(ElementClass::POINT);

  point_attrs.add_attribute("P", AttributeType::VEC3F);
  point_attrs.add_attribute("Cd", AttributeType::VEC3F);
  point_attrs.resize(100);

  size_t mem = point_attrs.memory_usage();
  EXPECT_GT(mem, 0);

  // Should be at least 100 * sizeof(Vec3f) * 2 attributes
  size_t expected_min = 100 * sizeof(Vec3f) * 2;
  EXPECT_GE(mem, expected_min);
}

TEST_F(AttributeSetTest, Clear) {
  AttributeSet point_attrs(ElementClass::POINT);

  point_attrs.add_attribute("P", AttributeType::VEC3F);
  point_attrs.resize(100);

  point_attrs.clear();

  EXPECT_EQ(point_attrs.size(), 0);
  EXPECT_EQ(point_attrs.attribute_count(), 1); // Attributes still exist

  auto* storage = point_attrs.get_storage("P");
  EXPECT_EQ(storage->size(), 0);
}

TEST_F(AttributeSetTest, ClearAll) {
  AttributeSet point_attrs(ElementClass::POINT);

  point_attrs.add_attribute("P", AttributeType::VEC3F);
  point_attrs.resize(100);

  point_attrs.clear_all();

  EXPECT_EQ(point_attrs.size(), 0);
  EXPECT_EQ(point_attrs.attribute_count(), 0); // No attributes
}
