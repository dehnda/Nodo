#include "nodeflux/core/geometry_container.hpp"
#include "nodeflux/core/standard_attributes.hpp"
#include "nodeflux/sop/extrude_sop.hpp"
#include <iostream>

using namespace nodeflux;

int main() {
  // Create a simple box-like input
  core::GeometryContainer input;
  input.set_point_count(4);

  input.add_point_attribute(core::standard_attrs::P,
                            core::AttributeType::VEC3F);
  auto *positions =
      input.get_point_attribute_typed<core::Vec3f>(core::standard_attrs::P);

  // Create a square in XY plane
  (*positions)[0] = core::Vec3f(-1.0F, -1.0F, 0.0F);
  (*positions)[1] = core::Vec3f(1.0F, -1.0F, 0.0F);
  (*positions)[2] = core::Vec3f(1.0F, 1.0F, 0.0F);
  (*positions)[3] = core::Vec3f(-1.0F, 1.0F, 0.0F);

  // Create a quad face
  input.topology().set_vertex_point(0, 0);
  input.topology().set_vertex_point(1, 1);
  input.topology().set_vertex_point(2, 2);
  input.topology().set_vertex_point(3, 3);
  input.add_primitive({0, 1, 2, 3});

  // Create and execute ExtrudeSOP
  sop::ExtrudeSOP extrude("test_extrude");
  extrude.set_parameter("distance", 2.0F);
  extrude.set_parameter("inset", 0.0F);
  extrude.set_parameter("mode", 0); // Face Normals

  // Set input
  extrude.set_input_data(0, std::make_shared<core::GeometryContainer>(input));

  // Execute
  auto result = extrude.execute();

  if (!result) {
    std::cerr << "ERROR: ExtrudeSOP execution failed!" << std::endl;
    return 1;
  }

  std::cout << "SUCCESS! ExtrudeSOP executed" << std::endl;
  std::cout << "Input: " << input.topology().point_count() << " points, "
            << input.topology().primitive_count() << " primitives" << std::endl;
  std::cout << "Output: " << result->topology().point_count() << " points, "
            << result->topology().primitive_count() << " primitives"
            << std::endl;

  // Verify output
  if (result->topology().primitive_count() > 0) {
    std::cout << "Extrusion created geometry successfully!" << std::endl;
    return 0;
  } else {
    std::cerr << "ERROR: No primitives in output!" << std::endl;
    return 1;
  }
}
