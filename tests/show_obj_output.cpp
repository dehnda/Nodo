#include "nodo/geometry/sphere_generator.hpp"
#include "nodo/io/obj_exporter.hpp"

#include <iostream>

using namespace nodo;

int main() {
  // Create a small sphere for demonstration
  auto sphere_container =
      geometry::SphereGenerator::generate_uv_sphere(1.0, 8, 4);

  if (!sphere_container) {
    std::cerr << "Failed to create sphere\n";
    return 1;
  }

  // Export to OBJ string
  auto obj_string = io::ObjExporter::geometry_to_obj_string(*sphere_container);

  if (obj_string) {
    std::cout << *obj_string;
  }

  return 0;
}
