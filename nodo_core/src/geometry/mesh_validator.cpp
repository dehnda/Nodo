#include "nodo/geometry/mesh_validator.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace nodo::geometry {

// Thread-local storage for error reporting
thread_local core::Error MeshValidator::last_error_{
    core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"};

// Constants for reporting limits
constexpr size_t MAX_LISTED_ITEMS = 10;
constexpr double HALF_FACTOR = 0.5;
constexpr double AREA_TOLERANCE = 1e-12;

std::string ValidationReport::summary() const {
  std::ostringstream oss;
  oss << "Mesh Validation Report:\n";
  oss << "  Valid: " << (is_valid ? "YES" : "NO") << "\n";
  oss << "  Vertices: " << num_vertices << "\n";
  oss << "  Faces: " << num_faces << "\n";
  oss << "  Edges: " << num_edges << "\n";
  oss << "  Boundary Edges: " << num_boundary_edges << "\n";
  oss << "  Non-manifold Edges: " << num_non_manifold_edges << "\n";
  oss << "  Isolated Vertices: " << num_isolated_vertices << "\n";
  oss << "  Is Manifold: " << (is_manifold ? "YES" : "NO") << "\n";
  oss << "  Is Closed: " << (is_closed ? "YES" : "NO") << "\n";
  oss << "  Has Self-intersections: " << (has_self_intersections ? "YES" : "NO")
      << "\n";
  oss << "  Has Degenerate Faces: " << (has_degenerate_faces ? "YES" : "NO")
      << "\n";
  oss << "  Has Duplicate Vertices: " << (has_duplicate_vertices ? "YES" : "NO")
      << "\n";
  oss << "  Has Unreferenced Vertices: "
      << (has_unreferenced_vertices ? "YES" : "NO") << "\n";

  // Show some indices for debugging
  if (!degenerate_face_indices.empty()) {
    oss << "  Degenerate faces (first "
        << std::min(MAX_LISTED_ITEMS, degenerate_face_indices.size()) << "): ";
    for (size_t i = 0;
         i < std::min(MAX_LISTED_ITEMS, degenerate_face_indices.size()); ++i) {
      oss << degenerate_face_indices[i] << " ";
    }
    if (degenerate_face_indices.size() > MAX_LISTED_ITEMS)
      oss << "...";
    oss << "\n";
  }

  if (!duplicate_vertex_indices.empty()) {
    oss << "  Duplicate vertices (first "
        << std::min(MAX_LISTED_ITEMS, duplicate_vertex_indices.size()) << "): ";
    for (size_t i = 0;
         i < std::min(MAX_LISTED_ITEMS, duplicate_vertex_indices.size()); ++i) {
      oss << duplicate_vertex_indices[i] << " ";
    }
    if (duplicate_vertex_indices.size() > MAX_LISTED_ITEMS)
      oss << "...";
    oss << "\n";
  }

  if (!unreferenced_vertex_indices.empty()) {
    oss << "  Unreferenced vertices (first "
        << std::min(MAX_LISTED_ITEMS, unreferenced_vertex_indices.size())
        << "): ";
    for (size_t i = 0;
         i < std::min(MAX_LISTED_ITEMS, unreferenced_vertex_indices.size());
         ++i) {
      oss << unreferenced_vertex_indices[i] << " ";
    }
    if (unreferenced_vertex_indices.size() > MAX_LISTED_ITEMS)
      oss << "...";
    oss << "\n";
  }

  return oss.str();
}

std::string ValidationReport::detailed_report() const {
  return summary(); // For now, detailed report is the same as summary
}

ValidationReport MeshValidator::validate(const core::Mesh& mesh) {
  ValidationReport report;

  if (mesh.vertices().rows() == 0 || mesh.faces().rows() == 0) {
    report.is_valid = false;
    return report;
  }

  try {
    // Basic statistics
    report.num_vertices = mesh.vertices().rows();
    report.num_faces = mesh.faces().rows();

    // Calculate statistics (including edge count)
    calculate_statistics(mesh, report);

    // Check for degenerate faces
    auto degenerate_faces = find_degenerate_faces(mesh);
    report.has_degenerate_faces = !degenerate_faces.empty();
    report.degenerate_face_indices = degenerate_faces;

    // Check for duplicate vertices
    auto duplicate_vertices = find_duplicate_vertices(mesh);
    report.has_duplicate_vertices = !duplicate_vertices.empty();
    report.duplicate_vertex_indices = duplicate_vertices;

    // Check for unreferenced vertices
    auto unreferenced_vertices = find_unreferenced_vertices(mesh);
    report.has_unreferenced_vertices = !unreferenced_vertices.empty();
    report.unreferenced_vertex_indices = unreferenced_vertices;

    // Check if manifold and store non-manifold edges
    auto non_manifold_faces = find_non_manifold_edges(mesh);
    report.is_manifold = non_manifold_faces.empty();
    report.non_manifold_edge_indices = non_manifold_faces;
    report.num_non_manifold_edges = non_manifold_faces.size();

    // Check if closed
    report.is_closed = is_closed(mesh);

    // Overall validity
    report.is_valid = !report.has_degenerate_faces &&
                      !report.has_duplicate_vertices && report.is_manifold;

  } catch (const std::exception& e) {
    report.is_valid = false;
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::Unknown,
                               std::string("Validation failed: ") + e.what()});
  }

  return report;
}

