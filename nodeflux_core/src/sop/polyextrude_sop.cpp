#include "nodeflux/sop/polyextrude_sop.hpp"
#include "nodeflux/core/math.hpp"
#include <Eigen/Dense>
#include <iostream>
#include <map>

namespace attrs = nodeflux::core::standard_attrs;

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

// Helper to convert GeometryData to GeometryContainer (bridge for migration)
static std::unique_ptr<core::GeometryContainer>
convert_to_container(const GeometryData &old_data) {
  auto container = std::make_unique<core::GeometryContainer>();

  auto mesh = old_data.get_mesh();
  if (!mesh || mesh->empty()) {
    return container;
  }

  const auto &vertices = mesh->vertices();
  const auto &faces = mesh->faces();

  // Set up topology
  auto &topology = container->topology();
  topology.set_point_count(vertices.rows());

  // Add primitives
  for (int i = 0; i < faces.rows(); ++i) {
    std::vector<int> prim_verts(3);
    for (int j = 0; j < 3; ++j) {
      prim_verts[j] = faces(i, j);
    }
    topology.add_primitive(prim_verts);
  }

  // Add position attribute
  container->add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *p_storage = container->get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage) {
    auto p_span = p_storage->values_writable();
    for (size_t i = 0; i < static_cast<size_t>(vertices.rows()); ++i) {
      p_span[i] = vertices.row(i).cast<float>();
    }
  }

  return container;
}

// Helper to convert GeometryContainer to GeometryData (bridge for migration)
static std::shared_ptr<GeometryData>
convert_from_container(const core::GeometryContainer &container) {
  const auto &topology = container.topology();

  // Extract positions
  auto *p_storage =
      container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (!p_storage)
    return std::make_shared<GeometryData>(std::make_shared<core::Mesh>());

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

  auto mesh = std::make_shared<core::Mesh>(vertices, faces);
  return std::make_shared<GeometryData>(mesh);
}

std::shared_ptr<GeometryData> PolyExtrudeSOP::execute() {
  // Get input geometry
  auto input_geo = get_input_data(0);
  if (!input_geo) {
    set_error("No input geometry connected");
    return nullptr;
  }

  // Convert to GeometryContainer
  auto input_container = convert_to_container(*input_geo);
  if (input_container->topology().point_count() == 0) {
    set_error("Input geometry is empty");
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

  // Convert result to GeometryContainer
  core::GeometryContainer output_container;
  auto &output_topology = output_container.topology();

  output_topology.set_point_count(output_verts.rows());

  // Add primitives
  for (int i = 0; i < output_faces.rows(); ++i) {
    std::vector<int> prim_verts(3);
    for (int j = 0; j < 3; ++j) {
      prim_verts[j] = output_faces(i, j);
    }
    output_topology.add_primitive(prim_verts);
  }

  // Add position attribute
  output_container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *p_storage = output_container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage) {
    auto p_span = p_storage->values_writable();
    for (size_t i = 0; i < static_cast<size_t>(output_verts.rows()); ++i) {
      p_span[i] = output_verts.row(i).cast<float>();
    }
  }

  // Convert back to GeometryData for compatibility
  return convert_from_container(output_container);
}

} // namespace nodeflux::sop
