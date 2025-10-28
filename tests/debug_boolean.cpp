#include "nodo/sop/boolean_sop.hpp"
#include "nodo/geometry/box_generator.hpp"
#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"
#include <iostream>

using namespace nodo;

int main() {
  // Generate two boxes
  auto box1_result = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
  auto box2_result = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);

  if (!box1_result.has_value()) {
    std::cout << "Failed to generate box1\n";
    return 1;
  }

  if (!box2_result.has_value()) {
    std::cout << "Failed to generate box2\n";
    return 1;
  }

  auto geo1 = std::make_shared<core::GeometryContainer>(box1_result.value().clone());
  auto geo2 = std::make_shared<core::GeometryContainer>(box2_result.value().clone());

  std::cout << "Box1 points: " << geo1->topology().point_count() << "\n";
  std::cout << "Box1 primitives: " << geo1->topology().primitive_count() << "\n";

  std::cout << "Box2 points: " << geo2->topology().point_count() << "\n";
  std::cout << "Box2 primitives: " << geo2->topology().primitive_count() << "\n";

  // Create boolean node
  auto boolean_node = std::make_shared<sop::BooleanSOP>("test_boolean");

  // Connect inputs
  auto *port_a = boolean_node->get_input_ports().get_port("mesh_a");
  auto *port_b = boolean_node->get_input_ports().get_port("mesh_b");

  if (!port_a) {
    std::cout << "Port A not found!\n";
    return 1;
  }

  if (!port_b) {
    std::cout << "Port B not found!\n";
    return 1;
  }

  port_a->set_data(geo1);
  port_b->set_data(geo2);

  std::cout << "Ports connected\n";

  // Set operation to UNION
  boolean_node->set_parameter("operation", 0);
  std::cout << "Parameter set\n";

  // Execute
  auto result = boolean_node->cook();

  if (!result) {
    std::cout << "Boolean operation failed! (no error details available)\n";
    return 1;
  }

  std::cout << "Success! Result points: " << result->topology().point_count() << "\n";
  std::cout << "Result primitives: " << result->topology().primitive_count() << "\n";

  return 0;
}
