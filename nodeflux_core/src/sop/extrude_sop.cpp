#include "nodeflux/sop/extrude_sop.hpp"
#include <iostream>

namespace nodeflux::sop {

ExtrudeSOP::ExtrudeSOP(const std::string &name)
    : SOPNode(name, "Extrude") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);
}

std::shared_ptr<GeometryData> ExtrudeSOP::execute() {
  // Get input geometry
  auto input_geo = get_input_data(0);
  if (!input_geo) {
    set_error("No input geometry connected");
    return nullptr;
  }

  auto input_mesh = input_geo->get_mesh();
  if (!input_mesh) {
    set_error("Input geometry does not contain a mesh");
    return nullptr;
  }

  if (input_mesh->faces().rows() == 0) {
    set_error("Input mesh has no faces to extrude");
    return nullptr;
  }

  if (mode_ == ExtrusionMode::UNIFORM_DIRECTION && direction_.norm() < 1e-6) {
    set_error("Invalid direction vector");
    return nullptr;
  }

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

  auto result_mesh = std::make_shared<core::Mesh>(std::move(result));
  return std::make_shared<GeometryData>(result_mesh);
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
