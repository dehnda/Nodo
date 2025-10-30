#include "nodo/geometry/boolean_ops.hpp"
#include <array>
#include <fstream>
#include <iostream>
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

  Manifold result(manifold_mesh);

  // Verify the mesh is properly oriented (positive volume)
  // If genus is valid (0 for sphere), check if we need to flip
  if (result.Status() == Manifold::Error::NoError && result.Genus() >= 0) {
    // Manifold should handle this automatically, but let's verify
    auto bounds = result.BoundingBox();
    std::cout << "    [eigen_to_manifold] Created manifold with status="
              << static_cast<int>(result.Status())
              << ", genus=" << result.Genus() << ", NumTri=" << result.NumTri()
              << std::endl;
  }

  return result;
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

    // Check input mesh status and orientation
    auto status_a = mesh_a.Status();
    auto status_b = mesh_b.Status();
    double volume_a = mesh_a.Volume();
    double volume_b = mesh_b.Volume();

    std::cout << "Input mesh A: status=" << static_cast<int>(status_a)
              << ", genus=" << mesh_a.Genus()
              << ", NumVert=" << mesh_a.NumVert()
              << ", NumTri=" << mesh_a.NumTri() << ", volume=" << volume_a
              << std::endl;

    auto bounds_a = mesh_a.BoundingBox();
    std::cout << "  Bounds A: min(" << bounds_a.min.x << "," << bounds_a.min.y
              << "," << bounds_a.min.z << ") max(" << bounds_a.max.x << ","
              << bounds_a.max.y << "," << bounds_a.max.z << ")" << std::endl;

    std::cout << "Input mesh B: status=" << static_cast<int>(status_b)
              << ", genus=" << mesh_b.Genus()
              << ", NumVert=" << mesh_b.NumVert()
              << ", NumTri=" << mesh_b.NumTri() << ", volume=" << volume_b
              << std::endl;

    // Check for negative volumes (inside-out meshes)
    if (volume_a < 0) {
      std::cout << "  ❌ ERROR: Mesh A has NEGATIVE volume (" << volume_a
                << ") - faces are inside-out!" << std::endl;
    }
    if (volume_b < 0) {
      std::cout << "  ❌ ERROR: Mesh B has NEGATIVE volume (" << volume_b
                << ") - faces are inside-out!" << std::endl;
    }

    auto bounds_b = mesh_b.BoundingBox();
    std::cout << "  Bounds B: min(" << bounds_b.min.x << "," << bounds_b.min.y
              << "," << bounds_b.min.z << ") max(" << bounds_b.max.x << ","
              << bounds_b.max.y << "," << bounds_b.max.z << ")" << std::endl;

    // Check if they overlap
    bool overlaps =
        (bounds_a.min.x <= bounds_b.max.x &&
         bounds_a.max.x >= bounds_b.min.x) &&
        (bounds_a.min.y <= bounds_b.max.y &&
         bounds_a.max.y >= bounds_b.min.y) &&
        (bounds_a.min.z <= bounds_b.max.z && bounds_a.max.z >= bounds_b.min.z);
    std::cout << "  Meshes " << (overlaps ? "DO" : "DO NOT")
              << " overlap in bounding boxes" << std::endl;

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
      std::cout << "Boolean UNION: " << mesh_a.NumTri() << " + "
                << mesh_b.NumTri() << " = " << result.NumTri() << " triangles"
                << std::endl;
      break;
    case 1: // Intersection
      result = mesh_a ^ mesh_b;
      std::cout << "Boolean INTERSECTION: " << mesh_a.NumTri() << " ^ "
                << mesh_b.NumTri() << " = " << result.NumTri() << " triangles"
                << std::endl;
      break;
    case 2: // Difference
      result = mesh_a - mesh_b;
      std::cout << "Boolean DIFFERENCE: " << mesh_a.NumTri() << " - "
                << mesh_b.NumTri() << " = " << result.NumTri()
                << " triangles (raw)" << std::endl;

      // Try AsOriginal() to resolve the mesh (genus -1 indicates non-manifold
      // intermediate)
      result = result.AsOriginal();
      std::cout << "  After AsOriginal(): " << result.NumTri() << " triangles"
                << std::endl;
      std::cout << "  Result genus: " << result.Genus()
                << ", NumVert: " << result.NumVert()
                << ", NumEdge: " << result.NumEdge() << std::endl;

      // Expected: A difference should have MORE triangles than A alone
      // because it includes interior cavity faces
      if (result.NumTri() < mesh_a.NumTri()) {
        std::cout << "  ⚠️  WARNING: Result has FEWER triangles than input A!"
                  << std::endl;
        std::cout << "  This suggests interior faces are missing!" << std::endl;
      } else {
        std::cout << "  ✓ Result has more triangles (includes cavity surfaces)"
                  << std::endl;
      }
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

    // Check if result has multiple components (disconnected meshes)
    auto decomposed = result.Decompose();
    std::cout << "  Result has " << decomposed.size() << " component(s)"
              << std::endl;

    if (decomposed.size() > 1) {
      std::cout << "  ⚠️  WARNING: Result has multiple disconnected components!"
                << std::endl;

      // List all components with bounding boxes to understand what each
      // represents
      for (size_t i = 0; i < decomposed.size(); ++i) {
        size_t tri_count = decomposed[i].NumTri();
        auto comp_bounds = decomposed[i].BoundingBox();
        std::cout << "    Component " << i << ": " << decomposed[i].NumVert()
                  << " verts, " << tri_count
                  << " tris, genus=" << decomposed[i].Genus()
                  << "\n      Bounds: min(" << comp_bounds.min.x << ","
                  << comp_bounds.min.y << "," << comp_bounds.min.z << ") max("
                  << comp_bounds.max.x << "," << comp_bounds.max.y << ","
                  << comp_bounds.max.z << ")" << std::endl;
      }

      // IMPORTANT: For difference operations, Manifold sometimes incorrectly
      // decomposes the result. Instead of trusting Decompose(), we should NOT
      // decompose and keep the full result as Manifold computed it. The
      // decomposition seems to be splitting it wrong - one is original A, one
      // is intersection
      std::cout << "  ⚠️  Keeping FULL result without decomposition to preserve "
                   "boolean output"
                << std::endl;
      std::cout << "  (Decompose() appears to be incorrectly splitting the "
                   "difference result)"
                << std::endl;

      // Do NOT reassign result - keep it as the full manifold computed it
    }

    // Diagnostic: export the manifold result to OBJ for external inspection
    try {
      auto debug_mesh = manifold_to_eigen(result);
      std::ofstream obj_out("/tmp/boolean_result.obj");
      if (obj_out.is_open()) {
        // Write vertices
        const auto &V = debug_mesh.vertices();
        const auto &F = debug_mesh.faces();
        for (int i = 0; i < static_cast<int>(V.rows()); ++i) {
          obj_out << "v " << V(i, 0) << " " << V(i, 1) << " " << V(i, 2)
                  << "\n";
        }
        // Write faces (1-based indices for OBJ)
        for (int fi = 0; fi < static_cast<int>(F.rows()); ++fi) {
          obj_out << "f " << (F(fi, 0) + 1) << " " << (F(fi, 1) + 1) << " "
                  << (F(fi, 2) + 1) << "\n";
        }
        obj_out.close();
        std::cout << "Diagnostic OBJ written to /tmp/boolean_result.obj"
                  << std::endl;
      } else {
        std::cout << "Failed to open /tmp/boolean_result.obj for writing"
                  << std::endl;
      }
    } catch (const std::exception &e) {
      std::cout << "Failed to export diagnostic OBJ: " << e.what() << std::endl;
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
