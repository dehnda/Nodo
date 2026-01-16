#include "nodo/core/attribute_types.hpp"
#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/sop/wrangle_sop.hpp"

#include <gtest/gtest.h>

using namespace nodo::core;
using namespace nodo::sop;

class WrangleSOPTest : public ::testing::Test {
protected:
  void SetUp() override { wrangle_sop = std::make_shared<WrangleSOP>("wrangle_test"); }

  std::shared_ptr<WrangleSOP> wrangle_sop;
};

TEST_F(WrangleSOPTest, SimplePositionOffset) {
  // Create a simple box geometry
  auto box_geo = std::make_shared<GeometryContainer>();
  box_geo->set_point_count(8);

  // Add position attribute and fill it
  box_geo->add_point_attribute(standard_attrs::P, AttributeType::VEC3F);
  auto* positions = box_geo->get_point_attribute_typed<Vec3f>(standard_attrs::P);
  auto pos_span = positions->values_writable();
  pos_span[0] = Vec3f(-0.5f, -0.5f, -0.5f);
  pos_span[1] = Vec3f(0.5f, -0.5f, -0.5f);
  pos_span[2] = Vec3f(-0.5f, 0.5f, -0.5f);
  pos_span[3] = Vec3f(0.5f, 0.5f, -0.5f);
  pos_span[4] = Vec3f(-0.5f, -0.5f, 0.5f);
  pos_span[5] = Vec3f(0.5f, -0.5f, 0.5f);
  pos_span[6] = Vec3f(-0.5f, 0.5f, 0.5f);
  pos_span[7] = Vec3f(0.5f, 0.5f, 0.5f);

  // Connect input
  auto* input_port = wrangle_sop->get_input_ports().get_port("0");
  ASSERT_NE(input_port, nullptr);
  input_port->set_data(box_geo);

  // Set expression to offset Y position
  wrangle_sop->set_parameter("run_over", 0); // Points mode
  wrangle_sop->set_parameter("expression", std::string("Py = Py + 0.5"));

  // Execute
  auto result = wrangle_sop->cook();

  ASSERT_NE(result, nullptr);
  EXPECT_GT(result->point_count(), 0);

  // Verify positions were modified
  auto* result_positions = result->get_point_attribute_typed<Vec3f>(standard_attrs::P);
  ASSERT_NE(result_positions, nullptr);

  // Check that Y values were offset
  auto result_span = result_positions->values();
  for (size_t i = 0; i < result_span.size(); ++i) {
    const auto& pos = result_span[i];
    // Original box vertices are at -0.5 to 0.5, so after +0.5 they should be
    // 0.0 to 1.0
    EXPECT_GE(pos.y(), -0.01f); // Small tolerance
    EXPECT_LE(pos.y(), 1.01f);
  }
}

TEST_F(WrangleSOPTest, PointNumberAccess) {
  // Create simple geometry
  auto geo = std::make_shared<GeometryContainer>();
  geo->set_point_count(5);

  geo->add_point_attribute(standard_attrs::P, AttributeType::VEC3F);
  auto* positions = geo->get_point_attribute_typed<Vec3f>(standard_attrs::P);
  auto pos_span = positions->values_writable();
  for (int i = 0; i < 5; ++i) {
    pos_span[i] = Vec3f(float(i), 0.0f, 0.0f);
  }

  auto* input_port = wrangle_sop->get_input_ports().get_port("0");
  ASSERT_NE(input_port, nullptr);
  input_port->set_data(geo);

  // Set expression using @ptnum
  wrangle_sop->set_parameter("run_over", 0);
  wrangle_sop->set_parameter("expression", std::string("Py = @ptnum * 0.5"));

  auto result = wrangle_sop->cook();

  ASSERT_NE(result, nullptr);
  auto* result_positions = result->get_point_attribute_typed<Vec3f>(standard_attrs::P);
  ASSERT_NE(result_positions, nullptr);

  // Verify Y positions match point numbers
  auto result_span = result_positions->values();
  for (size_t i = 0; i < result_span.size(); ++i) {
    const auto& pos = result_span[i];
    EXPECT_NEAR(pos.y(), i * 0.5f, 0.01f);
  }
}

TEST_F(WrangleSOPTest, ColorAttribute) {
  // Create simple geometry
  auto geo = std::make_shared<GeometryContainer>();
  geo->set_point_count(3);

  geo->add_point_attribute(standard_attrs::P, AttributeType::VEC3F);
  auto* positions = geo->get_point_attribute_typed<Vec3f>(standard_attrs::P);
  auto pos_span = positions->values_writable();
  pos_span[0] = Vec3f(0.0f, 0.0f, 0.0f);
  pos_span[1] = Vec3f(1.0f, 0.0f, 0.0f);
  pos_span[2] = Vec3f(2.0f, 0.0f, 0.0f);

  auto* input_port = wrangle_sop->get_input_ports().get_port("0");
  ASSERT_NE(input_port, nullptr);
  input_port->set_data(geo);

  // Set expression to create color
  wrangle_sop->set_parameter("run_over", 0);
  wrangle_sop->set_parameter("expression", std::string("Cr = 1.0; Cg = 0.5; Cb = 0.0"));

  auto result = wrangle_sop->cook();

  ASSERT_NE(result, nullptr);

  // Verify color attribute was created
  auto* colors = result->get_point_attribute_typed<Vec3f>(standard_attrs::Cd);
  ASSERT_NE(colors, nullptr);

  // Check color values
  auto color_span = colors->values();
  for (size_t i = 0; i < color_span.size(); ++i) {
    const auto& color = color_span[i];
    EXPECT_NEAR(color.x(), 1.0f, 0.01f);
    EXPECT_NEAR(color.y(), 0.5f, 0.01f);
    EXPECT_NEAR(color.z(), 0.0f, 0.01f);
  }
}

TEST_F(WrangleSOPTest, NoInputReturnsNull) {
  // Don't connect any input
  wrangle_sop->set_parameter("run_over", 0);
  wrangle_sop->set_parameter("expression", std::string("Py = Py + 1.0"));

  auto result = wrangle_sop->cook();

  // Should return null when no input connected
  EXPECT_EQ(result, nullptr);
}

TEST_F(WrangleSOPTest, ExpressionError) {
  // Create geometry
  auto geo = std::make_shared<GeometryContainer>();
  geo->set_point_count(3);

  geo->add_point_attribute(standard_attrs::P, AttributeType::VEC3F);
  auto* positions = geo->get_point_attribute_typed<Vec3f>(standard_attrs::P);
  auto pos_span = positions->values_writable();
  pos_span[0] = Vec3f(0.0f, 0.0f, 0.0f);
  pos_span[1] = Vec3f(1.0f, 0.0f, 0.0f);
  pos_span[2] = Vec3f(2.0f, 0.0f, 0.0f);

  auto* input_port = wrangle_sop->get_input_ports().get_port("0");
  ASSERT_NE(input_port, nullptr);
  input_port->set_data(geo);

  // Set invalid expression
  wrangle_sop->set_parameter("run_over", 0);
  wrangle_sop->set_parameter("expression", std::string("this is not valid syntax"));

  auto result = wrangle_sop->cook();

  // Should still return geometry (pass-through on error)
  ASSERT_NE(result, nullptr);
}
