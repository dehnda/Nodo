#include "nodeflux/sop/laplacian_sop.hpp"
#include "nodeflux/core/types.hpp"
#include <cmath>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace nodeflux::sop {

namespace {
/// Default values for smoothing parameters
constexpr int DEFAULT_ITERATIONS = 1;
constexpr double DEFAULT_LAMBDA = 0.5;
constexpr double DEFAULT_MU = -0.51; // Slightly stronger than lambda for Taubin
constexpr bool DEFAULT_PRESERVE_BOUNDARIES = true;
} // namespace

LaplacianSOP::LaplacianSOP(const std::string &name)
    : name_(name), iterations_(DEFAULT_ITERATIONS), lambda_(DEFAULT_LAMBDA),
      mu_(DEFAULT_MU), method_(SmoothingMethod::UNIFORM),
      preserve_boundaries_(DEFAULT_PRESERVE_BOUNDARIES), cache_valid_(false),
      last_input_hash_(0) {}

void LaplacianSOP::set_input_mesh(std::shared_ptr<core::Mesh> mesh) {
  if (input_mesh_ != mesh) {
    input_mesh_ = mesh;
    cache_valid_ = false;
  }
}

void LaplacianSOP::set_iterations(int iterations) {
  if (iterations_ != iterations) {
    iterations_ = std::max(0, iterations);
    cache_valid_ = false;
  }
}

void LaplacianSOP::set_lambda(double lambda) {
  if (lambda_ != lambda) {
    lambda_ = std::clamp(lambda, 0.0, 1.0);
    cache_valid_ = false;
  }
}

void LaplacianSOP::set_method(SmoothingMethod method) {
  if (method_ != method) {
    method_ = method;
    cache_valid_ = false;
  }
}

void LaplacianSOP::set_mu(double mu) {
  if (mu_ != mu) {
    mu_ = mu;
    cache_valid_ = false;
  }
}

void LaplacianSOP::set_preserve_boundaries(bool preserve) {
  if (preserve_boundaries_ != preserve) {
    preserve_boundaries_ = preserve;
    cache_valid_ = false;
  }
}

std::shared_ptr<core::Mesh> LaplacianSOP::cook() {
  if (cache_valid_ && cached_result_ && !needs_recalculation()) {
    return cached_result_;
  }

  auto start_time = std::chrono::high_resolution_clock::now();

  std::cout << "LaplacianSOP '" << name_ << "': Computing smoothing...\n";

  cached_result_ = execute();

  auto end_time = std::chrono::high_resolution_clock::now();
  last_cook_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);

  if (cached_result_) {
    std::cout << "LaplacianSOP '" << name_ << "': Completed in "
              << last_cook_time_.count() << "ms\n";
    std::cout << "  Result: " << cached_result_->vertices().rows()
              << " vertices, " << cached_result_->faces().rows() << " faces\n";
    cache_valid_ = true;

    // Update input hash for change detection
    if (input_mesh_) {
      std::hash<size_t> hasher;
      last_input_hash_ =
          hasher(input_mesh_->vertices().rows() * input_mesh_->faces().rows());
    }
  } else {
    std::cerr << "LaplacianSOP '" << name_ << "': Failed to execute\n";
  }

  return cached_result_;
}

std::shared_ptr<core::Mesh> LaplacianSOP::execute() {
  if (!validate_parameters()) {
    return nullptr;
  }

  const auto &original_vertices = input_mesh_->vertices();
  const auto &faces = input_mesh_->faces();

  Eigen::MatrixXd smoothed_vertices;

  switch (method_) {
  case SmoothingMethod::UNIFORM:
    smoothed_vertices = apply_uniform_laplacian(original_vertices, faces);
    break;
  case SmoothingMethod::COTANGENT:
    smoothed_vertices = apply_cotangent_laplacian(original_vertices, faces);
    break;
  case SmoothingMethod::TAUBIN:
    smoothed_vertices = apply_taubin_smoothing(original_vertices, faces);
    break;
  }

  return std::make_shared<core::Mesh>(smoothed_vertices, faces);
}

