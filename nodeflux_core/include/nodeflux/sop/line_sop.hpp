#pragma once

#include "nodeflux/core/mesh.hpp"
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
class LineSOP {
public:
  explicit LineSOP(std::string name);

  /**
   * @brief Set the start point of the line
   */
  void set_start_point(float x, float y, float z);
  void set_start_point(const std::array<float, 3> &point);

  /**
   * @brief Set the end point of the line
   */
  void set_end_point(float x, float y, float z);
  void set_end_point(const std::array<float, 3> &point);

  /**
   * @brief Set the number of segments (number of edges)
   * @param segments Number of segments (minimum 1)
   */
  void set_segments(int segments);

  /**
   * @brief Get current parameters
   */
  std::array<float, 3> get_start_point() const { return start_point_; }
  std::array<float, 3> get_end_point() const { return end_point_; }
  int get_segments() const { return segments_; }

  /**
   * @brief Execute the line generation
   * @return Generated line mesh or empty optional on failure
   */
  std::optional<core::Mesh> execute();

  /**
   * @brief Get cached result or compute if dirty
   * @return Result mesh or nullptr on failure
   */
  std::shared_ptr<core::Mesh> cook();

  /**
   * @brief Mark node as needing recomputation
   */
  void mark_dirty() { is_dirty_ = true; }

private:
  std::string name_;
  std::array<float, 3> start_point_ = {0.0F, 0.0F, 0.0F};
  std::array<float, 3> end_point_ = {1.0F, 0.0F, 0.0F};
  int segments_ = 10;

  bool is_dirty_ = true;
  std::shared_ptr<core::Mesh> cached_result_;
};

} // namespace nodeflux::sop
