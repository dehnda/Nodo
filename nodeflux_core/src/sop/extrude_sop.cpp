#include "nodeflux/sop/extrude_sop.hpp"
#include "nodeflux/core/math.hpp"
#include <iostream>

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::sop {

ExtrudeSOP::ExtrudeSOP(const std::string &name)
    : SOPNode(name, "Extrude") {
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
      define_int_parameter("mode", 0)
          .label("Mode")
          .options({"Face Normals", "Uniform Direction", "Region Normals"})
          .category("Extrusion")
          .build());

  register_parameter(
      define_float_parameter("inset", 0.0F)
          .label("Inset")
          .range(0.0, 5.0)
          .category("Extrusion")
          .build());

  // Direction vector (for uniform mode)
  register_parameter(
      define_float_parameter("direction_x", 0.0F)
          .label("Direction X")
          .range(-1.0, 1.0)
          .category("Direction")
          .build());

  register_parameter(
      define_float_parameter("direction_y", 0.0F)
          .label("Direction Y")
          .range(-1.0, 1.0)
          .category("Direction")
          .build());

  register_parameter(
      define_float_parameter("direction_z", 1.0F)
          .label("Direction Z")
          .range(-1.0, 1.0)
          .category("Direction")
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

  // Add normals if available
  try {
    const auto &normals = mesh->vertex_normals();
    container->add_point_attribute(attrs::N, core::AttributeType::VEC3F);
    auto *n_storage = container->get_point_attribute_typed<core::Vec3f>(attrs::N);
    if (n_storage) {
      auto n_span = n_storage->values_writable();
      for (size_t i = 0; i < static_cast<size_t>(normals.rows()); ++i) {
        n_span[i] = normals.row(i).cast<float>();
      }
    }
  } catch (...) {
    // No normals available
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

std::shared_ptr<GeometryData> ExtrudeSOP::execute() {
  // Sync member variables from parameter system
  distance_ = get_parameter<float>("distance", 1.0F);
  inset_ = get_parameter<float>("inset", 0.0F);
  mode_ = static_cast<ExtrusionMode>(get_parameter<int>("mode", 0));
  direction_ = Eigen::Vector3d(
      get_parameter<float>("direction_x", 0.0F),
      get_parameter<float>("direction_y", 0.0F),
      get_parameter<float>("direction_z", 1.0F)
  );

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

  if (input_container->topology().primitive_count() == 0) {
    set_error("Input mesh has no faces to extrude");
    return nullptr;
  }

  if (mode_ == ExtrusionMode::UNIFORM_DIRECTION && direction_.norm() < 1e-6) {
    set_error("Invalid direction vector");
    return nullptr;
  }

  // For now, convert to Mesh for extrusion (TODO: refactor to use GeometryContainer directly)
  auto input_mesh = input_geo->get_mesh();
  core::Mesh result;

  switch (mode_) {
  case ExtrusionMode::FACE_NORMALS:
    result = extrude_face_normals(*input_mesh);
    break;
  case ExtrusionMode::UNIFORM_DIRECTION:
    result = extrude_uniform_direction(*input_mesh);
    break;
  case ExtrusionMode::REGION_NORMALS:
    result = extrude_region_normals(*input_mesh);
    break;
  }

  // Apply inset if requested
  if (inset_ > 0.0) {
    apply_inset(result, inset_);
  }

  // Convert result Mesh to GeometryContainer
  core::GeometryContainer output_container;
  auto &output_topology = output_container.topology();

  const auto &vertices = result.vertices();
  const auto &faces = result.faces();

  output_topology.set_point_count(vertices.rows());

  // Add primitives
  for (int i = 0; i < faces.rows(); ++i) {
    std::vector<int> prim_verts(3);
    for (int j = 0; j < 3; ++j) {
      prim_verts[j] = faces(i, j);
    }
    output_topology.add_primitive(prim_verts);
  }

  // Add position attribute
  output_container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *p_storage = output_container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage) {
    auto p_span = p_storage->values_writable();
    for (size_t i = 0; i < static_cast<size_t>(vertices.rows()); ++i) {
      p_span[i] = vertices.row(i).cast<float>();
    }
  }

  // Convert back to GeometryData for compatibility
  return convert_from_container(output_container);
}

core::Mesh ExtrudeSOP::extrude_face_normals(const core::Mesh &input) {
  const auto &vertices = input.vertices();
  const auto &faces = input.faces();

  // Calculate vertex normals (average of adjacent face normals)
  Eigen::MatrixXd vertex_normals = Eigen::MatrixXd::Zero(vertices.rows(), 3);
  std::vector<int> vertex_face_count(vertices.rows(), 0);

  auto face_normals = calculate_face_normals(input);

  // Accumulate normals for each vertex
  for (int f = 0; f < faces.rows(); ++f) {
    const Eigen::Vector3d normal = face_normals.row(f);
    for (int v = 0; v < 3; ++v) {
      int vertex_idx = faces(f, v);
      vertex_normals.row(vertex_idx) += normal.transpose();
      vertex_face_count[vertex_idx]++;
    }
  }

  // Normalize vertex normals
  for (int i = 0; i < vertices.rows(); ++i) {
    if (vertex_face_count[i] > 0) {
      vertex_normals.row(i) /= static_cast<double>(vertex_face_count[i]);
      vertex_normals.row(i).normalize();
    }
  }

  // Create new vertices: original + extruded
  Eigen::MatrixXd new_vertices(vertices.rows() * 2, 3);
  new_vertices.topRows(vertices.rows()) = vertices;

  // Create extruded vertices using vertex normals
  for (int i = 0; i < vertices.rows(); ++i) {
    const Eigen::Vector3d displacement = vertex_normals.row(i) * distance_;
    new_vertices.row(vertices.rows() + i) =
        vertices.row(i) + displacement.transpose();
  }

  // Create faces: original + extruded + sides
  std::vector<Eigen::RowVector3i> all_faces;

  // Original faces (flip if extruding outward)
  for (int i = 0; i < faces.rows(); ++i) {
    if (distance_ > 0.0) {
      all_faces.emplace_back(faces(i, 2), faces(i, 1), faces(i, 0));
    } else {
      all_faces.emplace_back(faces(i, 0), faces(i, 1), faces(i, 2));
    }
  }

  // Extruded faces
  for (int i = 0; i < faces.rows(); ++i) {
    int v0 = faces(i, 0) + vertices.rows();
    int v1 = faces(i, 1) + vertices.rows();
    int v2 = faces(i, 2) + vertices.rows();

    if (distance_ > 0.0) {
      all_faces.emplace_back(v0, v1, v2);
    } else {
      all_faces.emplace_back(v2, v1, v0);
    }
  }

  // Side faces connecting original to extruded
  for (int i = 0; i < faces.rows(); ++i) {
    for (int edge = 0; edge < 3; ++edge) {
      int v1 = faces(i, edge);
      int v2 = faces(i, (edge + 1) % 3);
      int v1_ext = v1 + vertices.rows();
      int v2_ext = v2 + vertices.rows();

      // Two triangles per edge
      if (distance_ > 0.0) {
        all_faces.emplace_back(v1, v2, v1_ext);
        all_faces.emplace_back(v2, v2_ext, v1_ext);
      } else {
        all_faces.emplace_back(v1, v1_ext, v2);
        all_faces.emplace_back(v2, v1_ext, v2_ext);
      }
    }
  }

  // Convert to Eigen matrix
  Eigen::MatrixXi new_faces(all_faces.size(), 3);
  for (size_t i = 0; i < all_faces.size(); ++i) {
    new_faces.row(i) = all_faces[i];
  }

  return core::Mesh(new_vertices, new_faces);
}

core::Mesh ExtrudeSOP::extrude_uniform_direction(const core::Mesh &input) {
  const auto &vertices = input.vertices();
  const auto &faces = input.faces();

  const Eigen::Vector3d displacement = direction_ * distance_;

  // Create extruded vertices
  Eigen::MatrixXd new_vertices(vertices.rows() * 2, 3);
  new_vertices.topRows(vertices.rows()) = vertices;

  // Displace all vertices uniformly
  for (int i = 0; i < vertices.rows(); ++i) {
    new_vertices.row(vertices.rows() + i) =
        vertices.row(i) + displacement.transpose();
  }

  // Create faces using vector approach like face normals method
  std::vector<Eigen::RowVector3i> all_faces;

  // Original faces
  for (int i = 0; i < faces.rows(); ++i) {
    if (distance_ > 0.0) {
      all_faces.emplace_back(faces(i, 2), faces(i, 1), faces(i, 0));
    } else {
      all_faces.emplace_back(faces(i, 0), faces(i, 1), faces(i, 2));
    }
  }

  // Extruded faces
  for (int i = 0; i < faces.rows(); ++i) {
    int v0 = faces(i, 0) + vertices.rows();
    int v1 = faces(i, 1) + vertices.rows();
    int v2 = faces(i, 2) + vertices.rows();

    if (distance_ > 0.0) {
      all_faces.emplace_back(v0, v1, v2);
    } else {
      all_faces.emplace_back(v2, v1, v0);
    }
  }

  // Side faces
  for (int i = 0; i < faces.rows(); ++i) {
    for (int edge = 0; edge < 3; ++edge) {
      int v1 = faces(i, edge);
      int v2 = faces(i, (edge + 1) % 3);
      int v1_ext = v1 + vertices.rows();
      int v2_ext = v2 + vertices.rows();

      if (distance_ > 0.0) {
        all_faces.emplace_back(v1, v2, v1_ext);
        all_faces.emplace_back(v2, v2_ext, v1_ext);
      } else {
        all_faces.emplace_back(v1, v1_ext, v2);
        all_faces.emplace_back(v2, v1_ext, v2_ext);
      }
    }
  }

  // Convert to Eigen matrix
  Eigen::MatrixXi new_faces(all_faces.size(), 3);
  for (size_t i = 0; i < all_faces.size(); ++i) {
    new_faces.row(i) = all_faces[i];
  }

  return core::Mesh(new_vertices, new_faces);
}

core::Mesh ExtrudeSOP::extrude_region_normals(const core::Mesh &input) {
  // For now, use vertex normals as approximation of region normals
  // This could be enhanced with proper region detection
  return extrude_face_normals(input);
}

Eigen::MatrixXd ExtrudeSOP::calculate_face_normals(const core::Mesh &mesh) {
  const auto &vertices = mesh.vertices();
  const auto &faces = mesh.faces();

  Eigen::MatrixXd normals(faces.rows(), 3);

  for (int i = 0; i < faces.rows(); ++i) {
    const Eigen::Vector3d v1 = vertices.row(faces(i, 0));
    const Eigen::Vector3d v2 = vertices.row(faces(i, 1));
    const Eigen::Vector3d v3 = vertices.row(faces(i, 2));

    const Eigen::Vector3d edge1 = v2 - v1;
    const Eigen::Vector3d edge2 = v3 - v1;
    const Eigen::Vector3d normal = edge1.cross(edge2).normalized();

    normals.row(i) = normal.transpose();
  }

  return normals;
}

void ExtrudeSOP::apply_inset(core::Mesh &mesh, double inset_amount) {
  // Placeholder for inset functionality
  // This would shrink faces inward by the specified amount
  // Complex implementation - could be added in future iteration
  std::cout << "  Note: Inset functionality not yet implemented\n";
}

} // namespace nodeflux::sop