Eigen::MatrixXd
LaplacianSOP::apply_uniform_laplacian(const Eigen::MatrixXd &vertices,
                                      const Eigen::MatrixXi &faces) {
  Eigen::MatrixXd result = vertices;
  auto boundary_vertices = find_boundary_vertices(faces, vertices.rows());

  for (int iter = 0; iter < iterations_; ++iter) {
    auto laplacian = build_uniform_laplacian(faces, vertices.rows());
    Eigen::MatrixXd laplacian_coords = laplacian * result;

    // Apply smoothing update
    for (int i = 0; i < result.rows(); ++i) {
      if (preserve_boundaries_ && boundary_vertices[i]) {
        continue; // Skip boundary vertices
      }
      result.row(i) += lambda_ * laplacian_coords.row(i);
    }
  }

  return result;
}

Eigen::MatrixXd
LaplacianSOP::apply_cotangent_laplacian(const Eigen::MatrixXd &vertices,
                                        const Eigen::MatrixXi &faces) {
  Eigen::MatrixXd result = vertices;
  auto boundary_vertices = find_boundary_vertices(faces, vertices.rows());

  for (int iter = 0; iter < iterations_; ++iter) {
    auto laplacian = build_cotangent_laplacian(result, faces);
    Eigen::MatrixXd laplacian_coords = laplacian * result;

    // Apply smoothing update
    for (int i = 0; i < result.rows(); ++i) {
      if (preserve_boundaries_ && boundary_vertices[i]) {
        continue; // Skip boundary vertices
      }
      result.row(i) += lambda_ * laplacian_coords.row(i);
    }
  }

  return result;
}

Eigen::MatrixXd
LaplacianSOP::apply_taubin_smoothing(const Eigen::MatrixXd &vertices,
                                     const Eigen::MatrixXi &faces) {
  Eigen::MatrixXd result = vertices;
  auto boundary_vertices = find_boundary_vertices(faces, vertices.rows());

  for (int iter = 0; iter < iterations_; ++iter) {
    // Lambda step (smoothing)
    auto laplacian = build_uniform_laplacian(faces, result.rows());
    Eigen::MatrixXd laplacian_coords = laplacian * result;

    for (int i = 0; i < result.rows(); ++i) {
      if (preserve_boundaries_ && boundary_vertices[i]) {
        continue;
      }
      result.row(i) += lambda_ * laplacian_coords.row(i);
    }

    // Mu step (anti-smoothing to prevent shrinkage)
    laplacian_coords = laplacian * result;
    for (int i = 0; i < result.rows(); ++i) {
      if (preserve_boundaries_ && boundary_vertices[i]) {
        continue;
      }
      result.row(i) += mu_ * laplacian_coords.row(i);
    }
  }

  return result;
}

Eigen::SparseMatrix<double>
LaplacianSOP::build_uniform_laplacian(const Eigen::MatrixXi &faces,
                                      int num_vertices) {
  Eigen::SparseMatrix<double> laplacian(num_vertices, num_vertices);

  // Count vertex degrees (number of connected vertices)
  std::vector<int> degrees(num_vertices, 0);
  std::vector<std::unordered_set<int>> adjacency(num_vertices);

  // Build adjacency information
  for (int f = 0; f < faces.rows(); ++f) {
    for (int i = 0; i < 3; ++i) {
      int v1 = faces(f, i);
      int v2 = faces(f, (i + 1) % 3);

      adjacency[v1].insert(v2);
      adjacency[v2].insert(v1);
    }
  }

  // Build Laplacian matrix
  std::vector<Eigen::Triplet<double>> triplets;

  for (int i = 0; i < num_vertices; ++i) {
    int degree = adjacency[i].size();
    if (degree > 0) {
      // Diagonal entry
      triplets.emplace_back(i, i, -1.0);

      // Off-diagonal entries
      double weight = 1.0 / static_cast<double>(degree);
      for (int neighbor : adjacency[i]) {
        triplets.emplace_back(i, neighbor, weight);
      }
    }
  }

  laplacian.setFromTriplets(triplets.begin(), triplets.end());
  return laplacian;
}

