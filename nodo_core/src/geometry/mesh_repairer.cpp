#include "nodo/geometry/mesh_repairer.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace nodo::geometry {

// Thread-local storage for error reporting
thread_local core::Error MeshRepairer::last_error_{
    core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"};

constexpr double DEFAULT_TOLERANCE = DEFAULT_VERTEX_MERGE_TOLERANCE;

std::string MeshRepairer::RepairResult::summary() const {
  std::ostringstream oss;
  oss << "Mesh Repair Summary:\n";
  oss << "  Success: " << (success ? "YES" : "NO") << "\n";
  if (!message.empty()) {
    oss << "  Message: " << message << "\n";
  }
  oss << "  Faces removed: " << faces_removed << "\n";
  oss << "  Vertices merged: " << vertices_merged << "\n";
  oss << "  Vertices removed: " << vertices_removed << "\n";
  oss << "  Faces reoriented: " << faces_reoriented << "\n";
  oss << "\nFinal mesh state:\n";
  oss << final_report.summary();
  return oss.str();
}

MeshRepairer::RepairResult MeshRepairer::repair(core::Mesh &mesh,
                                                const RepairOptions &options) {
  RepairResult result;

  if (mesh.vertices().rows() == 0 || mesh.faces().rows() == 0) {
    result.message = "Empty mesh - nothing to repair";
    result.success = true;
    return result;
  }

  if (options.verbose) {
    // Initial validation
    auto initial_report = MeshValidator::validate(mesh);
    std::cout << "Initial mesh state:\n" << initial_report.summary() << "\n";
  }

  try {
    // Step 1: Remove degenerate faces
    if (options.remove_degenerate_faces) {
      result.faces_removed = remove_degenerate_faces(mesh);
      if (options.verbose && result.faces_removed > 0) {
        std::cout << "Removed " << result.faces_removed
                  << " degenerate faces\n";
      }
    }

    // Step 2: Merge duplicate vertices
    if (options.merge_duplicate_vertices) {
      result.vertices_merged =
          merge_duplicate_vertices(mesh, options.vertex_merge_tolerance);
      if (options.verbose && result.vertices_merged > 0) {
        std::cout << "Merged " << result.vertices_merged
                  << " duplicate vertices\n";
      }
    }

    // Step 3: Remove unreferenced vertices
    if (options.remove_unreferenced_vertices) {
      result.vertices_removed = remove_unreferenced_vertices(mesh);
      if (options.verbose && result.vertices_removed > 0) {
        std::cout << "Removed " << result.vertices_removed
                  << " unreferenced vertices\n";
      }
    }

    // Step 4: Fix face orientation
    if (options.fix_face_orientation) {
      result.faces_reoriented = fix_face_orientation(mesh);
      if (options.verbose && result.faces_reoriented > 0) {
        std::cout << "Reoriented " << result.faces_reoriented << " faces\n";
      }
    }

    // Final validation
    result.final_report = MeshValidator::validate(mesh);
    result.success = result.final_report.is_valid;

    if (result.success) {
      result.message = "Mesh repaired successfully";
    } else {
      result.message = "Mesh repair completed but issues remain";
    }

    if (options.verbose) {
      std::cout << "\nFinal mesh state:\n"
                << result.final_report.summary() << "\n";
    }

  } catch (const std::exception &e) {
    result.success = false;
    result.message = std::string("Repair failed: ") + e.what();
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::Unknown, result.message});
  }

  return result;
}

int MeshRepairer::remove_degenerate_faces(core::Mesh &mesh) {
  auto degenerate_faces = MeshValidator::find_degenerate_faces(mesh);

  if (degenerate_faces.empty()) {
    return 0;
  }

  // Sort in descending order to remove from back to front
  std::sort(degenerate_faces.rbegin(), degenerate_faces.rend());

  // Create new face matrix without degenerate faces
  const int original_face_count = mesh.faces().rows();
  const int new_face_count = original_face_count - degenerate_faces.size();

  Eigen::MatrixXi new_faces(new_face_count, 3);
  int dest_idx = 0;

  std::unordered_set<int> degenerate_set(degenerate_faces.begin(),
                                         degenerate_faces.end());

  for (int i = 0; i < original_face_count; ++i) {
    if (degenerate_set.find(i) == degenerate_set.end()) {
      new_faces.row(dest_idx++) = mesh.faces().row(i);
    }
  }

  mesh.faces() = new_faces;
  return degenerate_faces.size();
}

