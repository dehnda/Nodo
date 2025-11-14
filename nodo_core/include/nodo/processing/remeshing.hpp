#pragma once

#include "../core/geometry_container.hpp"

#include <optional>
#include <string>

namespace nodo::processing {

/**
 * @brief Parameters for adaptive remeshing
 */
struct RemeshingParams {
  // Target edge length for uniform remeshing
  float target_edge_length = 1.0F;

  // Number of iterations
  int iterations = 10;

  // Use adaptive remeshing (adjust to curvature)
  bool use_adaptive = false;

  // Minimum edge length for adaptive remeshing
  float min_edge_length = 0.1F;

  // Maximum edge length for adaptive remeshing
  float max_edge_length = 2.0F;

  // Approximation error for adaptive remeshing
  float approx_error = 0.01F;

  // Number of smoothing iterations
  int smoothing_iterations = 10;

  // Preserve boundaries
  bool preserve_boundaries = true;
};

/**
 * @brief Mesh remeshing using PMP library
 */
class Remeshing {
public:
  /**
   * @brief Remesh geometry
   * @param container Input geometry
   * @param params Remeshing parameters
   * @param error Optional error message output
   * @return Remeshed geometry, or nullopt on failure
   */
  static std::optional<core::GeometryContainer>
  remesh(const core::GeometryContainer& container,
         const RemeshingParams& params, std::string* error = nullptr);
};

} // namespace nodo::processing
