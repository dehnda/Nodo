#pragma once

#include "nodo/core/geometry_container.hpp"

#include <optional>
#include <string>

namespace nodo::processing {

/**
 * @brief Parameterization method selection
 */
enum class ParameterizationMethod {
  Harmonic = 0, ///< Discrete harmonic parameterization (works on general
                ///< polygon meshes)
  LSCM = 1      ///< Least Squares Conformal Maps (triangle meshes only)
};

/**
 * @brief Parameters for mesh parameterization
 */
struct ParameterizationParams {
  /// Parameterization method to use
  ParameterizationMethod method = ParameterizationMethod::Harmonic;

  /// Use uniform weights for harmonic (vs. cotangent weights)
  bool use_uniform_weights = false;

  /// Name of the UV attribute to create (default: "uv")
  std::string uv_attribute_name = "uv";

  /**
   * @brief Create parameters for harmonic parameterization
   */
  static ParameterizationParams harmonic(bool uniform = false) {
    ParameterizationParams params;
    params.method = ParameterizationMethod::Harmonic;
    params.use_uniform_weights = uniform;
    return params;
  }

  /**
   * @brief Create parameters for LSCM parameterization
   */
  static ParameterizationParams lscm() {
    ParameterizationParams params;
    params.method = ParameterizationMethod::LSCM;
    return params;
  }
};

/**
 * @brief Mesh parameterization using PMP library
 *
 * Provides UV parameterization for meshes using different methods:
 * - Harmonic: Discrete harmonic parameterization, works on general polygon
 * meshes
 * - LSCM: Least Squares Conformal Maps, triangle meshes only
 *
 * Both methods require the input mesh to have at least one boundary.
 */
class Parameterization {
public:
  /**
   * @brief Compute UV parameterization for geometry
   * @param input Input geometry (must have boundary)
   * @param params Parameterization parameters
   * @param error Optional error output string
   * @return Geometry with UV attribute or nullopt on error
   *
   * @note Requires mesh with at least one boundary
   * @note LSCM requires triangle mesh
   * @note UV coordinates are stored as Vec2f point attribute
   */
  static std::optional<core::GeometryContainer> parameterize(const core::GeometryContainer& input,
                                                             const ParameterizationParams& params, std::string* error);
};

} // namespace nodo::processing
