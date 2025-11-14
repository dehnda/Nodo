#pragma once

#include "nodo/core/geometry_container.hpp"

#include <optional>

namespace nodo::processing {

/**
 * @brief Parameters for mesh decimation
 */
struct DecimationParams {
  /// Target number of vertices (if use_vertex_count is true)
  int target_vertex_count = 1000;

  /// Target percentage of original vertices (if use_vertex_count is false)
  /// Range: 0.0 to 1.0
  float target_percentage = 0.5f;

  /// Use target_vertex_count instead of target_percentage
  bool use_vertex_count = false;

  /// Aspect ratio for decimation quality control
  /// Higher values preserve shape better but reduce less
  /// Range: typically 1.0 to 10.0
  float aspect_ratio = 0.0f;

  /// Edge length threshold for decimation
  /// Edges shorter than this will be preserved
  float edge_length = 0.0f;

  /// Maximum number of iterations
  int max_valence = 0;

  /// Preserve topology (no holes created)
  bool preserve_topology = true;

  /// Preserve boundaries
  bool preserve_boundaries = true;
};

/**
 * @brief Mesh decimation using PMP library
 *
 * Reduces mesh complexity while preserving overall shape.
 * Uses error quadrics method from PMP library.
 */
class Decimation {
public:
  /**
   * @brief Decimate a mesh to reduce vertex/face count
   * @param input Input geometry (must be triangular mesh)
   * @param params Decimation parameters
   * @return Decimated geometry, or std::nullopt on error
   */
  static std::optional<core::GeometryContainer>
  decimate(const core::GeometryContainer& input,
           const DecimationParams& params);

  /**
   * @brief Get last error message
   */
  static const std::string& get_last_error();

private:
  static void set_error(std::string error);
  static std::string last_error_;
};

} // namespace nodo::processing
