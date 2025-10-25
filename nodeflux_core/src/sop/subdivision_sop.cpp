#include "nodeflux/core/geometry_container.hpp"
#include "nodeflux/core/math.hpp"
#include "nodeflux/core/standard_attributes.hpp"
#include "nodeflux/core/types.hpp"
#include "nodeflux/sop/subdivisions_sop.hpp"
#include <vector>

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::sop {

// Helper to convert GeometryContainer to Mesh for subdivision
static core::Mesh container_to_mesh(const core::GeometryContainer &container) {
  const auto &topology = container.topology();

  auto *p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (!p_storage)
    return core::Mesh();

  Eigen::MatrixXd vertices(topology.point_count(), 3);
  auto p_span = p_storage->values();
  for (size_t i = 0; i < p_span.size(); ++i) {
    vertices.row(i) = p_span[i].cast<double>();
  }

  Eigen::MatrixXi faces(topology.primitive_count(), 3);
  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &vert_indices = topology.get_primitive_vertices(prim_idx);
    for (size_t j = 0; j < 3 && j < vert_indices.size(); ++j) {
      faces(prim_idx, j) = topology.get_vertex_point(vert_indices[j]);
    }
  }

  return core::Mesh(vertices, faces);
}

// Helper to convert Mesh back to GeometryContainer
static core::GeometryContainer mesh_to_container(const core::Mesh &mesh) {
  core::GeometryContainer container;
  const auto &vertices = mesh.vertices();
  const auto &faces = mesh.faces();

  container.set_point_count(vertices.rows());

  // Build topology
  size_t vert_idx = 0;
  for (int face_idx = 0; face_idx < faces.rows(); ++face_idx) {
    std::vector<int> prim_verts;
    for (int j = 0; j < faces.cols(); ++j) {
      int point_idx = faces(face_idx, j);
      container.topology().set_vertex_point(vert_idx, point_idx);
      prim_verts.push_back(static_cast<int>(vert_idx));
      ++vert_idx;
    }
    container.add_primitive(prim_verts);
  }

  // Copy positions
  container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *positions = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (positions) {
    for (int i = 0; i < vertices.rows(); ++i) {
      (*positions)[i] = vertices.row(i).cast<float>();
    }
  }

  return container;
}

SubdivisionSOP::SubdivisionSOP(const std::string &name)
    : SOPNode(name, "Subdivision") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_int_parameter("subdivision_levels", 1)
                         .label("Subdivision Levels")
                         .range(0, 5)
                         .category("Subdivision")
                         .build());

  register_parameter(define_int_parameter("preserve_boundaries", 1)
                         .label("Preserve Boundaries")
                         .range(0, 1)
                         .category("Subdivision")
                         .build());
}

std::shared_ptr<core::GeometryContainer> SubdivisionSOP::execute() {
  // TODO: Implement subdivision with GeometryContainer
  auto input = get_input_data(0);
  if (!input) {
    return nullptr;
  }
  return std::make_shared<core::GeometryContainer>(input->clone());
}

} // namespace nodeflux::sop
