#pragma once

#include "nodo/core/mesh.hpp"
#include "nodo/sop/sop_node.hpp"
#include <memory>
#include <string>

namespace nodo::sop {

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
  std::shared_ptr<core::GeometryContainer> execute() override;
};

} // namespace nodo::sop
