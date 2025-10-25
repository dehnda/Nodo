#include "nodeflux/sop/resample_sop.hpp"
#include "nodeflux/core/geometry_container.hpp"
#include "nodeflux/core/standard_attributes.hpp"
#include <Eigen/Dense>
#include <cmath>
#include <iostream>
#include <vector>

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::sop {

// Helper to convert GeometryContainer to Mesh for resampling
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

  // For line/curve resampling, faces may not be meaningful
  // Create a simple face matrix if primitives exist
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

  // Build topology if faces exist
  if (faces.rows() > 0) {
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

ResampleSOP::ResampleSOP(const std::string &name) : SOPNode(name, "Resample") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_int_parameter("mode", 0)
                         .label("Mode")
                         .options({"By Count", "By Length"})
                         .category("Resample")
                         .build());

  register_parameter(define_int_parameter("point_count", 10)
                         .label("Point Count")
                         .range(2, 1000)
                         .category("Resample")
                         .build());

  register_parameter(define_float_parameter("segment_length", 0.1F)
                         .label("Segment Length")
                         .range(0.001, 10.0)
                         .category("Resample")
                         .build());
}

// Helper function for calculating curve length
static float calculate_curve_length(const core::Mesh &mesh) {
  const auto &vertices = mesh.vertices();
  if (vertices.rows() < 2) {
    return 0.0F;
  }

  float total_length = 0.0F;
  for (int i = 0; i < vertices.rows() - 1; ++i) {
    Eigen::Vector3d diff = vertices.row(i + 1) - vertices.row(i);
    total_length += static_cast<float>(diff.norm());
  }

  return total_length;
}

std::shared_ptr<core::GeometryContainer> ResampleSOP::execute() {
  // TODO: Implement resample with GeometryContainer
  auto input = get_input_data(0);
  if (!input) {
    return nullptr;
  }
  return std::make_shared<core::GeometryContainer>(input->clone());
}

} // namespace nodeflux::sop
