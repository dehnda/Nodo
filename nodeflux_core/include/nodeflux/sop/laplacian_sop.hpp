#pragma once

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/sop/geometry_data.hpp"
#include "nodeflux/sop/sop_node.hpp"
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <memory>
#include <string>

namespace nodeflux::sop {

/**
 * @brief Laplacian mesh smoothing operator
 *
 * LaplacianSOP applies Laplacian smoothing to reduce mesh noise and
 * create smoother surfaces while preserving overall shape.
 */
class LaplacianSOP : public SOPNode {
public:
  /// Smoothing algorithm variants
  enum class SmoothingMethod {
    UNIFORM,   ///< Uniform Laplacian (simple average)
    COTANGENT, ///< Cotangent-weighted Laplacian (angle-aware)
    TAUBIN     ///< Taubin smoothing (prevents shrinkage)
  };

  /**
   * @brief Construct Laplacian smoother with name
   * @param name Unique identifier for this SOP instance
   */
  explicit LaplacianSOP(const std::string &name = "laplacian");

  /**
   * @brief Set number of smoothing iterations
   * @param iterations Number of smoothing passes (default: 1)
   */
  void set_iterations(int iterations) {
    if (iterations_ != iterations) {
      iterations_ = iterations;
      mark_dirty();
    }
  }

  /**
   * @brief Get number of iterations
   */
  int get_iterations() const { return iterations_; }

  /**
   * @brief Set smoothing strength factor
   * @param lambda Smoothing factor (0.0 = no change, 1.0 = full smoothing)
   */
  void set_lambda(double lambda) {
    if (lambda_ != lambda) {
      lambda_ = lambda;
      mark_dirty();
    }
  }

  /**
   * @brief Get smoothing strength
   */
  double get_lambda() const { return lambda_; }

  /**
   * @brief Set smoothing method
   * @param method Algorithm variant to use
   */
  void set_method(SmoothingMethod method) {
    if (method_ != method) {
      method_ = method;
      mark_dirty();
    }
  }

  /**
   * @brief Get smoothing method
   */
  SmoothingMethod get_method() const { return method_; }

  /**
   * @brief Set shrinkage prevention factor (for Taubin method)
   * @param mu Negative smoothing factor to prevent volume loss
   */
  void set_mu(double mu) {
    if (mu_ != mu) {
      mu_ = mu;
      mark_dirty();
    }
  }

  /**
   * @brief Get shrinkage prevention factor
   */
  double get_mu() const { return mu_; }

  /**
   * @brief Set boundary vertex handling
   * @param preserve If true, boundary vertices remain fixed
   */
  void set_preserve_boundaries(bool preserve) {
    if (preserve_boundaries_ != preserve) {
      preserve_boundaries_ = preserve;
      mark_dirty();
    }
  }

  /**
   * @brief Get boundary preservation setting
   */
  bool get_preserve_boundaries() const { return preserve_boundaries_; }

protected:
  /**
   * @brief Execute the smoothing operation (SOPNode override)
   */
  std::shared_ptr<GeometryData> execute() override;

private:
  /// Apply uniform Laplacian smoothing
  Eigen::MatrixXd apply_uniform_laplacian(const Eigen::MatrixXd &vertices,
                                          const Eigen::MatrixXi &faces);

  /// Apply cotangent-weighted Laplacian smoothing
  Eigen::MatrixXd apply_cotangent_laplacian(const Eigen::MatrixXd &vertices,
                                            const Eigen::MatrixXi &faces);

  /// Apply Taubin smoothing (lambda-mu scheme)
  Eigen::MatrixXd apply_taubin_smoothing(const Eigen::MatrixXd &vertices,
                                         const Eigen::MatrixXi &faces);

  /// Build uniform Laplacian matrix
  Eigen::SparseMatrix<double> build_uniform_laplacian(const Eigen::MatrixXi &faces,
                                                      int num_vertices);

  /// Build cotangent-weighted Laplacian matrix
  Eigen::SparseMatrix<double> build_cotangent_laplacian(const Eigen::MatrixXd &vertices,
                                                        const Eigen::MatrixXi &faces);

  /// Find boundary vertices
  std::vector<bool> find_boundary_vertices(const Eigen::MatrixXi &faces,
                                           int num_vertices);

  /// Calculate cotangent weights for an edge
  double calculate_cotangent_weight(const Eigen::Vector3d &v1,
                                    const Eigen::Vector3d &v2,
                                    const Eigen::Vector3d &opposite);

  // Smoothing parameters
  int iterations_ = 1;                         ///< Number of smoothing passes
  double lambda_ = 0.5;                        ///< Smoothing strength
  double mu_ = -0.53;                          ///< Shrinkage prevention
  SmoothingMethod method_ = SmoothingMethod::UNIFORM; ///< Algorithm
  bool preserve_boundaries_ = true;            ///< Keep boundary fixed
};

} // namespace nodeflux::sop
