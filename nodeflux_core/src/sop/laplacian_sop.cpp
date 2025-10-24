#include "nodeflux/sop/laplacian_sop.hpp"
#include "nodeflux/core/types.hpp"
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace nodeflux::sop {

LaplacianSOP::LaplacianSOP(const std::string &name)
    : SOPNode(name, "Laplacian") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(
      define_int_parameter("iterations", 5)
          .label("Iterations")
          .range(1, 100)
          .category("Smoothing")
          .build());

  register_parameter(
      define_float_parameter("lambda", 0.5F)
          .label("Lambda")
          .range(0.0, 1.0)
          .category("Smoothing")
          .build());

  register_parameter(
      define_int_parameter("method", 0)
          .label("Method")
          .options({"Uniform", "Cotangent", "Taubin"})
          .category("Smoothing")
          .build());
}

std::shared_ptr<GeometryData> LaplacianSOP::execute() {
  // Sync member variables from parameter system
  iterations_ = get_parameter<int>("iterations", 5);
  lambda_ = get_parameter<float>("lambda", 0.5F);
  method_ = static_cast<SmoothingMethod>(get_parameter<int>("method", 0));

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

  if (input_mesh->vertices().rows() == 0) {
    set_error("Input mesh has no vertices");
    return nullptr;
  }

  if (iterations_ < 0) {
    set_error("Iterations must be non-negative");
    return nullptr;
  }

  const auto &original_vertices = input_mesh->vertices();
  const auto &faces = input_mesh->faces();

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

  auto result_mesh = std::make_shared<core::Mesh>(smoothed_vertices, faces);
  return std::make_shared<GeometryData>(result_mesh);
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

} // namespace nodeflux::sop
