#include <iostream>
#include <nodeflux/geometry/cylinder_generator.hpp>
#include <nodeflux/geometry/plane_generator.hpp>
#include <nodeflux/geometry/sphere_generator.hpp>
#include <nodeflux/io/obj_exporter.hpp>

using namespace nodeflux;

int main() {
  std::cout << "NodeFluxEngine - Primitive Generators Demo\n";
  std::cout << "=========================================\n\n";

  // Test sphere generation
  {
    std::cout << "Generating UV Sphere...\n";
    auto sphere = geometry::SphereGenerator::generate_uv_sphere(1.0, 16, 8);
    if (sphere) {
      std::cout << "  UV Sphere: " << sphere->vertices().rows() << " vertices, "
                << sphere->faces().rows() << " faces\n";

      if (auto result = io::ObjExporter::export_mesh(
              *sphere, "examples/output/uv_sphere.obj")) {
        std::cout << "  Exported to examples/output/uv_sphere.obj\n";
      }
    } else {
      std::cout << "  Error: "
                << geometry::SphereGenerator::last_error().message << "\n";
    }

    std::cout << "Generating Icosphere...\n";
    auto icosphere = geometry::SphereGenerator::generate_icosphere(1.0, 2);
    if (icosphere) {
      std::cout << "  Icosphere: " << icosphere->vertices().rows()
                << " vertices, " << icosphere->faces().rows() << " faces\n";

      if (auto result = io::ObjExporter::export_mesh(
              *icosphere, "examples/output/icosphere.obj")) {
        std::cout << "  Exported to examples/output/icosphere.obj\n";
      }
    }
  }

  std::cout << "\n";

  // Test cylinder generation
  {
    std::cout << "Generating Cylinder...\n";
    auto cylinder =
        geometry::CylinderGenerator::generate(1.0, 2.0, 16, 1, true, true);
    if (cylinder) {
      std::cout << "  Cylinder: " << cylinder->vertices().rows()
                << " vertices, " << cylinder->faces().rows() << " faces\n";

      if (auto result = io::ObjExporter::export_mesh(
              *cylinder, "examples/output/cylinder.obj")) {
        std::cout << "  Exported to examples/output/cylinder.obj\n";
      }
    } else {
      std::cout << "  Error: "
                << geometry::CylinderGenerator::last_error().message << "\n";
    }
  }

  std::cout << "\n";

  // Test plane generation
  {
    std::cout << "Generating Plane...\n";
    auto plane = geometry::PlaneGenerator::generate(4.0, 4.0, 4, 4);
    if (plane) {
      std::cout << "  Plane: " << plane->vertices().rows() << " vertices, "
                << plane->faces().rows() << " faces\n";

      if (auto result = io::ObjExporter::export_mesh(
              *plane, "examples/output/plane.obj")) {
        std::cout << "  Exported to examples/output/plane.obj\n";
      }
    } else {
      std::cout << "  Error: " << geometry::PlaneGenerator::last_error().message
                << "\n";
    }
  }

  std::cout << "\nPrimitive generation complete!\n";
  std::cout
      << "Check the examples/output/ directory for the generated OBJ files.\n";

  return 0;
}
