#pragma once

#include "nodeflux/core/mesh.hpp"
#include <memory>
#include <optional>
#include <string>

namespace nodeflux::sop {

/**
 * @brief Resample SOP - Resamples curve/line geometry with uniform spacing
 *
 * Takes input geometry (line/curve) and resamples it with a specified number
 * of points or target segment length. Useful for controlling point density
 * on curves before further operations.
 */
class ResampleSOP {
public:
  enum class Mode {
    BY_COUNT, ///< Resample to exact point count
    BY_LENGTH ///< Resample with target segment length
  };

  explicit ResampleSOP(std::string name);

  /**
   * @brief Set resampling mode
   */
  void set_mode(Mode mode);

  /**
   * @brief Set target point count (for BY_COUNT mode)
   * @param count Number of points (minimum 2)
   */
  void set_point_count(int count);

  /**
   * @brief Set target segment length (for BY_LENGTH mode)
   * @param length Target length of each segment
   */
  void set_segment_length(float length);

  /**
   * @brief Set input mesh to resample
   */
  void set_input_mesh(std::shared_ptr<core::Mesh> mesh);

  /**
   * @brief Get current parameters
   */
  Mode get_mode() const { return mode_; }
  int get_point_count() const { return point_count_; }
  float get_segment_length() const { return segment_length_; }

  /**
   * @brief Execute the resampling operation
   * @return Resampled mesh or empty optional on failure
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
  Mode mode_ = Mode::BY_COUNT;
  int point_count_ = 20;
  float segment_length_ = 0.1F;
  std::shared_ptr<core::Mesh> input_mesh_;

  bool is_dirty_ = true;
  std::shared_ptr<core::Mesh> cached_result_;

  // Helper to calculate total curve length
  float calculate_curve_length(const core::Mesh &mesh) const;
};

} // namespace nodeflux::sop