bool MeshValidator::is_closed(const core::Mesh& mesh) {
  // Edge structure for tracking edge occurrences
  struct Edge {
    int vertex1, vertex2;

    Edge(int vert1, int vert2)
        : vertex1(std::min(vert1, vert2)), vertex2(std::max(vert1, vert2)) {}

    bool operator<(const Edge& other) const {
      return vertex1 < other.vertex1 ||
             (vertex1 == other.vertex1 && vertex2 < other.vertex2);
    }
  };

  std::map<Edge, int> edge_count;

  // Count occurrences of each edge
  // Handle both triangles and quads (or any n-gon)
  for (int face_idx = 0; face_idx < mesh.faces().rows(); ++face_idx) {
    int num_verts = mesh.faces().cols(); // 3 for triangles, 4 for quads
    for (int edge_idx = 0; edge_idx < num_verts; ++edge_idx) {
      int vert1 = mesh.faces()(face_idx, edge_idx);
      int vert2 = mesh.faces()(face_idx, (edge_idx + 1) % num_verts);
      Edge edge(vert1, vert2);
      edge_count[edge]++;
    }
  }

  // In a closed mesh, every edge should appear exactly twice
  bool all_internal = true;
  for (const auto& pair : edge_count) {
    if (pair.second != 2) {
      all_internal = false;
      break;
    }
  }

  return all_internal;
}

std::vector<int> MeshValidator::find_degenerate_faces(const core::Mesh& mesh) {
  std::vector<int> degenerate_faces;

  for (int face_idx = 0; face_idx < mesh.faces().rows(); ++face_idx) {
    const int vert1 = mesh.faces()(face_idx, 0);
    const int vert2 = mesh.faces()(face_idx, 1);
    const int vert3 = mesh.faces()(face_idx, 2);

    // Check for duplicate vertices in the face
    if (vert1 == vert2 || vert2 == vert3 || vert1 == vert3) {
      degenerate_faces.push_back(face_idx);
      continue;
    }

    // Check for zero area (collinear vertices)
    Eigen::Vector3d vec1 = mesh.vertices().row(vert1);
    Eigen::Vector3d vec2 = mesh.vertices().row(vert2);
    Eigen::Vector3d vec3 = mesh.vertices().row(vert3);

    Eigen::Vector3d edge1 = vec2 - vec1;
    Eigen::Vector3d edge2 = vec3 - vec1;

    // Cross product gives area (doubled)
    Eigen::Vector3d cross = edge1.cross(edge2);
    double area = cross.norm() * HALF_FACTOR;

    if (area < AREA_TOLERANCE) {
      degenerate_faces.push_back(face_idx);
    }
  }

  return degenerate_faces;
}

std::vector<int> MeshValidator::find_duplicate_vertices(const core::Mesh& mesh,
                                                        double tolerance) {
  std::vector<int> duplicates;
  const double tolerance_sq = tolerance * tolerance;

  for (int i = 0; i < mesh.vertices().rows(); ++i) {
    for (int j = i + 1; j < mesh.vertices().rows(); ++j) {
      Eigen::Vector3d diff = mesh.vertices().row(i) - mesh.vertices().row(j);
      if (diff.squaredNorm() < tolerance_sq) {
        duplicates.push_back(j); // Mark the later one as duplicate
      }
    }
  }

  return duplicates;
}

