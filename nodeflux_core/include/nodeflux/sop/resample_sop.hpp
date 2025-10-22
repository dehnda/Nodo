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
    BY_COUNT, ///< Resample to exact point count
    BY_LENGTH ///< Resample with target segment length
  };

  explicit ResampleSOP(const std::string &name = "resample");

  /**
   * @brief Set resampling mode
   */
  void set_mode(Mode mode) {
    if (mode_ != mode) {
      mode_ = mode;
      mark_dirty();
    }
  }

  /**
   * @brief Set target point count (for BY_COUNT mode)
   * @param count Number of points (minimum 2)
   */
  void set_point_count(int count) {
    int clamped = std::max(2, count);
    if (point_count_ != clamped) {
      point_count_ = clamped;
      mark_dirty();
    }
  }

  /**
   * @brief Set target segment length (for BY_LENGTH mode)
   * @param length Target length of each segment
   */
  void set_segment_length(float length) {
    if (segment_length_ != length) {
      segment_length_ = length;
      mark_dirty();
    }
  }

  /**
   * @brief Get current parameters
   */
  Mode get_mode() const { return mode_; }
  int get_point_count() const { return point_count_; }
  float get_segment_length() const { return segment_length_; }

protected:
  /**
   * @brief Execute the resample operation (SOPNode override)
   */
  std::shared_ptr<GeometryData> execute() override;

private:
  Mode mode_ = Mode::BY_COUNT;
  int point_count_ = 10;
  float segment_length_ = 0.1F;
};

} // namespace nodeflux::sop
