#include <iostream>
#include <nodeflux/geometry/boolean_ops.hpp>
#include <nodeflux/geometry/cylinder_generator.hpp>
#include <nodeflux/geometry/sphere_generator.hpp>
#include <nodeflux/io/obj_exporter.hpp>

using namespace nodeflux;

int main() {
  std::cout << "NodeFluxEngine - Boolean Operations with New Primitives\n";
  std::cout << "======================================================\n\n";

  // Generate a sphere
  auto sphere = geometry::SphereGenerator::generate_uv_sphere(1.0, 24, 12);
  if (!sphere) {
    std::cerr << "Failed to generate sphere: "
              << geometry::SphereGenerator::last_error().message << "\n";
    return 1;
  }

  // Generate a cylinder that intersects the sphere
  auto cylinder =
      geometry::CylinderGenerator::generate(0.6, 2.5, 16, 1, true, true);
  if (!cylinder) {
    std::cerr << "Failed to generate cylinder: "
              << geometry::CylinderGenerator::last_error().message << "\n";
    return 1;
  }

  std::cout << "Generated sphere: " << sphere->vertices().rows()
            << " vertices, " << sphere->faces().rows() << " faces\n";
  std::cout << "Generated cylinder: " << cylinder->vertices().rows()
            << " vertices, " << cylinder->faces().rows() << " faces\n\n";

  // Export individual meshes
  io::ObjExporter::export_mesh(*sphere,
                               "examples/output/sphere_for_boolean.obj");
  io::ObjExporter::export_mesh(*cylinder,
                               "examples/output/cylinder_for_boolean.obj");

  // Test union
  std::cout << "Computing union...\n";
  auto union_result = geometry::BooleanOps::union_meshes(*sphere, *cylinder);
  if (union_result) {
    std::cout << "Union result: " << union_result->vertices().rows()
              << " vertices, " << union_result->faces().rows() << " faces\n";
    io::ObjExporter::export_mesh(*union_result,
                                 "examples/output/sphere_cylinder_union.obj");
  } else {
    std::cout << "Union failed: " << geometry::BooleanOps::last_error().message
              << "\n";
  }

  // Test intersection
  std::cout << "Computing intersection...\n";
  auto intersection_result =
      geometry::BooleanOps::intersect_meshes(*sphere, *cylinder);
  if (intersection_result) {
    std::cout << "Intersection result: "
              << intersection_result->vertices().rows() << " vertices, "
              << intersection_result->faces().rows() << " faces\n";
    io::ObjExporter::export_mesh(
        *intersection_result,
        "examples/output/sphere_cylinder_intersection.obj");
  } else {
    std::cout << "Intersection failed: "
              << geometry::BooleanOps::last_error().message << "\n";
  }

  // Test difference (sphere - cylinder)
  std::cout << "Computing difference (sphere - cylinder)...\n";
  auto difference_result =
      geometry::BooleanOps::difference_meshes(*sphere, *cylinder);
  if (difference_result) {
    std::cout << "Difference result: " << difference_result->vertices().rows()
              << " vertices, " << difference_result->faces().rows()
              << " faces\n";
    io::ObjExporter::export_mesh(*difference_result,
                                 "examples/output/sphere_minus_cylinder.obj");
  } else {
    std::cout << "Difference failed: "
              << geometry::BooleanOps::last_error().message << "\n";
  }

  std::cout << "\nBoolean operations with new primitives complete!\n";
  std::cout
      << "Check the examples/output/ directory for the generated OBJ files.\n";

  return 0;
}