Eigen::SparseMatrix<double>
LaplacianSOP::build_cotangent_laplacian(const Eigen::MatrixXd &vertices,
                                        const Eigen::MatrixXi &faces) {
  Eigen::SparseMatrix<double> laplacian(vertices.rows(), vertices.rows());

  // Build edge-to-faces mapping for cotangent weight calculation
  std::map<std::pair<int, int>, std::vector<int>> edge_to_faces;

  for (int f = 0; f < faces.rows(); ++f) {
    for (int i = 0; i < 3; ++i) {
      int v1 = faces(f, i);
      int v2 = faces(f, (i + 1) % 3);

      auto edge = std::make_pair(std::min(v1, v2), std::max(v1, v2));
      edge_to_faces[edge].push_back(f);
    }
  }

  // Calculate cotangent weights
  std::vector<Eigen::Triplet<double>> triplets;
  std::vector<double> vertex_weights(vertices.rows(), 0.0);

  for (const auto &[edge, face_list] : edge_to_faces) {
    int v1 = edge.first;
    int v2 = edge.second;

    double total_weight = 0.0;

    // Sum cotangent weights from adjacent faces
    for (int face_idx : face_list) {
      // Find the third vertex of the triangle
      int v3 = -1;
      for (int i = 0; i < 3; ++i) {
        int v = faces(face_idx, i);
        if (v != v1 && v != v2) {
          v3 = v;
          break;
        }
      }

      if (v3 != -1) {
        double weight = calculate_cotangent_weight(
            vertices.row(v1), vertices.row(v2), vertices.row(v3));
        total_weight += weight;
      }
    }

    // Add to sparse matrix
    if (total_weight > 1e-12) {
      triplets.emplace_back(v1, v2, total_weight);
      triplets.emplace_back(v2, v1, total_weight);
      vertex_weights[v1] += total_weight;
      vertex_weights[v2] += total_weight;
    }
  }

  // Add diagonal entries
  for (int i = 0; i < vertices.rows(); ++i) {
    if (vertex_weights[i] > 1e-12) {
      triplets.emplace_back(i, i, -vertex_weights[i]);
    }
  }

  laplacian.setFromTriplets(triplets.begin(), triplets.end());
  return laplacian;
}

std::vector<bool>
LaplacianSOP::find_boundary_vertices(const Eigen::MatrixXi &faces,
                                     int num_vertices) {
  std::vector<bool> is_boundary(num_vertices, false);
  std::map<std::pair<int, int>, int> edge_count;

  // Count edge occurrences
  for (int f = 0; f < faces.rows(); ++f) {
    for (int i = 0; i < 3; ++i) {
      int v1 = faces(f, i);
      int v2 = faces(f, (i + 1) % 3);

      auto edge = std::make_pair(std::min(v1, v2), std::max(v1, v2));
      edge_count[edge]++;
    }
  }

  // Mark vertices on boundary edges (edges with count == 1)
  for (const auto &[edge, count] : edge_count) {
    if (count == 1) {
      is_boundary[edge.first] = true;
      is_boundary[edge.second] = true;
    }
  }

  return is_boundary;
}

double
LaplacianSOP::calculate_cotangent_weight(const Eigen::Vector3d &v1,
                                         const Eigen::Vector3d &v2,
                                         const Eigen::Vector3d &opposite) {
  const Eigen::Vector3d e1 = v1 - opposite;
  const Eigen::Vector3d e2 = v2 - opposite;

  const double dot_product = e1.dot(e2);
  const double cross_magnitude = e1.cross(e2).norm();

  if (cross_magnitude < 1e-12) {
    return 0.0; // Degenerate triangle
  }

  return dot_product / cross_magnitude; // cotangent = cos/sin
}

bool LaplacianSOP::validate_parameters() const {
  if (!input_mesh_) {
    std::cerr << "LaplacianSOP: No input mesh provided\n";
    return false;
  }

  if (input_mesh_->vertices().rows() == 0) {
    std::cerr << "LaplacianSOP: Input mesh has no vertices\n";
    return false;
  }

  if (iterations_ < 0) {
    std::cerr << "LaplacianSOP: Invalid iteration count\n";
    return false;
  }

  return true;
}

bool LaplacianSOP::needs_recalculation() const {
  if (!input_mesh_)
    return true;

  std::hash<size_t> hasher;
  size_t current_hash =
      hasher(input_mesh_->vertices().rows() * input_mesh_->faces().rows());

  return current_hash != last_input_hash_;
}

} // namespace nodeflux::sop
