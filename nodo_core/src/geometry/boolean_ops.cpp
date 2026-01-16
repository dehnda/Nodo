#include "nodo/geometry/boolean_ops.hpp"

#include "nodo/core/standard_attributes.hpp"

#include <array>
#include <fstream>
#include <iostream>
#include <vector>

#include <manifold/manifold.h>

using namespace manifold;

namespace attrs = nodo::core::standard_attrs;

namespace nodo::geometry {

// Thread-local storage for error reporting
thread_local core::Error BooleanOps::last_error_{core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"};

// ============================================================================
// NEW API: GeometryContainer-based operations
// ============================================================================

std::optional<core::GeometryContainer> BooleanOps::union_geometries(const core::GeometryContainer& a,
                                                                    const core::GeometryContainer& b) {
  // Extract Eigen matrices directly from GeometryContainers
  auto [vertices_a, faces_a] = extract_eigen_mesh(a);
  auto [vertices_b, faces_b] = extract_eigen_mesh(b);

  if (faces_a.rows() == 0 || faces_b.rows() == 0) {
    set_last_error(core::Error{core::ErrorCategory::Validation, core::ErrorCode::EmptyMesh,
                               "One or both input geometries are empty"});
    return std::nullopt;
  }

  // Perform boolean operation directly with Eigen matrices
  auto [result_verts, result_faces] = manifold_boolean_operation_direct(vertices_a, faces_a, vertices_b, faces_b, 0);
  if (result_faces.rows() == 0) {
    return std::nullopt;
  }

  // Build GeometryContainer directly from result
  return build_container_from_eigen(result_verts, result_faces);
}

std::optional<core::GeometryContainer> BooleanOps::intersect_geometries(const core::GeometryContainer& a,
                                                                        const core::GeometryContainer& b) {
  // Extract Eigen matrices directly from GeometryContainers
  auto [vertices_a, faces_a] = extract_eigen_mesh(a);
  auto [vertices_b, faces_b] = extract_eigen_mesh(b);

  if (faces_a.rows() == 0 || faces_b.rows() == 0) {
    set_last_error(core::Error{core::ErrorCategory::Validation, core::ErrorCode::EmptyMesh,
                               "One or both input geometries are empty"});
    return std::nullopt;
  }

  // Perform boolean operation directly with Eigen matrices
  auto [result_verts, result_faces] = manifold_boolean_operation_direct(vertices_a, faces_a, vertices_b, faces_b, 1);
  if (result_faces.rows() == 0) {
    return std::nullopt;
  }

  // Build GeometryContainer directly from result
  return build_container_from_eigen(result_verts, result_faces);
}

std::optional<core::GeometryContainer> BooleanOps::difference_geometries(const core::GeometryContainer& a,
                                                                         const core::GeometryContainer& b) {
  // Extract Eigen matrices directly from GeometryContainers
  auto [vertices_a, faces_a] = extract_eigen_mesh(a);
  auto [vertices_b, faces_b] = extract_eigen_mesh(b);

  if (faces_a.rows() == 0 || faces_b.rows() == 0) {
    set_last_error(core::Error{core::ErrorCategory::Validation, core::ErrorCode::EmptyMesh,
                               "One or both input geometries are empty"});
    return std::nullopt;
  }

  // Perform boolean operation directly with Eigen matrices
  auto [result_verts, result_faces] = manifold_boolean_operation_direct(vertices_a, faces_a, vertices_b, faces_b, 2);
  if (result_faces.rows() == 0) {
    return std::nullopt;
  }

  // Build GeometryContainer directly from result
  return build_container_from_eigen(result_verts, result_faces);
}

const core::Error& BooleanOps::last_error() {
  return last_error_;
}

void BooleanOps::set_last_error(const core::Error& error) {
  last_error_ = error;
}

// ============================================================================
// Direct Conversion Helpers: GeometryContainer ↔ Eigen matrices
// ============================================================================

// Extract Eigen matrices directly from GeometryContainer (no Mesh intermediate)
std::pair<Eigen::MatrixXd, Eigen::MatrixXi> BooleanOps::extract_eigen_mesh(const core::GeometryContainer& container) {
  const auto& topology = container.topology();

  // Get position attribute
  auto* p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (!p_storage) {
    // Return empty matrices if no positions
    return {Eigen::MatrixXd(0, 3), Eigen::MatrixXi(0, 3)};
  }

  // Copy vertices with double precision for Manifold
  Eigen::MatrixXd vertices(topology.point_count(), 3);
  auto p_span = p_storage->values();
  for (size_t i = 0; i < p_span.size(); ++i) {
    vertices.row(i) = p_span[i].cast<double>();
  }

  // Triangulate: Convert quads/polygons to triangles
  std::vector<Eigen::Vector3i> triangle_list;
  triangle_list.reserve(topology.primitive_count() * 2); // Estimate for quads

  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto& vert_indices = topology.get_primitive_vertices(prim_idx);

    // Get point indices for this primitive
    std::vector<int> point_indices;
    point_indices.reserve(vert_indices.size());
    for (size_t vi : vert_indices) {
      point_indices.push_back(topology.get_vertex_point(vi));
    }

    // Triangulate based on polygon size
    if (point_indices.size() == 3) {
      // Already a triangle
      triangle_list.emplace_back(point_indices[0], point_indices[1], point_indices[2]);
    } else if (point_indices.size() == 4) {
      // Quad: split into two triangles
      triangle_list.emplace_back(point_indices[0], point_indices[1], point_indices[2]);
      triangle_list.emplace_back(point_indices[0], point_indices[2], point_indices[3]);
    } else if (point_indices.size() > 4) {
      // N-gon: use fan triangulation from first vertex
      for (size_t i = 1; i + 1 < point_indices.size(); ++i) {
        triangle_list.emplace_back(point_indices[0], point_indices[i], point_indices[i + 1]);
      }
    }
  }

  // Convert triangle list to Eigen matrix
  Eigen::MatrixXi faces(triangle_list.size(), 3);
  for (size_t i = 0; i < triangle_list.size(); ++i) {
    faces.row(i) = triangle_list[i];
  }

  return {vertices, faces};
}

// Build GeometryContainer directly from Eigen matrices (no Mesh intermediate)
core::GeometryContainer BooleanOps::build_container_from_eigen(const Eigen::MatrixXd& vertices,
                                                               const Eigen::MatrixXi& faces) {
  core::GeometryContainer container;

  container.set_point_count(vertices.rows());

  // Build topology
  size_t total_verts = faces.rows() * faces.cols();
  container.topology().set_vertex_count(total_verts);

  size_t vert_idx = 0;
  for (int face_idx = 0; face_idx < faces.rows(); ++face_idx) {
    std::vector<int> prim_verts;
    prim_verts.reserve(faces.cols());
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
  auto* positions = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (positions != nullptr) {
    for (int i = 0; i < vertices.rows(); ++i) {
      (*positions)[i] = vertices.row(i).cast<float>();
    }
  }

  // Compute face normals
  std::vector<core::Vec3f> face_normals(faces.rows());
  for (int face_idx = 0; face_idx < faces.rows(); ++face_idx) {
    int idx0 = faces(face_idx, 0);
    int idx1 = faces(face_idx, 1);
    int idx2 = faces(face_idx, 2);

    // Validate face indices
    if (idx0 < 0 || idx0 >= vertices.rows() || idx1 < 0 || idx1 >= vertices.rows() || idx2 < 0 ||
        idx2 >= vertices.rows()) {
      face_normals[face_idx] = core::Vec3f(0, 1, 0); // Default normal
      continue;
    }

    core::Vec3f v0 = vertices.row(idx0).cast<float>();
    core::Vec3f v1 = vertices.row(idx1).cast<float>();
    core::Vec3f v2 = vertices.row(idx2).cast<float>();

    // Compute face normal using cross product
    core::Vec3f edge1 = v1 - v0;
    core::Vec3f edge2 = v2 - v0;
    face_normals[face_idx] = edge1.cross(edge2);

    float length = face_normals[face_idx].norm();
    if (length > 1e-6F) {
      face_normals[face_idx] /= length;
    }
  }

  // Compute vertex normals by averaging adjacent face normals
  container.add_point_attribute(attrs::N, core::AttributeType::VEC3F);
  auto* normals = container.get_point_attribute_typed<core::Vec3f>(attrs::N);
  if (normals != nullptr) {
    // Initialize all normals to zero
    for (size_t i = 0; i < container.topology().point_count(); ++i) {
      (*normals)[i] = core::Vec3f{0.0F, 0.0F, 0.0F};
    }

    // Accumulate face normals at each vertex
    for (int face_idx = 0; face_idx < faces.rows(); ++face_idx) {
      int idx0 = faces(face_idx, 0);
      int idx1 = faces(face_idx, 1);
      int idx2 = faces(face_idx, 2);

      if (idx0 >= 0 && idx0 < static_cast<int>(normals->size()) && idx1 >= 0 &&
          idx1 < static_cast<int>(normals->size()) && idx2 >= 0 && idx2 < static_cast<int>(normals->size())) {
        (*normals)[idx0] += face_normals[face_idx];
        (*normals)[idx1] += face_normals[face_idx];
        (*normals)[idx2] += face_normals[face_idx];
      }
    }

    // Normalize all vertex normals
    for (size_t i = 0; i < normals->size(); ++i) {
      float length = (*normals)[i].norm();
      if (length > 1e-6F) {
        (*normals)[i] /= length;
      } else {
        (*normals)[i] = core::Vec3f(0, 1, 0); // Default up vector
      }
    }
  }

  return container;
}

// ============================================================================
// Direct Manifold Operations (no Mesh intermediate)
// ============================================================================

// New direct boolean operation working with Eigen matrices
std::pair<Eigen::MatrixXd, Eigen::MatrixXi>
BooleanOps::manifold_boolean_operation_direct(const Eigen::MatrixXd& vertices_a, const Eigen::MatrixXi& faces_a,
                                              const Eigen::MatrixXd& vertices_b, const Eigen::MatrixXi& faces_b,
                                              int operation_type) {
  try {
    // Convert to Manifold format directly from Eigen matrices
    manifold::Manifold mesh_a = eigen_to_manifold_direct(vertices_a, faces_a);
    manifold::Manifold mesh_b = eigen_to_manifold_direct(vertices_b, faces_b);

    // Check if meshes are valid
    if (mesh_a.IsEmpty() || mesh_b.IsEmpty()) {
      set_last_error(core::Error{core::ErrorCategory::Validation, core::ErrorCode::EmptyMesh,
                                 "One or both meshes are empty or invalid"});
      return {Eigen::MatrixXd(0, 3), Eigen::MatrixXi(0, 3)};
    }

    // Perform boolean operation (same logic as before)
    manifold::Manifold result = perform_manifold_operation(mesh_a, mesh_b, operation_type);

    if (result.IsEmpty()) {
      set_last_error(core::Error{core::ErrorCategory::Geometry, core::ErrorCode::BooleanOperationFailed,
                                 "Boolean operation returned empty result"});
      return {Eigen::MatrixXd(0, 3), Eigen::MatrixXi(0, 3)};
    }

    // Convert back to Eigen matrices directly
    return manifold_to_eigen_direct(result);

  } catch (const std::exception& e) {
    set_last_error(core::Error{core::ErrorCategory::Geometry, core::ErrorCode::Unknown,
                               std::string("Manifold exception: ") + e.what()});
    return {Eigen::MatrixXd(0, 3), Eigen::MatrixXi(0, 3)};
  }
}

// Helper: Convert Eigen matrices directly to Manifold (no Mesh intermediate)
Manifold BooleanOps::eigen_to_manifold_direct(const Eigen::MatrixXd& vertices, const Eigen::MatrixXi& faces) {
  MeshGL manifold_mesh;

  // Convert vertices - MeshGL uses flat float array
  const int num_verts = vertices.rows();
  manifold_mesh.vertProperties.resize(num_verts * 3);
  for (int i = 0; i < num_verts; ++i) {
    manifold_mesh.vertProperties[i * 3 + 0] = vertices(i, 0);
    manifold_mesh.vertProperties[i * 3 + 1] = vertices(i, 1);
    manifold_mesh.vertProperties[i * 3 + 2] = vertices(i, 2);
  }

  // Convert faces - MeshGL uses flat uint array
  const int num_faces = faces.rows();
  manifold_mesh.triVerts.resize(num_faces * 3);
  for (int i = 0; i < num_faces; ++i) {
    manifold_mesh.triVerts[i * 3 + 0] = static_cast<uint32_t>(faces(i, 0));
    manifold_mesh.triVerts[i * 3 + 1] = static_cast<uint32_t>(faces(i, 1));
    manifold_mesh.triVerts[i * 3 + 2] = static_cast<uint32_t>(faces(i, 2));
  }

  manifold_mesh.numProp = 3;
  return Manifold(manifold_mesh);
}

// Helper: Convert Manifold directly to Eigen matrices (no Mesh intermediate)
std::pair<Eigen::MatrixXd, Eigen::MatrixXi> BooleanOps::manifold_to_eigen_direct(const Manifold& manifold) {
  MeshGL manifold_mesh = manifold.GetMeshGL();

  // Convert vertices from flat array to Eigen (double precision)
  const size_t num_verts = manifold_mesh.NumVert();
  Eigen::MatrixXd vertices(num_verts, 3);
  for (size_t i = 0; i < num_verts; ++i) {
    vertices(i, 0) = static_cast<double>(manifold_mesh.vertProperties[(i * manifold_mesh.numProp) + 0]);
    vertices(i, 1) = static_cast<double>(manifold_mesh.vertProperties[(i * manifold_mesh.numProp) + 1]);
    vertices(i, 2) = static_cast<double>(manifold_mesh.vertProperties[(i * manifold_mesh.numProp) + 2]);
  }

  // Convert faces from flat array to Eigen
  const size_t num_faces = manifold_mesh.NumTri();
  Eigen::MatrixXi faces(num_faces, 3);
  for (size_t i = 0; i < num_faces; ++i) {
    faces(i, 0) = static_cast<int>(manifold_mesh.triVerts[(i * 3) + 0]);
    faces(i, 1) = static_cast<int>(manifold_mesh.triVerts[(i * 3) + 1]);
    faces(i, 2) = static_cast<int>(manifold_mesh.triVerts[(i * 3) + 2]);
  }

  return {vertices, faces};
}

// Extracted operation logic (called by both new and legacy APIs)
Manifold BooleanOps::perform_manifold_operation(const Manifold& mesh_a, const Manifold& mesh_b, int operation_type) {
  // Same diagnostic output as before
  auto status_a = mesh_a.Status();
  auto status_b = mesh_b.Status();
  double volume_a = mesh_a.Volume();
  double volume_b = mesh_b.Volume();

  std::cout << "Input mesh A: status=" << static_cast<int>(status_a) << ", genus=" << mesh_a.Genus()
            << ", NumVert=" << mesh_a.NumVert() << ", NumTri=" << mesh_a.NumTri() << ", volume=" << volume_a
            << std::endl;

  auto bounds_a = mesh_a.BoundingBox();
  std::cout << "  Bounds A: min(" << bounds_a.min.x << "," << bounds_a.min.y << "," << bounds_a.min.z << ") max("
            << bounds_a.max.x << "," << bounds_a.max.y << "," << bounds_a.max.z << ")" << std::endl;

  std::cout << "Input mesh B: status=" << static_cast<int>(status_b) << ", genus=" << mesh_b.Genus()
            << ", NumVert=" << mesh_b.NumVert() << ", NumTri=" << mesh_b.NumTri() << ", volume=" << volume_b
            << std::endl;

  if (volume_a < 0) {
    std::cout << "  ❌ ERROR: Mesh A has NEGATIVE volume (" << volume_a << ") - faces are inside-out!" << std::endl;
  }
  if (volume_b < 0) {
    std::cout << "  ❌ ERROR: Mesh B has NEGATIVE volume (" << volume_b << ") - faces are inside-out!" << std::endl;
  }

  auto bounds_b = mesh_b.BoundingBox();
  std::cout << "  Bounds B: min(" << bounds_b.min.x << "," << bounds_b.min.y << "," << bounds_b.min.z << ") max("
            << bounds_b.max.x << "," << bounds_b.max.y << "," << bounds_b.max.z << ")" << std::endl;

  bool overlaps = (bounds_a.min.x <= bounds_b.max.x && bounds_a.max.x >= bounds_b.min.x) &&
                  (bounds_a.min.y <= bounds_b.max.y && bounds_a.max.y >= bounds_b.min.y) &&
                  (bounds_a.min.z <= bounds_b.max.z && bounds_a.max.z >= bounds_b.min.z);
  std::cout << "  Meshes " << (overlaps ? "DO" : "DO NOT") << " overlap in bounding boxes" << std::endl;

  if (status_a != manifold::Manifold::Error::NoError) {
    std::cout << "⚠️  Warning: Input mesh A has issues!" << std::endl;
  }

  if (status_b != manifold::Manifold::Error::NoError) {
    std::cout << "⚠️  Warning: Input mesh B has issues!" << std::endl;
  }

  // Perform boolean operation
  manifold::Manifold result;
  switch (operation_type) {
    case 0: // Union
      result = mesh_a + mesh_b;
      std::cout << "Boolean UNION: " << mesh_a.NumTri() << " + " << mesh_b.NumTri() << " = " << result.NumTri()
                << " triangles" << std::endl;
      break;
    case 1: // Intersection
      result = mesh_a ^ mesh_b;
      std::cout << "Boolean INTERSECTION: " << mesh_a.NumTri() << " ^ " << mesh_b.NumTri() << " = " << result.NumTri()
                << " triangles" << std::endl;
      break;
    case 2: // Difference
      result = mesh_a - mesh_b;
      std::cout << "Boolean DIFFERENCE: " << mesh_a.NumTri() << " - " << mesh_b.NumTri() << " = " << result.NumTri()
                << " triangles (raw)" << std::endl;

      std::cout << "  Raw result status: " << static_cast<int>(result.Status()) << ", genus: " << result.Genus()
                << ", volume: " << result.Volume() << std::endl;

      result = result.AsOriginal();
      std::cout << "  After AsOriginal(): " << result.NumTri() << " triangles" << std::endl;
      std::cout << "  Result genus: " << result.Genus() << ", NumVert: " << result.NumVert()
                << ", NumEdge: " << result.NumEdge() << ", Status: " << static_cast<int>(result.Status())
                << ", Volume: " << result.Volume() << std::endl;

      if (result.NumTri() < mesh_a.NumTri()) {
        std::cout << "  ⚠️  WARNING: Result has FEWER triangles than input A!" << std::endl;
        std::cout << "  This suggests interior faces are missing!" << std::endl;
      } else {
        std::cout << "  ✓ Result has more triangles (includes cavity surfaces)" << std::endl;
      }

      if (result.Volume() < 0) {
        std::cout << "  ⚠️  WARNING: Result has NEGATIVE volume - inside out!" << std::endl;
      }
      break;
    default:
      set_last_error(
          core::Error{core::ErrorCategory::Geometry, core::ErrorCode::Unknown, "Unknown boolean operation type"});
      return Manifold();
  }

  // Check for multiple components
  auto decomposed = result.Decompose();
  std::cout << "  Result has " << decomposed.size() << " component(s)" << std::endl;

  if (decomposed.size() > 1) {
    std::cout << "  ⚠️  WARNING: Result has multiple disconnected components!" << std::endl;

    for (size_t i = 0; i < decomposed.size(); ++i) {
      size_t tri_count = decomposed[i].NumTri();
      auto comp_bounds = decomposed[i].BoundingBox();
      std::cout << "    Component " << i << ": " << decomposed[i].NumVert() << " verts, " << tri_count
                << " tris, genus=" << decomposed[i].Genus() << "\n      Bounds: min(" << comp_bounds.min.x << ","
                << comp_bounds.min.y << "," << comp_bounds.min.z << ") max(" << comp_bounds.max.x << ","
                << comp_bounds.max.y << "," << comp_bounds.max.z << ")" << std::endl;
    }

    std::cout << "  ⚠️  Keeping FULL result without decomposition to preserve boolean output" << std::endl;
    std::cout << "  (Decompose() appears to be incorrectly splitting the difference result)" << std::endl;
  }

  // Diagnostic OBJ export
  try {
    auto [vertices, faces] = manifold_to_eigen_direct(result);
    std::ofstream obj_out("/tmp/boolean_result.obj");
    if (obj_out.is_open()) {
      for (int i = 0; i < static_cast<int>(vertices.rows()); ++i) {
        obj_out << "v " << vertices(i, 0) << " " << vertices(i, 1) << " " << vertices(i, 2) << "\n";
      }
      for (int fi = 0; fi < static_cast<int>(faces.rows()); ++fi) {
        obj_out << "f " << (faces(fi, 0) + 1) << " " << (faces(fi, 1) + 1) << " " << (faces(fi, 2) + 1) << "\n";
      }
      obj_out.close();
      std::cout << "Diagnostic OBJ written to /tmp/boolean_result.obj" << std::endl;
    } else {
      std::cout << "Failed to open /tmp/boolean_result.obj for writing" << std::endl;
    }
  } catch (const std::exception& e) {
    std::cout << "Failed to export diagnostic OBJ: " << e.what() << std::endl;
  }

  return result;
}

} // namespace nodo::geometry
