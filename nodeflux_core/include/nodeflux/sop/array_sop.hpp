#pragma once

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/core/types.hpp"

#include <Eigen/Dense>
#include <optional>
#include <string>

namespace nodeflux::sop {

// Default constants
constexpr float DEFAULT_RADIAL_RADIUS = 2.0F;
constexpr float DEFAULT_ANGLE_STEP = 60.0F; // degrees

/**
 * @brief Array SOP - Duplicates geometry in various patterns
 *
 * Creates multiple copies of input geometry arranged in linear, radial, or grid
 * patterns. This is a simplified version that works with the existing mesh
 * system.
 */
class ArraySOP {
public:
  enum class ArrayType {
    LINEAR, ///< Copies along a line with specified offset
    RADIAL, ///< Copies around a center point with rotation
    GRID    ///< Copies in a 2D grid pattern
  };

private:
  std::string name_;
  ArrayType array_type_ = ArrayType::LINEAR;
  int count_ = 3;

  // Linear array parameters
  core::Vector3 linear_offset_{1.0F, 0.0F, 0.0F};

  // Radial array parameters
  core::Vector3 radial_center_{0.0F, 0.0F, 0.0F};
  float radial_radius_ = DEFAULT_RADIAL_RADIUS;
  float angle_step_ = DEFAULT_ANGLE_STEP; // degrees

  // Grid array parameters
  core::Vector2i grid_size_{3, 3};
  Eigen::Vector2f grid_spacing_{1.0F, 1.0F};

public:
  explicit ArraySOP(std::string name = "array") : name_(std::move(name)) {}

  const std::string &get_name() const { return name_; }

  // Configuration methods
  void set_array_type(ArrayType type) { array_type_ = type; }
  void set_count(int count) { count_ = std::max(1, count); }

  // Linear array configuration
  void set_linear_offset(const core::Vector3 &offset) {
    linear_offset_ = offset;
  }

  // Radial array configuration
  void set_radial_center(const core::Vector3 &center) {
    radial_center_ = center;
  }
  void set_radial_radius(float radius) { radial_radius_ = radius; }
  void set_angle_step(float degrees) { angle_step_ = degrees; }

  // Grid array configuration
  void set_grid_size(int width, int height) {
    grid_size_ = core::Vector2i(width, height);
  }
  void set_grid_spacing(float x_spacing, float y_spacing) {
    grid_spacing_ = Eigen::Vector2f(x_spacing, y_spacing);
  }

  // Getters
  ArrayType get_array_type() const { return array_type_; }
  int get_count() const { return count_; }
  const core::Vector3 &get_linear_offset() const { return linear_offset_; }
  const core::Vector3 &get_radial_center() const { return radial_center_; }
  float get_radial_radius() const { return radial_radius_; }
  float get_angle_step() const { return angle_step_; }
  const core::Vector2i &get_grid_size() const { return grid_size_; }
  const core::Vector2f &get_grid_spacing() const { return grid_spacing_; }

  /**
   * @brief Apply array operation to input mesh
   */
  std::optional<core::Mesh> process(const core::Mesh &input_mesh);

private:
  std::optional<core::Mesh> create_linear_array(const core::Mesh &input_mesh);
  std::optional<core::Mesh> create_radial_array(const core::Mesh &input_mesh);
  std::optional<core::Mesh> create_grid_array(const core::Mesh &input_mesh);
};

} // namespace nodeflux::sop
