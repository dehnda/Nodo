#include "nodo/processing/pmp_converter.hpp"

#include "nodo/core/attribute_types.hpp"
#include "nodo/core/math.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/processing/processing_common.hpp"

#include <unordered_map>

#include <pmp/algorithms/normals.h>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::processing::detail {

pmp::SurfaceMesh PMPConverter::to_pmp(const core::Mesh& mesh) {
  pmp::SurfaceMesh result;

  // Validate input
  if (mesh.vertices().rows() == 0) {
    throw std::runtime_error("Cannot convert empty mesh to PMP");
  }

  // Add vertices
  std::vector<pmp::Vertex> vertices;
  vertices.reserve(mesh.vertices().rows());

  for (int i = 0; i < mesh.vertices().rows(); ++i) {
    pmp::Point point(static_cast<float>(mesh.vertices()(i, 0)),
                     static_cast<float>(mesh.vertices()(i, 1)),
                     static_cast<float>(mesh.vertices()(i, 2)));
    vertices.push_back(result.add_vertex(point));
  }

  // Add faces
  for (int i = 0; i < mesh.faces().rows(); ++i) {
    std::vector<pmp::Vertex> face_verts = {vertices[mesh.faces()(i, 0)],
                                           vertices[mesh.faces()(i, 1)],
                                           vertices[mesh.faces()(i, 2)]};
    result.add_face(face_verts);
  }

  // Compute normals
  pmp::vertex_normals(result);

  return result;
}

pmp::SurfaceMesh
PMPConverter::to_pmp(const core::GeometryContainer& container) {
  pmp::SurfaceMesh result;

  // Extract positions
  const auto* positions =
      container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (positions == nullptr) {
    throw std::runtime_error(
        "GeometryContainer missing position attribute 'P'");
  }

  auto pos_span = positions->values();

  // Add vertices
  std::vector<pmp::Vertex> vertices;
  vertices.reserve(pos_span.size());

  for (size_t i = 0; i < pos_span.size(); ++i) {
    const auto& pos = pos_span[i];
    vertices.push_back(result.add_vertex(pmp::Point(pos(0), pos(1), pos(2))));
  }

  // Add faces from primitives
  // Note: primitives reference vertex indices, which we need to map to point
  // indices
  const auto& topology = container.topology();
  size_t skipped_degenerate = 0;

  for (size_t i = 0; i < topology.primitive_count(); ++i) {
    const auto& prim_verts = topology.get_primitive_vertices(i);

    // PMP SurfaceMesh can handle any polygon (not just triangles)
    // Map vertex indices to point indices first
    std::vector<int> point_indices;
    point_indices.reserve(prim_verts.size());
    for (int vert_idx : prim_verts) {
      int point_idx = topology.get_vertex_point(vert_idx);
      point_indices.push_back(point_idx);
    }

    // Check for degenerate faces (faces with duplicate point indices)
    bool is_degenerate = false;
    for (size_t j = 0; j < point_indices.size(); ++j) {
      for (size_t k = j + 1; k < point_indices.size(); ++k) {
        if (point_indices[j] == point_indices[k]) {
          is_degenerate = true;
          break;
        }
      }
      if (is_degenerate)
        break;
    }

    if (is_degenerate) {
      skipped_degenerate++;
      continue;
    }

    // Now convert to PMP vertices
    std::vector<pmp::Vertex> face_verts;
    face_verts.reserve(point_indices.size());
    for (int point_idx : point_indices) {
      face_verts.push_back(vertices[point_idx]);
    }

    result.add_face(face_verts);
  }

  if (skipped_degenerate > 0) {
    std::cout << "PMPConverter: Skipped " << skipped_degenerate
              << " degenerate faces out of " << topology.primitive_count()
              << "\n";
  }

  // Compute normals if not present
  compute_normals(result);

  return result;
}

core::Mesh PMPConverter::from_pmp(const pmp::SurfaceMesh& pmp_mesh) {
  core::Mesh result;

  const size_t num_vertices = pmp_mesh.n_vertices();
  const size_t num_faces = pmp_mesh.n_faces();

  // Extract vertices
  Eigen::Matrix<double, Eigen::Dynamic, 3, Eigen::RowMajor> vertices_matrix(
      num_vertices, 3);
  auto points = pmp_mesh.get_vertex_property<pmp::Point>("v:point");

  size_t vert_idx = 0;
  for (auto vert : pmp_mesh.vertices()) {
    const auto& point = points[vert];
    vertices_matrix(vert_idx, 0) = static_cast<double>(point[0]);
    vertices_matrix(vert_idx, 1) = static_cast<double>(point[1]);
    vertices_matrix(vert_idx, 2) = static_cast<double>(point[2]);
    ++vert_idx;
  }

  // Extract faces
  Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor> faces_matrix(num_faces,
                                                                      3);

  size_t face_idx = 0;
  for (auto face : pmp_mesh.faces()) {
    auto face_verts = pmp_mesh.vertices(face);
    auto iter = face_verts.begin();
    faces_matrix(face_idx, 0) = (*iter++).idx();
    faces_matrix(face_idx, 1) = (*iter++).idx();
    faces_matrix(face_idx, 2) = (*iter).idx();
    ++face_idx;
  }

  result.vertices() = vertices_matrix;
  result.faces() = faces_matrix;

  return result;
}

