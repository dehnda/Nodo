#pragma once

#include "nodo/core/geometry_container.hpp"

#include <optional>
#include <string>

namespace nodo::processing {

/**
 * @brief Geodesic distance computation method
 */
enum class GeodesicMethod {
  Dijkstra = 0, ///< Dijkstra-like method (fast, approximate, can limit distance/count)
  Heat = 1      ///< Heat method (higher quality, slower, works on polygon meshes)
};

/**
 * @brief Parameters for geodesic distance computation
 */
struct GeodesicParams {
  /// Method to use for geodesic computation
  GeodesicMethod method = GeodesicMethod::Dijkstra;

  /// Name of point group containing seed vertices (empty = use all points)
  std::string seed_group = "";

  /// Maximum geodesic distance to compute (Dijkstra only, 0 = unlimited)
  float max_distance = 0.0F;

  /// Maximum number of neighbors to process (Dijkstra only, 0 = unlimited)
  unsigned int max_neighbors = 0;

  /// Name of the output distance attribute
  std::string output_attribute = "geodesic_distance";

  /**
   * @brief Create parameters for Dijkstra method
   */
  static GeodesicParams dijkstra(float max_dist = 0.0F, unsigned int max_num = 0) {
    GeodesicParams params;
    params.method = GeodesicMethod::Dijkstra;
    params.max_distance = max_dist;
    params.max_neighbors = max_num;
    return params;
  }

  /**
   * @brief Create parameters for Heat method
   */
  static GeodesicParams heat() {
    GeodesicParams params;
    params.method = GeodesicMethod::Heat;
    return params;
  }
};

/**
 * @brief Geodesic distance computation using PMP library
 *
 * Computes geodesic distances on mesh surfaces from seed vertices.
 * Two methods available:
 * - Dijkstra: Fast, approximate, can limit by distance/count
 * - Heat: Higher quality, works on general polygon meshes
 *
 * Output: Float point attribute with distance values from nearest seed
 */
class Geodesic {
public:
  /**
   * @brief Compute geodesic distances on geometry
   * @param input Input geometry (triangle mesh for Dijkstra, any for Heat)
   * @param params Geodesic computation parameters
   * @param error Optional error output string
   * @return Geometry with distance attribute or nullopt on error
   *
   * @note Dijkstra requires triangle mesh
   * @note Seed vertices selected from group or all points if no group
   * @note Distance values stored as float point attribute
   */
  static std::optional<core::GeometryContainer> compute(const core::GeometryContainer& input,
                                                        const GeodesicParams& params, std::string* error);
};

} // namespace nodo::processing
