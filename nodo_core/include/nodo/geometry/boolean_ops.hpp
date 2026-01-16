#pragma once

#include "nodo/core/error.hpp"
#include "nodo/core/geometry_container.hpp"
#include "nodo/core/mesh.hpp"

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

  // ============================================================================
  // LEGACY API: Mesh-based operations (kept for backward compatibility)
  // ============================================================================

  /**
   * @brief Perform union of two meshes
   * @param a First mesh
   * @param b Second mesh
   * @return Optional mesh containing the union, or nullopt on failure
   * @deprecated Use union_geometries() instead
   */
  static std::optional<core::Mesh> union_meshes(const core::Mesh& a, const core::Mesh& b);

  /**
   * @brief Perform intersection of two meshes
   * @param a First mesh
   * @param b Second mesh
   * @return Optional mesh containing the intersection, or nullopt on failure
   * @deprecated Use intersect_geometries() instead
   */
  static std::optional<core::Mesh> intersect_meshes(const core::Mesh& a, const core::Mesh& b);

  /**
   * @brief Perform difference of two meshes (a - b)
   * @param a First mesh (minuend)
   * @param b Second mesh (subtrahend)
   * @return Optional mesh containing the difference, or nullopt on failure
   * @deprecated Use difference_geometries() instead
   */
  static std::optional<core::Mesh> difference_meshes(const core::Mesh& a, const core::Mesh& b);

  /**
   * @brief Get the last error that occurred
   * @return Error information for the last failed operation
   */
  static const core::Error& last_error();

  /**
   * @brief Check if meshes are compatible for boolean operations
   * @param a First mesh
   * @param b Second mesh
   * @return true if meshes can be used for boolean operations
   */
  static bool are_compatible(const core::Mesh& a, const core::Mesh& b) noexcept;

  /**
   * @brief Validate a single mesh for boolean operations
   * @param mesh Mesh to validate
   * @return true if mesh is valid for boolean operations
   */
  static bool validate_mesh(const core::Mesh& mesh);

private:
  // ============================================================================
  // Direct conversion helpers (GeometryContainer ↔ Eigen, no Mesh intermediate)
  // ============================================================================

  /// Extract Eigen matrices directly from GeometryContainer (no Mesh intermediate)
  static std::pair<Eigen::MatrixXd, Eigen::MatrixXi> extract_eigen_mesh(const core::GeometryContainer& container);

  /// Build GeometryContainer directly from Eigen matrices (no Mesh intermediate)
  static core::GeometryContainer build_container_from_eigen(const Eigen::MatrixXd& vertices,
                                                            const Eigen::MatrixXi& faces);

  // ============================================================================
  // Direct Manifold operations (no Mesh intermediate)
  // ============================================================================

  /// Direct boolean operation working with Eigen matrices (no Mesh intermediate)
  static std::pair<Eigen::MatrixXd, Eigen::MatrixXi>
  manifold_boolean_operation_direct(const Eigen::MatrixXd& vertices_a, const Eigen::MatrixXi& faces_a,
                                    const Eigen::MatrixXd& vertices_b, const Eigen::MatrixXi& faces_b,
                                    int operation_type);

  /// Convert Eigen matrices directly to Manifold (no Mesh intermediate)
  static manifold::Manifold eigen_to_manifold_direct(const Eigen::MatrixXd& vertices, const Eigen::MatrixXi& faces);

  /// Convert Manifold directly to Eigen matrices (no Mesh intermediate)
  static std::pair<Eigen::MatrixXd, Eigen::MatrixXi> manifold_to_eigen_direct(const manifold::Manifold& manifold);

  /// Perform the Manifold boolean operation (shared by new and legacy APIs)
  static manifold::Manifold perform_manifold_operation(const manifold::Manifold& mesh_a,
                                                       const manifold::Manifold& mesh_b, int operation_type);

  // ============================================================================
  // Legacy helpers (for backward compatibility with Mesh API)
  // ============================================================================

  /// Internal Manifold boolean operation implementation (Apache 2.0 license)
  static std::optional<core::Mesh> manifold_boolean_operation(const core::Mesh& a, const core::Mesh& b,
                                                              int operation_type);

  /// Set the last error for error reporting
  static void set_last_error(const core::Error& error);

  /// Thread-local storage for last error
  static thread_local core::Error last_error_;
};

} // namespace nodo::geometry
