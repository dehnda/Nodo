#pragma once

#include "nodo/core/geometry_container.hpp"
#include <optional>
#include <string>

namespace nodo::processing {

/**
 * @brief Parameters for mesh smoothing operations
 */
struct SmoothingParams {
  /// Smoothing method: false = explicit (fast), true = implicit (quality)
  bool use_implicit = false;

  /// Number of smoothing iterations
  unsigned int iterations = 10;

  /// Use uniform Laplacian (true) or cotangent Laplacian (false)
  bool use_uniform_laplace = false;

  /// Timestep for implicit smoothing (smaller = more stable)
  float timestep = 0.001f;

  /// Re-center and re-scale after implicit smoothing
  bool rescale = true;
};

/**
 * @brief Mesh smoothing using PMP library
 */
class Smoothing {
public:
  /**
   * @brief Smooth a mesh using Laplacian smoothing
   *
   * @param container Input geometry
   * @param params Smoothing parameters
   * @param error Optional error message output
   * @return Smoothed geometry or nullopt on failure
   */
  static std::optional<core::GeometryContainer>
  smooth(const core::GeometryContainer &container,
         const SmoothingParams &params, std::string *error = nullptr);
};

} // namespace nodo::processing
