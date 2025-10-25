#include "nodeflux/geometry/sphere_generator.hpp"
#include "nodeflux/io/obj_exporter.hpp"
#include <iostream>

using namespace nodeflux;

// Helper to convert GeometryContainer to Mesh
static core::Mesh container_to_mesh(const core::GeometryContainer &container) {
  const auto &topology = container.topology();

  // Extract positions
  auto *p_storage =
      container.get_point_attribute_typed<core::Vec3f>(core::standard_attrs::P);
  if (!p_storage)
    return core::Mesh();

  Eigen::MatrixXd vertices(topology.point_count(), 3);
  auto p_span = p_storage->values();
  for (size_t i = 0; i < p_span.size(); ++i) {
    vertices.row(i) = p_span[i].cast<double>();
  }

  // Extract faces
  Eigen::MatrixXi faces(topology.primitive_count(), 3);
  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &verts = topology.get_primitive_vertices(prim_idx);
    for (size_t j = 0; j < 3 && j < verts.size(); ++j) {
      faces(prim_idx, j) = verts[j];
    }
  }

  return core::Mesh(vertices, faces);
}

int main() {
  // Create a small sphere for demonstration
  auto sphere_container = geometry::SphereGenerator::generate_uv_sphere(1.0, 8, 4);

  if (!sphere_container) {
    std::cerr << "Failed to create sphere\n";
    return 1;
  }

  // Convert to Mesh for OBJ export
  auto sphere_mesh = container_to_mesh(*sphere_container);
  auto obj_string = io::ObjExporter::mesh_to_obj_string(sphere_mesh);

  if (obj_string) {
    std::cout << *obj_string;
  }

  return 0;
}