core::GeometryContainer
PMPConverter::from_pmp_container(const pmp::SurfaceMesh& pmp_mesh,
                                 bool preserve_attributes) {
  core::GeometryContainer result;

  // Create mapping from PMP vertex indices to sequential indices
  std::unordered_map<int, int> pmp_to_seq;

  // Extract and store positions
  auto points_prop = pmp_mesh.get_vertex_property<pmp::Point>("v:point");
  std::vector<core::Vec3f> positions;
  positions.reserve(pmp_mesh.n_vertices());

  int seq_idx = 0;
  for (auto vert : pmp_mesh.vertices()) {
    pmp_to_seq[vert.idx()] = seq_idx++;
    const auto& point = points_prop[vert];
    positions.push_back({point[0], point[1], point[2]});
  }

  const size_t num_vertices = positions.size(); // Actual active vertex count

  // Set up topology with actual vertex count
  // For simple meshes without corner attributes, points == vertices (1:1
  // mapping)
  result.topology().set_point_count(num_vertices);
  result.topology().set_vertex_count(num_vertices);

  // Initialize 1:1 vertex→point mapping
  auto vertex_points = result.topology().get_vertex_points_writable();
  for (size_t i = 0; i < num_vertices; ++i) {
    vertex_points[i] = static_cast<int>(i);
  }

  result.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto* pos_attr = result.get_point_attribute_typed<core::Vec3f>(attrs::P);
  pos_attr->resize(num_vertices);
  auto pos_span = pos_attr->values_writable();
  for (size_t i = 0; i < num_vertices; ++i) {
    pos_span[i] = positions[i];
  }

  // Extract normals if available
  if (preserve_attributes) {
    auto normals_prop = pmp_mesh.get_vertex_property<pmp::Normal>("v:normal");
    if (normals_prop) {
      std::vector<core::Vec3f> normals;
      normals.reserve(num_vertices);

      for (auto vert : pmp_mesh.vertices()) {
        const auto& normal = normals_prop[vert];
        normals.push_back({normal[0], normal[1], normal[2]});
      }

      result.add_point_attribute(attrs::N, core::AttributeType::VEC3F);
      auto* norm_attr = result.get_point_attribute_typed<core::Vec3f>(attrs::N);
      norm_attr->resize(num_vertices);
      auto norm_span = norm_attr->values_writable();
      for (size_t i = 0; i < num_vertices; ++i) {
        norm_span[i] = normals[i];
      }
    }
  }

  // Add faces as primitives using the index mapping
  for (auto face : pmp_mesh.faces()) {
    std::vector<int> face_indices;

    // Iterate through all vertices of the face (handles triangles, quads,
    // n-gons)
    for (auto vert : pmp_mesh.vertices(face)) {
      int mapped_idx = pmp_to_seq.at(vert.idx());

      // Validate index is in range
      if (mapped_idx < 0 || mapped_idx >= static_cast<int>(num_vertices)) {
        throw std::runtime_error(
            "Internal error: Invalid vertex index mapping");
      }

      face_indices.push_back(mapped_idx);
    }

    result.topology().add_primitive(face_indices);
  }

  return result;
}
std::string PMPConverter::validate_for_pmp(const core::Mesh& mesh) {
  if (mesh.vertices().rows() < 3) {
    return "Mesh must have at least 3 vertices";
  }

  if (mesh.faces().rows() < 1) {
    return "Mesh must have at least 1 face";
  }

  // Check face indices are valid
  const int max_idx = static_cast<int>(mesh.vertices().rows());
  for (int i = 0; i < mesh.faces().rows(); ++i) {
    for (int j = 0; j < 3; ++j) {
      int idx = mesh.faces()(i, j);
      if (idx < 0 || idx >= max_idx) {
        return "Face references invalid vertex index";
      }
    }
  }

  return ""; // Valid
}

std::string
PMPConverter::validate_for_pmp(const core::GeometryContainer& container) {
  if (container.topology().point_count() < 3) {
    return "Container must have at least 3 points";
  }

  if (container.topology().primitive_count() < 1) {
    return "Container must have at least 1 primitive";
  }

  // Check for position attribute
  if (!container.has_point_attribute(attrs::P)) {
    return "Container missing position attribute 'P'";
  }

  // Note: We no longer require triangles here since pmp::triangulate()
  // can handle any polygon mesh

  return ""; // Valid
}

void PMPConverter::compute_normals(pmp::SurfaceMesh& mesh) {
  if (!mesh.has_vertex_property("v:normal")) {
    pmp::vertex_normals(mesh);
  }
}

} // namespace nodo::processing::detail