int MeshRepairer::merge_duplicate_vertices(core::Mesh &mesh, double tolerance) {
  auto vertex_mapping = build_vertex_mapping(mesh, tolerance);

  // Count merged vertices
  int merged_count = 0;
  for (size_t i = 0; i < vertex_mapping.size(); ++i) {
    if (vertex_mapping[i] != static_cast<int>(i)) {
      merged_count++;
    }
  }

  if (merged_count == 0) {
    return 0;
  }

  // Build unique vertex list
  std::vector<bool> vertex_keep(mesh.vertices().rows(), false);
  for (int mapping : vertex_mapping) {
    vertex_keep[mapping] = true;
  }

  // Create mapping from old unique indices to new compact indices
  std::vector<int> compact_mapping(mesh.vertices().rows(), -1);
  int new_vertex_count = 0;
  for (int i = 0; i < mesh.vertices().rows(); ++i) {
    if (vertex_keep[i]) {
      compact_mapping[i] = new_vertex_count++;
    }
  }

  // Create new vertex matrix
  Eigen::MatrixXd new_vertices(new_vertex_count, 3);
  new_vertex_count = 0;
  for (int i = 0; i < mesh.vertices().rows(); ++i) {
    if (vertex_keep[i]) {
      new_vertices.row(new_vertex_count++) = mesh.vertices().row(i);
    }
  }

  // Update face indices
  for (int i = 0; i < mesh.faces().rows(); ++i) {
    for (int j = 0; j < 3; ++j) {
      int old_vertex = mesh.faces()(i, j);
      int unique_vertex = vertex_mapping[old_vertex];
      int new_vertex = compact_mapping[unique_vertex];
      mesh.faces()(i, j) = new_vertex;
    }
  }

  mesh.vertices() = new_vertices;
  return merged_count;
}

int MeshRepairer::remove_unreferenced_vertices(core::Mesh &mesh) {
  auto unreferenced = MeshValidator::find_unreferenced_vertices(mesh);

  if (unreferenced.empty()) {
    return 0;
  }

  // Create keep mask
  std::vector<bool> vertex_keep(mesh.vertices().rows(), true);
  for (int vertex_idx : unreferenced) {
    vertex_keep[vertex_idx] = false;
  }

  compact_mesh(mesh, vertex_keep);
  return unreferenced.size();
}

int MeshRepairer::fix_face_orientation(core::Mesh &mesh) {
  // Simple heuristic: ensure consistent winding order
  // This is a basic implementation - more sophisticated algorithms exist

  if (mesh.faces().rows() < 2) {
    return 0;
  }

  int flipped_count = 0;
  std::vector<bool> visited(mesh.faces().rows(), false);
  std::vector<bool> should_flip(mesh.faces().rows(), false);

  // Start with first face as reference orientation
  visited[0] = true;

  // Simple propagation based on shared edges
  bool changed = true;
  while (changed) {
    changed = false;

    for (int i = 0; i < mesh.faces().rows(); ++i) {
      if (!visited[i])
        continue;

      for (int j = 0; j < mesh.faces().rows(); ++j) {
        if (visited[j] || i == j)
          continue;

        // Check if faces share an edge
        int shared_vertices = 0;
        for (int vi = 0; vi < 3; ++vi) {
          for (int vj = 0; vj < 3; ++vj) {
            if (mesh.faces()(i, vi) == mesh.faces()(j, vj)) {
              shared_vertices++;
            }
          }
        }

        if (shared_vertices >= 2) {
          // Faces share an edge - check orientation
          visited[j] = true;
          changed = true;

          // Simple heuristic: if shared edge has same direction in both faces,
          // one should be flipped
          // This is simplified - real algorithm would check edge orientation
          should_flip[j] = should_flip[i]; // Placeholder logic
        }
      }
    }
  }

  // Apply flips
  for (int i = 0; i < mesh.faces().rows(); ++i) {
    if (should_flip[i]) {
      std::swap(mesh.faces()(i, 1), mesh.faces()(i, 2));
      flipped_count++;
    }
  }

  return flipped_count;
}

