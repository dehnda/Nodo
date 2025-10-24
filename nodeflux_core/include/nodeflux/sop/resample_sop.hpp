#pragma once

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/sop/geometry_data.hpp"
#include "nodeflux/sop/sop_node.hpp"
#include <memory>
#include <string>

namespace nodeflux::sop {

/**
 * @brief Resample SOP - Resamples curve/line geometry with uniform spacing
 *
 * Takes input geometry (line/curve) and resamples it with a specified number
 * of points or target segment length. Useful for controlling point density
 * on curves before further operations.
 */
class ResampleSOP : public SOPNode {
public:
  enum class Mode {
    BY_COUNT = 0, ///< Resample to exact point count
    BY_LENGTH = 1 ///< Resample with target segment length
  };

  explicit ResampleSOP(const std::string &name = "resample");

protected:
  /**
   * @brief Execute the resample operation (SOPNode override)
   */
  std::shared_ptr<GeometryData> execute() override;
};

} // namespace nodeflux::sop
