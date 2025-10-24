#include "nodeflux/sop/polyextrude_sop.hpp"
#include <Eigen/Dense>
#include <iostream>
#include <map>

namespace nodeflux::sop {

PolyExtrudeSOP::PolyExtrudeSOP(const std::string &name)
    : SOPNode(name, "PolyExtrude") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(
      define_float_parameter("distance", 1.0F)
          .label("Distance")
          .range(0.0, 10.0)
          .category("Extrusion")
          .build());

  register_parameter(
      define_float_parameter("inset", 0.0F)
          .label("Inset")
          .range(0.0, 1.0)
          .category("Extrusion")
          .build());

  register_parameter(
      define_int_parameter("individual_faces", 1)
          .label("Individual Faces")
          .range(0, 1)
          .category("Extrusion")
          .build());
}

std::shared_ptr<GeometryData> PolyExtrudeSOP::execute() {
  // Get input geometry
  auto input_geo = get_input_data(0);
  if (!input_geo) {
    set_error("No input geometry connected");
    return nullptr;
  }

  auto input_mesh = input_geo->get_mesh();
  if (!input_mesh || input_mesh->empty()) {
    set_error("Input geometry does not contain a valid mesh");
    return nullptr;
  }

  const auto &input_verts = input_mesh->vertices();
  const auto &input_faces = input_mesh->faces();
  const auto &face_normals = input_mesh->face_normals();

  // Calculate output size
  // For each face: 1 top face + 3 side quads (2 triangles each)
  const int num_input_faces = static_cast<int>(input_faces.rows());
  const int num_output_faces = num_input_faces * 7; // 1 top + 6 side triangles
  const int num_output_verts =
      num_input_faces * 6; // 3 original + 3 extruded per face

  core::Mesh::Vertices output_verts(num_output_verts, 3);
  core::Mesh::Faces output_faces(num_output_faces, 3);

  int vert_idx = 0;
  int face_idx = 0;

  for (int f = 0; f < num_input_faces; ++f) {
    const int v0 = input_faces(f, 0);
    const int v1 = input_faces(f, 1);
    const int v2 = input_faces(f, 2);

    // Get face vertices
    Eigen::Vector3d p0 = input_verts.row(v0);
    Eigen::Vector3d p1 = input_verts.row(v1);
    Eigen::Vector3d p2 = input_verts.row(v2);

    // Get face normal
    Eigen::Vector3d normal = face_normals.row(f);

    // Calculate face center for inset
    Eigen::Vector3d center = (p0 + p1 + p2) / 3.0;

    // Apply inset if specified
    if (inset_ > 0.0F) {
      p0 = center + (p0 - center) * (1.0 - static_cast<double>(inset_));
      p1 = center + (p1 - center) * (1.0 - static_cast<double>(inset_));
      p2 = center + (p2 - center) * (1.0 - static_cast<double>(inset_));
    }

    // Create extruded vertices (move along normal)
    Eigen::Vector3d e0 = p0 + normal * static_cast<double>(distance_);
    Eigen::Vector3d e1 = p1 + normal * static_cast<double>(distance_);
    Eigen::Vector3d e2 = p2 + normal * static_cast<double>(distance_);

    // Store vertices (base triangle + extruded triangle)
    const int base_start = vert_idx;
    output_verts.row(vert_idx++) = p0;
    output_verts.row(vert_idx++) = p1;
    output_verts.row(vert_idx++) = p2;
    output_verts.row(vert_idx++) = e0;
    output_verts.row(vert_idx++) = e1;
    output_verts.row(vert_idx++) = e2;

    // Top face (extruded)
    output_faces.row(face_idx++) =
        Eigen::Vector3i(base_start + 3, base_start + 4, base_start + 5);

    // Side faces (3 quads, each as 2 triangles)
    // Quad 0: p0-p1-e1-e0
    output_faces.row(face_idx++) =
        Eigen::Vector3i(base_start + 0, base_start + 1, base_start + 4);
    output_faces.row(face_idx++) =
        Eigen::Vector3i(base_start + 0, base_start + 4, base_start + 3);

    // Quad 1: p1-p2-e2-e1
    output_faces.row(face_idx++) =
        Eigen::Vector3i(base_start + 1, base_start + 2, base_start + 5);
    output_faces.row(face_idx++) =
        Eigen::Vector3i(base_start + 1, base_start + 5, base_start + 4);

    // Quad 2: p2-p0-e0-e2
    output_faces.row(face_idx++) =
        Eigen::Vector3i(base_start + 2, base_start + 0, base_start + 3);
    output_faces.row(face_idx++) =
        Eigen::Vector3i(base_start + 2, base_start + 3, base_start + 5);
  }

  auto result_mesh =
      std::make_shared<core::Mesh>(std::move(output_verts), std::move(output_faces));
  return std::make_shared<GeometryData>(result_mesh);
}

} // namespace nodeflux::sop
