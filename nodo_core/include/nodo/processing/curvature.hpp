#pragma once

#include "nodo/core/geometry_container.hpp"

#include <optional>

namespace nodo::processing {

/// Curvature type to compute
enum class CurvatureType {
  MEAN,     ///< Mean curvature (H = (k1 + k2) / 2)
  GAUSSIAN, ///< Gaussian curvature (K = k1 * k2)
  MIN,      ///< Minimum principal curvature (k1)
  MAX,      ///< Maximum principal curvature (k2)
  ALL       ///< Compute all curvature types
};

/// Parameters for curvature computation
struct CurvatureParams {
  /// Type of curvature to compute
  CurvatureType type = CurvatureType::MEAN;

  /// Whether to use absolute values (easier to visualize)
  bool use_absolute = false;

  /// Whether to smooth curvature values
  bool smooth = true;

  /// Number of smoothing iterations (if smooth = true)
  int smoothing_iterations = 2;
};

/// Curvature analysis using PMP library
class Curvature {
public:
  /// Compute curvature on a geometry container
  /// Adds point attributes: "mean_curvature", "gaussian_curvature",
  /// "min_curvature", "max_curvature"
  /// @param input Input geometry
  /// @param params Curvature computation parameters
  /// @return Geometry with curvature attributes, or nullopt on error
  static std::optional<core::GeometryContainer> compute(const core::GeometryContainer& input,
                                                        const CurvatureParams& params = CurvatureParams{});
};

} // namespace nodo::processing