bool MeshRepairer::make_manifold(core::Mesh &mesh) {
  // Remove non-manifold edges by removing affected faces
  auto non_manifold_faces = MeshValidator::find_non_manifold_edges(mesh);

  if (non_manifold_faces.empty()) {
    return true;
  }

  // Sort in descending order
  std::sort(non_manifold_faces.rbegin(), non_manifold_faces.rend());

  // Remove non-manifold faces
  const int original_face_count = mesh.faces().rows();
  const int new_face_count = original_face_count - non_manifold_faces.size();

  if (new_face_count <= 0) {
    return false;
  }

  Eigen::MatrixXi new_faces(new_face_count, 3);
  int dest_idx = 0;

  std::unordered_set<int> remove_set(non_manifold_faces.begin(),
                                     non_manifold_faces.end());

  for (int i = 0; i < original_face_count; ++i) {
    if (remove_set.find(i) == remove_set.end()) {
      new_faces.row(dest_idx++) = mesh.faces().row(i);
    }
  }

  mesh.faces() = new_faces;

  // Clean up unreferenced vertices
  remove_unreferenced_vertices(mesh);

  return MeshValidator::is_manifold(mesh);
}

int MeshRepairer::recalculate_normals(core::Mesh & /* mesh */) {
  // This would implement normal recalculation
  // For now, just return 0 as we don't store normals in our basic mesh
  return 0;
}

void MeshRepairer::compact_mesh(core::Mesh &mesh,
                                const std::vector<bool> &vertex_keep_mask) {
  // Build index mapping
  std::vector<int> new_indices(mesh.vertices().rows(), -1);
  int new_vertex_count = 0;

  for (int i = 0; i < mesh.vertices().rows(); ++i) {
    if (vertex_keep_mask[i]) {
      new_indices[i] = new_vertex_count++;
    }
  }

  // Create new vertex matrix
  Eigen::MatrixXd new_vertices(new_vertex_count, 3);
  new_vertex_count = 0;

  for (int i = 0; i < mesh.vertices().rows(); ++i) {
    if (vertex_keep_mask[i]) {
      new_vertices.row(new_vertex_count++) = mesh.vertices().row(i);
    }
  }

  // Update face indices
  for (int i = 0; i < mesh.faces().rows(); ++i) {
    for (int j = 0; j < 3; ++j) {
      int old_index = mesh.faces()(i, j);
      mesh.faces()(i, j) = new_indices[old_index];
    }
  }

  mesh.vertices() = new_vertices;
}

std::vector<int> MeshRepairer::build_vertex_mapping(const core::Mesh &mesh,
                                                    double tolerance) {
  std::vector<int> mapping(mesh.vertices().rows());
  const double tolerance_sq = tolerance * tolerance;

  // Initialize with identity mapping
  for (int i = 0; i < mesh.vertices().rows(); ++i) {
    mapping[i] = i;
  }

  // Find duplicates and map to first occurrence
  for (int i = 0; i < mesh.vertices().rows(); ++i) {
    if (mapping[i] != i)
      continue; // Already mapped to earlier vertex

    for (int j = i + 1; j < mesh.vertices().rows(); ++j) {
      if (mapping[j] != j)
        continue; // Already mapped

      Eigen::Vector3d diff = mesh.vertices().row(i) - mesh.vertices().row(j);
      if (diff.squaredNorm() < tolerance_sq) {
        mapping[j] = i; // Map j to i
      }
    }
  }

  return mapping;
}

const core::Error &MeshRepairer::last_error() { return last_error_; }

void MeshRepairer::set_last_error(const core::Error &error) {
  last_error_ = error;
}

} // namespace nodo::geometry