std::vector<int>
MeshValidator::find_unreferenced_vertices(const core::Mesh& mesh) {
  std::vector<bool> referenced(mesh.vertices().rows(), false);

  // Mark all referenced vertices
  for (int face_idx = 0; face_idx < mesh.faces().rows(); ++face_idx) {
    for (int vertex_in_face = 0; vertex_in_face < 3; ++vertex_in_face) {
      int vertex_idx = mesh.faces()(face_idx, vertex_in_face);
      if (vertex_idx >= 0 && vertex_idx < mesh.vertices().rows()) {
        referenced[vertex_idx] = true;
      }
    }
  }

  // Collect unreferenced vertices
  std::vector<int> unreferenced;
  for (int i = 0; i < mesh.vertices().rows(); ++i) {
    if (!referenced[i]) {
      unreferenced.push_back(i);
    }
  }

  return unreferenced;
}

bool MeshValidator::is_manifold(const core::Mesh& mesh) {
  return find_non_manifold_edges(mesh).empty();
}

std::vector<int>
MeshValidator::find_non_manifold_edges(const core::Mesh& mesh) {
  // Edge structure for tracking which faces use each edge
  struct Edge {
    int vertex1, vertex2;

    Edge(int vert1, int vert2)
        : vertex1(std::min(vert1, vert2)), vertex2(std::max(vert1, vert2)) {}

    bool operator<(const Edge& other) const {
      return vertex1 < other.vertex1 ||
             (vertex1 == other.vertex1 && vertex2 < other.vertex2);
    }
  };

  std::map<Edge, std::vector<int>> edge_to_faces;

  // Build edge to faces mapping
  for (int face_idx = 0; face_idx < mesh.faces().rows(); ++face_idx) {
    for (int edge_idx = 0; edge_idx < 3; ++edge_idx) {
      int vert1 = mesh.faces()(face_idx, edge_idx);
      int vert2 = mesh.faces()(face_idx, (edge_idx + 1) % 3);
      Edge edge(vert1, vert2);
      edge_to_faces[edge].push_back(face_idx);
    }
  }

  // Find faces adjacent to non-manifold edges (edges shared by more than 2
  // faces)
  std::vector<int> non_manifold_faces;
  for (const auto& pair : edge_to_faces) {
    if (pair.second.size() > 2) {
      // Edge is shared by more than 2 faces - non-manifold
      for (int face_idx : pair.second) {
        non_manifold_faces.push_back(face_idx);
      }
    }
  }

  // Remove duplicates
  std::sort(non_manifold_faces.begin(), non_manifold_faces.end());
  non_manifold_faces.erase(
      std::unique(non_manifold_faces.begin(), non_manifold_faces.end()),
      non_manifold_faces.end());

  return non_manifold_faces;
}

void MeshValidator::calculate_statistics(const core::Mesh& mesh,
                                         ValidationReport& report) {
  // Edge structure for counting unique edges
  struct Edge {
    int vertex1, vertex2;

    Edge(int vert1, int vert2)
        : vertex1(std::min(vert1, vert2)), vertex2(std::max(vert1, vert2)) {}

    bool operator<(const Edge& other) const {
      return vertex1 < other.vertex1 ||
             (vertex1 == other.vertex1 && vertex2 < other.vertex2);
    }
  };

  std::map<Edge, int> edge_count;

  // Count edges
  for (int face_idx = 0; face_idx < mesh.faces().rows(); ++face_idx) {
    for (int edge_idx = 0; edge_idx < 3; ++edge_idx) {
      int vert1 = mesh.faces()(face_idx, edge_idx);
      int vert2 = mesh.faces()(face_idx, (edge_idx + 1) % 3);
      edge_count[Edge(vert1, vert2)]++;
    }
  }

  report.num_edges = edge_count.size();

  // Count boundary edges (appear only once)
  int boundary_edges = 0;
  for (const auto& pair : edge_count) {
    if (pair.second == 1) {
      boundary_edges++;
    }
  }
  report.num_boundary_edges = boundary_edges;

  // Count isolated vertices (unreferenced vertices)
  report.num_isolated_vertices = find_unreferenced_vertices(mesh).size();
}

const core::Error& MeshValidator::last_error() {
  return last_error_;
}

void MeshValidator::set_last_error(const core::Error& error) {
  last_error_ = error;
}

} // namespace nodo::geometry
