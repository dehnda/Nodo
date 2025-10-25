#include "nodeflux/core/attribute_types.hpp"
#include "nodeflux/core/standard_attributes.hpp"
#include <gtest/gtest.h>

using namespace nodeflux::core;

class AttributeTypesTest : public ::testing::Test {};

TEST_F(AttributeTypesTest, SizeOfTypes) {
  using namespace attribute_traits;

  EXPECT_EQ(size_of(AttributeType::FLOAT), sizeof(float));
  EXPECT_EQ(size_of(AttributeType::INT), sizeof(int));
  EXPECT_EQ(size_of(AttributeType::VEC2F), sizeof(Eigen::Vector2f));
  EXPECT_EQ(size_of(AttributeType::VEC3F), sizeof(Eigen::Vector3f));
  EXPECT_EQ(size_of(AttributeType::VEC4F), sizeof(Eigen::Vector4f));
  EXPECT_EQ(size_of(AttributeType::MATRIX3), sizeof(Eigen::Matrix3f));
  EXPECT_EQ(size_of(AttributeType::MATRIX4), sizeof(Eigen::Matrix4f));
  EXPECT_EQ(size_of(AttributeType::QUATERNION), sizeof(Eigen::Quaternionf));
  EXPECT_EQ(size_of(AttributeType::STRING), sizeof(std::string));
}

TEST_F(AttributeTypesTest, ComponentCounts) {
  using namespace attribute_traits;

  EXPECT_EQ(component_count(AttributeType::FLOAT), 1);
  EXPECT_EQ(component_count(AttributeType::INT), 1);
  EXPECT_EQ(component_count(AttributeType::VEC2F), 2);
  EXPECT_EQ(component_count(AttributeType::VEC3F), 3);
  EXPECT_EQ(component_count(AttributeType::VEC4F), 4);
  EXPECT_EQ(component_count(AttributeType::QUATERNION), 4);
  EXPECT_EQ(component_count(AttributeType::MATRIX3), 9);
  EXPECT_EQ(component_count(AttributeType::MATRIX4), 16);
  EXPECT_EQ(component_count(AttributeType::STRING), 0);
}

TEST_F(AttributeTypesTest, DefaultInterpolation) {
  using namespace attribute_traits;

  EXPECT_EQ(default_interpolation(AttributeType::FLOAT),
            InterpolationMode::LINEAR);
  EXPECT_EQ(default_interpolation(AttributeType::VEC3F),
            InterpolationMode::LINEAR);
  EXPECT_EQ(default_interpolation(AttributeType::INT),
            InterpolationMode::DISCRETE);
  EXPECT_EQ(default_interpolation(AttributeType::STRING),
            InterpolationMode::DISCRETE);
  EXPECT_EQ(default_interpolation(AttributeType::QUATERNION),
            InterpolationMode::QUATERNION_SLERP);
}

TEST_F(AttributeTypesTest, TypeChecks) {
  using namespace attribute_traits;

  // Numeric checks
  EXPECT_TRUE(is_numeric(AttributeType::FLOAT));
  EXPECT_TRUE(is_numeric(AttributeType::INT));
  EXPECT_TRUE(is_numeric(AttributeType::VEC3F));
  EXPECT_TRUE(is_numeric(AttributeType::MATRIX4));
  EXPECT_FALSE(is_numeric(AttributeType::STRING));

  // Vector checks
  EXPECT_TRUE(is_vector(AttributeType::VEC2F));
  EXPECT_TRUE(is_vector(AttributeType::VEC3F));
  EXPECT_TRUE(is_vector(AttributeType::VEC4F));
  EXPECT_FALSE(is_vector(AttributeType::FLOAT));
  EXPECT_FALSE(is_vector(AttributeType::MATRIX3));

  // Matrix checks
  EXPECT_TRUE(is_matrix(AttributeType::MATRIX3));
  EXPECT_TRUE(is_matrix(AttributeType::MATRIX4));
  EXPECT_FALSE(is_matrix(AttributeType::FLOAT));
  EXPECT_FALSE(is_matrix(AttributeType::VEC3F));
}

TEST_F(AttributeTypesTest, TypeNames) {
  using namespace attribute_traits;

  EXPECT_STREQ(type_name(AttributeType::FLOAT), "float");
  EXPECT_STREQ(type_name(AttributeType::INT), "int");
  EXPECT_STREQ(type_name(AttributeType::VEC2F), "vec2f");
  EXPECT_STREQ(type_name(AttributeType::VEC3F), "vec3f");
  EXPECT_STREQ(type_name(AttributeType::VEC4F), "vec4f");
  EXPECT_STREQ(type_name(AttributeType::MATRIX3), "matrix3");
  EXPECT_STREQ(type_name(AttributeType::MATRIX4), "matrix4");
  EXPECT_STREQ(type_name(AttributeType::QUATERNION), "quaternion");
  EXPECT_STREQ(type_name(AttributeType::STRING), "string");
}

