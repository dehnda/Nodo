#include <gtest/gtest.h>
#include <nodo/graph/node_graph.hpp>

TEST(NodeNamingTest, NodesShouldHaveDescriptiveNames) {
  nodo::graph::NodeGraph graph;

  // Add some nodes without specifying names
  int sphere1_id = graph.add_node(nodo::graph::NodeType::Sphere);
  int sphere2_id = graph.add_node(nodo::graph::NodeType::Sphere);
  int box_id = graph.add_node(nodo::graph::NodeType::Box);
  int transform_id = graph.add_node(nodo::graph::NodeType::Transform);

  // Check names are descriptive and unique
  auto* sphere1 = graph.get_node(sphere1_id);
  auto* sphere2 = graph.get_node(sphere2_id);
  auto* box = graph.get_node(box_id);
  auto* transform = graph.get_node(transform_id);

  ASSERT_NE(sphere1, nullptr);
  ASSERT_NE(sphere2, nullptr);
  ASSERT_NE(box, nullptr);
  ASSERT_NE(transform, nullptr);

  // Names should be based on node type
  std::string sphere1_name = sphere1->get_name();
  std::string sphere2_name = sphere2->get_name();
  std::string box_name = box->get_name();
  std::string transform_name = transform->get_name();

  std::cout << "Sphere 1 name: " << sphere1_name << std::endl;
  std::cout << "Sphere 2 name: " << sphere2_name << std::endl;
  std::cout << "Box name: " << box_name << std::endl;
  std::cout << "Transform name: " << transform_name << std::endl;

  // First sphere should be "Sphere" (metadata name)
  EXPECT_EQ(sphere1_name, "Sphere");

  // Second sphere should have a number suffix
  EXPECT_EQ(sphere2_name, "Sphere1");

  // Box should be "Box"
  EXPECT_EQ(box_name, "Box");

  // Transform should be "Transform"
  EXPECT_EQ(transform_name, "Transform");

  // All names should be unique
  std::set<std::string> names = {sphere1_name, sphere2_name, box_name, transform_name};
  EXPECT_EQ(names.size(), 4);
}
