#include "nodeflux/sop/boolean_sop.hpp"
#include "nodeflux/core/geometry_container.hpp"
#include "nodeflux/core/standard_attributes.hpp"
#include "nodeflux/geometry/boolean_ops.hpp"
#include <iostream>

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::sop {

// Helper to convert GeometryData to Mesh for BooleanOps (temporary bridge)
static std::shared_ptr<core::Mesh>
container_to_mesh(const core::GeometryContainer &container) {
  const auto &topology = container.topology();

  auto *p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (!p_storage)
    return nullptr;

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

  return std::make_shared<core::Mesh>(vertices, faces);
}

// Helper to convert Mesh back to GeometryContainer
static std::shared_ptr<core::GeometryContainer>
mesh_to_container(const core::Mesh &mesh) {
  auto container = std::make_shared<core::GeometryContainer>();
  const auto &vertices = mesh.vertices();
  const auto &faces = mesh.faces();

  container->set_point_count(vertices.rows());

  // Build topology
  size_t vert_idx = 0;
  for (int face_idx = 0; face_idx < faces.rows(); ++face_idx) {
    std::vector<int> prim_verts;
    for (int j = 0; j < faces.cols(); ++j) {
      int point_idx = faces(face_idx, j);
      container->topology().set_vertex_point(vert_idx, point_idx);
      prim_verts.push_back(static_cast<int>(vert_idx));
      ++vert_idx;
    }
    container->add_primitive(prim_verts);
  }

  // Copy positions
  container->add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *positions = container->get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (positions) {
    for (int i = 0; i < vertices.rows(); ++i) {
      (*positions)[i] = vertices.row(i).cast<float>();
    }
  }

  return container;
}

BooleanSOP::BooleanSOP(const std::string &node_name)
    : SOPNode(node_name, "Boolean") {
  // Add input ports
  input_ports_.add_port("mesh_a", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);
  input_ports_.add_port("mesh_b", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_int_parameter("operation", 0)
                         .label("Operation")
                         .options({"Union", "Intersection", "Difference"})
                         .category("Boolean")
                         .build());
}

std::shared_ptr<core::GeometryContainer> BooleanSOP::execute() {
  // TODO: Implement boolean ops with GeometryContainer
  auto inputA = get_input_data("A");
  if (!inputA) {
    return nullptr;
  }
  return std::make_shared<core::GeometryContainer>(inputA->clone());
}
// Read parameters from parameter system
} // namespace nodeflux::sop
