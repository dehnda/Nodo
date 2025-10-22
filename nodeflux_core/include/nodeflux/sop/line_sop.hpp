#pragma once

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/sop/geometry_data.hpp"
#include "nodeflux/sop/sop_node.hpp"
#include <array>
#include <memory>
#include <string>

namespace nodeflux::sop {

/**
 * @brief Line SOP - Generates a line between two points
 *
 * Creates a simple line geometry with a specified number of segments.
 * Useful for creating curves, paths, and guide lines for other operations.
 */
class LineSOP : public SOPNode {
public:
  explicit LineSOP(const std::string &name = "line");

  /**
   * @brief Set the start point of the line
   */
  void set_start_point(float x, float y, float z) {
    std::array<float, 3> new_point = {x, y, z};
    if (start_point_ != new_point) {
      start_point_ = new_point;
      mark_dirty();
    }
  }

  void set_start_point(const std::array<float, 3> &point) {
    if (start_point_ != point) {
      start_point_ = point;
      mark_dirty();
    }
  }

  /**
   * @brief Set the end point of the line
   */
  void set_end_point(float x, float y, float z) {
    std::array<float, 3> new_point = {x, y, z};
    if (end_point_ != new_point) {
      end_point_ = new_point;
      mark_dirty();
    }
  }

  void set_end_point(const std::array<float, 3> &point) {
    if (end_point_ != point) {
      end_point_ = point;
      mark_dirty();
    }
  }

  /**
   * @brief Set the number of segments (number of edges)
   * @param segments Number of segments (minimum 1)
   */
  void set_segments(int segments) {
    if (segments_ != segments) {
      segments_ = segments;
      mark_dirty();
    }
  }

  /**
   * @brief Get current parameters
   */
  std::array<float, 3> get_start_point() const { return start_point_; }
  std::array<float, 3> get_end_point() const { return end_point_; }
  int get_segments() const { return segments_; }

protected:
  /**
   * @brief Execute the line generation (SOPNode override)
   */
  std::shared_ptr<GeometryData> execute() override;

private:
  std::array<float, 3> start_point_ = {0.0F, 0.0F, 0.0F};
  std::array<float, 3> end_point_ = {1.0F, 0.0F, 0.0F};
  int segments_ = 10;
};

} // namespace nodeflux::sop
