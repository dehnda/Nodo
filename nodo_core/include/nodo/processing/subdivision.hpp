#pragma once

#include "nodo/core/geometry_container.hpp"

#include <optional>
#include <string>

namespace nodo::processing {

/**
 * @brief Subdivision algorithm types
 */
enum class SubdivisionType {
  CATMULL_CLARK, ///< Catmull-Clark subdivision (quads/mixed)
  LOOP,          ///< Loop subdivision (triangles)
  QUAD_TRI       ///< Quad-Tri subdivision (mixed)
};

/**
 * @brief Parameters for subdivision operations
 */
struct SubdivisionParams {
  /// Type of subdivision algorithm
  SubdivisionType type = SubdivisionType::CATMULL_CLARK;

  /// Number of subdivision levels (1-5 typical)
  unsigned int levels = 1;
};

/**
 * @brief Mesh subdivision using PMP library
 */
class Subdivision {
public:
  /**
   * @brief Subdivide a mesh
   *
   * @param container Input geometry
   * @param params Subdivision parameters
   * @param error Optional error message output
   * @return Subdivided geometry or nullopt on failure
   */
  static std::optional<core::GeometryContainer>
  subdivide(const core::GeometryContainer& container,
            const SubdivisionParams& params, std::string* error = nullptr);
};

} // namespace nodo::processing