TEST_F(AttributeTypesTest, ElementClassNames) {
  using namespace attribute_traits;

  EXPECT_STREQ(element_class_name(ElementClass::POINT), "point");
  EXPECT_STREQ(element_class_name(ElementClass::VERTEX), "vertex");
  EXPECT_STREQ(element_class_name(ElementClass::PRIMITIVE), "primitive");
  EXPECT_STREQ(element_class_name(ElementClass::DETAIL), "detail");
}

TEST_F(AttributeTypesTest, InterpolationModeNames) {
  using namespace attribute_traits;

  EXPECT_STREQ(interpolation_mode_name(InterpolationMode::LINEAR), "linear");
  EXPECT_STREQ(interpolation_mode_name(InterpolationMode::DISCRETE),
               "discrete");
  EXPECT_STREQ(interpolation_mode_name(InterpolationMode::QUATERNION_SLERP),
               "quaternion_slerp");
  EXPECT_STREQ(interpolation_mode_name(InterpolationMode::SMOOTH), "smooth");
}

// Standard attributes tests
TEST_F(AttributeTypesTest, StandardAttributeNames) {
  using namespace standard_attrs;

  EXPECT_EQ(P, "P");
  EXPECT_EQ(N, "N");
  EXPECT_EQ(Cd, "Cd");
  EXPECT_EQ(uv, "uv");
  EXPECT_EQ(v, "v");
  EXPECT_EQ(id, "id");
  EXPECT_EQ(material_id, "material_id");
  EXPECT_EQ(instance_id, "instance_id");
}

TEST_F(AttributeTypesTest, StandardAttributeInfo) {
  using namespace standard_attr_registry;

  // Check P (position)
  EXPECT_EQ(P.name, "P");
  EXPECT_EQ(P.type, AttributeType::VEC3F);
  EXPECT_EQ(P.element_class, ElementClass::POINT);
  EXPECT_EQ(P.interpolation, InterpolationMode::LINEAR);

  // Check N (normal)
  EXPECT_EQ(N.name, "N");
  EXPECT_EQ(N.type, AttributeType::VEC3F);
  EXPECT_EQ(N.element_class, ElementClass::VERTEX);
  EXPECT_EQ(N.interpolation, InterpolationMode::LINEAR);

  // Check uv
  EXPECT_EQ(uv.name, "uv");
  EXPECT_EQ(uv.type, AttributeType::VEC2F);
  EXPECT_EQ(uv.element_class, ElementClass::VERTEX);

  // Check id (discrete interpolation)
  EXPECT_EQ(id.name, "id");
  EXPECT_EQ(id.type, AttributeType::INT);
  EXPECT_EQ(id.element_class, ElementClass::POINT);
  EXPECT_EQ(id.interpolation, InterpolationMode::DISCRETE);

  // Check orient (quaternion slerp)
  EXPECT_EQ(orient.name, "orient");
  EXPECT_EQ(orient.type, AttributeType::QUATERNION);
  EXPECT_EQ(orient.element_class, ElementClass::POINT);
  EXPECT_EQ(orient.interpolation, InterpolationMode::QUATERNION_SLERP);
}

TEST_F(AttributeTypesTest, IsStandardAttribute) {
  EXPECT_TRUE(is_standard_attribute("P"));
  EXPECT_TRUE(is_standard_attribute("N"));
  EXPECT_TRUE(is_standard_attribute("Cd"));
  EXPECT_TRUE(is_standard_attribute("uv"));
  EXPECT_TRUE(is_standard_attribute("v"));
  EXPECT_TRUE(is_standard_attribute("Alpha"));
  EXPECT_TRUE(is_standard_attribute("pscale"));
  EXPECT_TRUE(is_standard_attribute("id"));

  EXPECT_FALSE(is_standard_attribute("custom_attr"));
  EXPECT_FALSE(is_standard_attribute("my_attribute"));
  EXPECT_FALSE(is_standard_attribute(""));
}

TEST_F(AttributeTypesTest, TypeAliases) {
  // Verify type aliases work correctly
  Vec2f v2(1.0F, 2.0F);
  Vec3f v3(1.0F, 2.0F, 3.0F);
  Vec4f v4(1.0F, 2.0F, 3.0F, 4.0F);

  EXPECT_EQ(v2.size(), 2);
  EXPECT_EQ(v3.size(), 3);
  EXPECT_EQ(v4.size(), 4);

  Matrix3f m3 = Matrix3f::Identity();
  Matrix4f m4 = Matrix4f::Identity();

  EXPECT_EQ(m3(0, 0), 1.0F);
  EXPECT_EQ(m4(0, 0), 1.0F);

  Quaternionf q = Quaternionf::Identity();
  EXPECT_FLOAT_EQ(q.w(), 1.0F);
  EXPECT_FLOAT_EQ(q.x(), 0.0F);
  EXPECT_FLOAT_EQ(q.y(), 0.0F);
  EXPECT_FLOAT_EQ(q.z(), 0.0F);
}
