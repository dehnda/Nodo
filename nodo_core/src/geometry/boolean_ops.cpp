#include "nodo/geometry/boolean_ops.hpp"
#include <array>
#include <manifold/manifold.h>
#include <vector>

using namespace manifold;

namespace nodo::geometry {

// Thread-local storage for error reporting
thread_local core::Error BooleanOps::last_error_{
    core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"};

std::optional<core::Mesh> BooleanOps::union_meshes(const core::Mesh &a,
                                                   const core::Mesh &b) {
  if (!are_compatible(a, b)) {
    set_last_error(core::Error{
        core::ErrorCategory::Validation, core::ErrorCode::InvalidMesh,
        "Meshes are not compatible for boolean operations"});
    return std::nullopt;
  }

  return manifold_boolean_operation(a, b, 0); // 0 = union
}

std::optional<core::Mesh> BooleanOps::intersect_meshes(const core::Mesh &a,
                                                       const core::Mesh &b) {
  if (!are_compatible(a, b)) {
    set_last_error(core::Error{
        core::ErrorCategory::Validation, core::ErrorCode::InvalidMesh,
        "Meshes are not compatible for boolean operations"});
    return std::nullopt;
  }

  return manifold_boolean_operation(a, b, 1); // 1 = intersection
}

std::optional<core::Mesh> BooleanOps::difference_meshes(const core::Mesh &a,
                                                        const core::Mesh &b) {
  if (!are_compatible(a, b)) {
    set_last_error(core::Error{
        core::ErrorCategory::Validation, core::ErrorCode::InvalidMesh,
        "Meshes are not compatible for boolean operations"});
    return std::nullopt;
  }

  return manifold_boolean_operation(a, b, 2); // 2 = difference
}

const core::Error &BooleanOps::last_error() { return last_error_; }

bool BooleanOps::are_compatible(const core::Mesh &a,
                                const core::Mesh &b) noexcept {
  return validate_mesh(a) && validate_mesh(b);
}

bool BooleanOps::validate_mesh(const core::Mesh &mesh) {
  if (mesh.vertices().rows() < 3 || mesh.faces().rows() < 1) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::EmptyMesh,
                               "Mesh has insufficient vertices or faces"});
    return false;
  }

  // Basic validation - ensure faces reference valid vertices
  const int num_vertices = static_cast<int>(mesh.vertices().rows());
  for (int i = 0; i < mesh.faces().rows(); ++i) {
    for (int j = 0; j < 3; ++j) {
      if (mesh.faces()(i, j) >= num_vertices || mesh.faces()(i, j) < 0) {
        set_last_error(core::Error{core::ErrorCategory::Validation,
                                   core::ErrorCode::InvalidMesh,
                                   "Face references invalid vertex index"});
        return false;
      }
    }
  }

  return true;
}

// Helper: Convert Eigen mesh to Manifold format
static Manifold eigen_to_manifold(const core::Mesh &mesh) {
  MeshGL manifold_mesh;

  // Convert vertices - MeshGL uses flat float array
  const int num_verts = mesh.vertices().rows();
  manifold_mesh.vertProperties.resize(num_verts * 3);
  for (int i = 0; i < num_verts; ++i) {
    manifold_mesh.vertProperties[i * 3 + 0] = mesh.vertices()(i, 0);
    manifold_mesh.vertProperties[i * 3 + 1] = mesh.vertices()(i, 1);
    manifold_mesh.vertProperties[i * 3 + 2] = mesh.vertices()(i, 2);
  }

  // Convert faces - MeshGL uses flat uint array
  const int num_faces = mesh.faces().rows();
  manifold_mesh.triVerts.resize(num_faces * 3);
  for (int i = 0; i < num_faces; ++i) {
    manifold_mesh.triVerts[i * 3 + 0] =
        static_cast<uint32_t>(mesh.faces()(i, 0));
    manifold_mesh.triVerts[i * 3 + 1] =
        static_cast<uint32_t>(mesh.faces()(i, 1));
    manifold_mesh.triVerts[i * 3 + 2] =
        static_cast<uint32_t>(mesh.faces()(i, 2));
  }

  // Set number of properties per vertex (x, y, z = 3)
  manifold_mesh.numProp = 3;

  return Manifold(manifold_mesh);
}

// Helper: Convert Manifold back to Eigen mesh
static core::Mesh manifold_to_eigen(const Manifold &manifold) {
  MeshGL manifold_mesh = manifold.GetMeshGL();

  core::Mesh result;

  // Convert vertices from flat array to Eigen (double precision)
  const size_t num_verts = manifold_mesh.NumVert();
  Eigen::Matrix<double, Eigen::Dynamic, 3, Eigen::RowMajor> V(num_verts, 3);
  for (size_t i = 0; i < num_verts; ++i) {
    V(i, 0) = static_cast<double>(
        manifold_mesh.vertProperties[(i * manifold_mesh.numProp) + 0]);
    V(i, 1) = static_cast<double>(
        manifold_mesh.vertProperties[(i * manifold_mesh.numProp) + 1]);
    V(i, 2) = static_cast<double>(
        manifold_mesh.vertProperties[(i * manifold_mesh.numProp) + 2]);
  }

  // Convert faces from flat array to Eigen
  const size_t num_faces = manifold_mesh.NumTri();
  Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor> F(num_faces, 3);
  for (size_t i = 0; i < num_faces; ++i) {
    F(i, 0) = static_cast<int>(manifold_mesh.triVerts[(i * 3) + 0]);
    F(i, 1) = static_cast<int>(manifold_mesh.triVerts[(i * 3) + 1]);
    F(i, 2) = static_cast<int>(manifold_mesh.triVerts[(i * 3) + 2]);
  }

  result.vertices() = V;
  result.faces() = F;

  return result;
}

std::optional<core::Mesh>
BooleanOps::manifold_boolean_operation(const core::Mesh &a, const core::Mesh &b,
                                       int operation_type) {
  try {
    // Convert to Manifold format
    manifold::Manifold mesh_a = eigen_to_manifold(a);
    manifold::Manifold mesh_b = eigen_to_manifold(b);

    // Check if meshes are valid
    if (mesh_a.IsEmpty() || mesh_b.IsEmpty()) {
      set_last_error(core::Error{core::ErrorCategory::Validation,
                                 core::ErrorCode::EmptyMesh,
                                 "One or both meshes are empty or invalid"});
      return std::nullopt;
    }

    // Perform boolean operation
    manifold::Manifold result;
    switch (operation_type) {
    case 0: // Union
      result = mesh_a + mesh_b;
      break;
    case 1: // Intersection
      result = mesh_a ^ mesh_b;
      break;
    case 2: // Difference
      result = mesh_a - mesh_b;
      break;
    default:
      set_last_error(core::Error{core::ErrorCategory::Geometry,
                                 core::ErrorCode::Unknown,
                                 "Unknown boolean operation type"});
      return std::nullopt;
    }

    // Check result
    if (result.IsEmpty()) {
      set_last_error(core::Error{core::ErrorCategory::Geometry,
                                 core::ErrorCode::BooleanOperationFailed,
                                 "Boolean operation returned empty result"});
      return std::nullopt;
    }

    // Convert back to Eigen format
    return manifold_to_eigen(result);

  } catch (const std::exception &e) {
    set_last_error(core::Error{core::ErrorCategory::Geometry,
                               core::ErrorCode::Unknown,
                               std::string("Manifold exception: ") + e.what()});
    return std::nullopt;
  }
}

void BooleanOps::set_last_error(const core::Error &error) {
  last_error_ = error;
}

} // namespace nodo::geometry
