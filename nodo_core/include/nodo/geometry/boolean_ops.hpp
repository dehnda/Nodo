#pragma once

#include "nodo/core/error.hpp"
#include "nodo/core/geometry_container.hpp"

#include <memory>
#include <optional>

// Forward declarations
namespace manifold {
class Manifold;
} // namespace manifold

namespace nodo::geometry {

/**
 * @brief Clean, modern interface for boolean operations using Manifold
 *
 * This class provides boolean mesh operations using the Manifold library
 * with simple C++20 patterns. Manifold is licensed under Apache 2.0, making
 * it fully commercial-friendly with no restrictions.
 * Operations return std::optional for success/failure, with separate error
 * reporting.
 *
 * @note Manifold provides robust, high-performance boolean operations
 * @see https://github.com/elalish/manifold
 */
class BooleanOps {
public:
  // ============================================================================
  // NEW API: GeometryContainer-based operations (preferred)
  // ============================================================================

  /**
   * @brief Perform union of two geometries
   * @param a First geometry
   * @param b Second geometry
   * @return Optional geometry containing the union, or nullopt on failure
   */
  static std::optional<core::GeometryContainer> union_geometries(const core::GeometryContainer& a,
                                                                 const core::GeometryContainer& b);

  /**
   * @brief Perform intersection of two geometries
   * @param a First geometry
   * @param b Second geometry
   * @return Optional geometry containing the intersection, or nullopt on failure
   */
  static std::optional<core::GeometryContainer> intersect_geometries(const core::GeometryContainer& a,
                                                                     const core::GeometryContainer& b);

  /**
   * @brief Perform difference of two geometries (a - b)
   * @param a First geometry (minuend)
   * @param b Second geometry (subtrahend)
   * @return Optional geometry containing the difference, or nullopt on failure
   */
  static std::optional<core::GeometryContainer> difference_geometries(const core::GeometryContainer& a,
                                                                      const core::GeometryContainer& b);

  /**
   * @brief Get the last error that occurred
   * @return Error information for the last failed operation
   */
  static const core::Error& last_error();

private:
  // ============================================================================
  // Direct conversion helpers (GeometryContainer ↔ Eigen)
  // ============================================================================

  /// Extract Eigen matrices directly from GeometryContainer
  static std::pair<Eigen::MatrixXd, Eigen::MatrixXi> extract_eigen_mesh(const core::GeometryContainer& container);

  /// Build GeometryContainer directly from Eigen matrices
  static core::GeometryContainer build_container_from_eigen(const Eigen::MatrixXd& vertices,
                                                            const Eigen::MatrixXi& faces);

  // ============================================================================
  // Manifold operations
  // ============================================================================

  /// Direct boolean operation working with Eigen matrices
  static std::pair<Eigen::MatrixXd, Eigen::MatrixXi>
  manifold_boolean_operation_direct(const Eigen::MatrixXd& vertices_a, const Eigen::MatrixXi& faces_a,
                                    const Eigen::MatrixXd& vertices_b, const Eigen::MatrixXi& faces_b,
                                    int operation_type);

  /// Convert Eigen matrices to Manifold format
  static manifold::Manifold eigen_to_manifold_direct(const Eigen::MatrixXd& vertices, const Eigen::MatrixXi& faces);

  /// Convert Manifold to Eigen matrices
  static std::pair<Eigen::MatrixXd, Eigen::MatrixXi> manifold_to_eigen_direct(const manifold::Manifold& manifold);

  /// Perform the Manifold boolean operation
  static manifold::Manifold perform_manifold_operation(const manifold::Manifold& mesh_a,
                                                       const manifold::Manifold& mesh_b, int operation_type);

  /// Set the last error for error reporting
  static void set_last_error(const core::Error& error);

  /// Thread-local storage for last error
  static thread_local core::Error last_error_;
};

} // namespace nodo::geometry
