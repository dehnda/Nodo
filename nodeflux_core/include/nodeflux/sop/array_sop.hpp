#pragma once

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/sop/geometry_data.hpp"
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
  std::shared_ptr<GeometryData> execute() override;

private:
  std::optional<core::Mesh> create_linear_array(const core::Mesh &input_mesh,
                                                int count);
  std::optional<core::Mesh> create_radial_array(const core::Mesh &input_mesh,
                                                int count);
  std::optional<core::Mesh> create_grid_array(const core::Mesh &input_mesh,
                                              int grid_width, int grid_height);

  void add_instance_attributes(std::shared_ptr<GeometryData> geo_data,
                               size_t verts_per_instance,
                               size_t faces_per_instance, int instance_count,
                               ArrayType array_type);
};

} // namespace nodeflux::sop
