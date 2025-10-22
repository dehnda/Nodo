#include "nodeflux/geometry/sphere_generator.hpp"
#include "nodeflux/io/obj_exporter.hpp"
#include <iostream>

int main() {
  using namespace nodeflux;

  // Create a small sphere for demonstration
  auto sphere = geometry::SphereGenerator::generate_uv_sphere(1.0, 8, 4);

  if (!sphere) {
    std::cerr << "Failed to create sphere\n";
    return 1;
  }

  auto obj_string = io::ObjExporter::mesh_to_obj_string(*sphere);

  if (obj_string) {
    std::cout << *obj_string;
  }

  return 0;
}
