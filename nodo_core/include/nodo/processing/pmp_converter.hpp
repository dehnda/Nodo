#pragma once

#include "nodo/core/geometry_container.hpp"
#include "nodo/core/mesh.hpp"

#include <optional>

#include <pmp/surface_mesh.h>

namespace nodo::processing::detail {

/**
 * @brief Converter between Nodo and PMP data structures
 *
 * Handles bidirectional conversion while preserving:
 * - Vertex positions
 * - Face topology
 * - Normals (when available)
 * - UVs (when available)
 */
class PMPConverter {
public:
  // ====================================================================
  // Nodo → PMP Conversions
  // ====================================================================

  /**
   * @brief Convert Nodo Mesh to PMP SurfaceMesh
   * @param mesh Input Nodo mesh
   * @return PMP surface mesh
   * @throws std::runtime_error if conversion fails
   */
  static pmp::SurfaceMesh to_pmp(const core::Mesh& mesh);

  /**
   * @brief Convert Nodo GeometryContainer to PMP SurfaceMesh
   * @param container Input geometry container
   * @return PMP surface mesh
   * @throws std::runtime_error if conversion fails
   */
  static pmp::SurfaceMesh to_pmp(const core::GeometryContainer& container);

  // ====================================================================
  // PMP → Nodo Conversions
  // ====================================================================

  /**
   * @brief Convert PMP SurfaceMesh back to Nodo Mesh
   * @param pmp_mesh Input PMP mesh
   * @return Nodo mesh
   */
  static core::Mesh from_pmp(const pmp::SurfaceMesh& pmp_mesh);

  /**
   * @brief Convert PMP SurfaceMesh back to Nodo GeometryContainer
   * @param pmp_mesh Input PMP mesh
   * @param preserve_attributes Try to preserve custom attributes
   * @return Geometry container with P, N attributes
   */
  static core::GeometryContainer from_pmp_container(const pmp::SurfaceMesh& pmp_mesh, bool preserve_attributes = true);

  // ====================================================================
  // Validation
  // ====================================================================

  /**
   * @brief Validate mesh is suitable for PMP processing
   * @param mesh Mesh to validate
   * @return Error message if invalid, empty string if valid
   */
  static std::string validate_for_pmp(const core::Mesh& mesh);

  /**
   * @brief Validate geometry container for PMP processing
   * @param container Container to validate
   * @return Error message if invalid, empty string if valid
   */
  static std::string validate_for_pmp(const core::GeometryContainer& container);

private:
  // Helper: Extract positions from GeometryContainer
  static std::vector<Eigen::Vector3d> extract_positions(const core::GeometryContainer& container);

  // Helper: Extract face indices from GeometryContainer
  static std::vector<std::array<int, 3>> extract_faces(const core::GeometryContainer& container);

  // Helper: Compute normals if not present
  static void compute_normals(pmp::SurfaceMesh& mesh);
};

} // namespace nodo::processing::detail
