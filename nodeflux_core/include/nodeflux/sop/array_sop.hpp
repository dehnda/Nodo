#pragma once

#include "nodeflux/core/geometry_container.hpp"
#include "nodeflux/sop/sop_node.hpp"

#include <optional>
#include <string>

namespace nodeflux::sop {

/**
 * @brief Array SOP - Duplicates geometry in various patterns
 *
 * Creates multiple copies of input geometry arranged in linear, radial, or grid
 * patterns. This is a simplified version that works with the existing mesh
 * system.
 */
class ArraySOP : public SOPNode {
public:
  enum class ArrayType {
    LINEAR = 0, ///< Copies along a line with specified offset
    RADIAL = 1, ///< Copies around a center point with rotation
    GRID = 2    ///< Copies in a 2D grid pattern
  };

  explicit ArraySOP(const std::string &name = "array");

  /**
   * @brief Execute the array operation (SOPNode override)
   */
  std::shared_ptr<core::GeometryContainer> execute() override;

private:
  std::unique_ptr<core::GeometryContainer>
  create_linear_array(const core::GeometryContainer &input_geo, int count);
  std::unique_ptr<core::GeometryContainer>
  create_radial_array(const core::GeometryContainer &input_geo, int count);
  std::unique_ptr<core::GeometryContainer>
  create_grid_array(const core::GeometryContainer &input_geo, int grid_width,
                    int grid_height);
};

} // namespace nodeflux::sop
