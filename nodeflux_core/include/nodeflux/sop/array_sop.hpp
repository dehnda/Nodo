#pragma once

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/core/types.hpp"
#include "nodeflux/sop/geometry_data.hpp"
#include "nodeflux/sop/sop_node.hpp"

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
class ArraySOP : public SOPNode {
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
  explicit ArraySOP(const std::string &name = "array")
      : SOPNode(name, "ArraySOP") {
    // Add input ports
    input_ports_.add_port("mesh", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);
  }

  const std::string &get_name() const { return name_; }

  // Configuration methods
  void set_array_type(ArrayType type) {
    if (array_type_ != type) {
      array_type_ = type;
      mark_dirty();
    }
  }

  void set_count(int count) {
    int new_count = std::max(1, count);
    if (count_ != new_count) {
      count_ = new_count;
      mark_dirty();
    }
  }

  // Linear array configuration
  void set_linear_offset(const core::Vector3 &offset) {
    linear_offset_ = offset;
    mark_dirty();
  }

  // Radial array configuration
  void set_radial_center(const core::Vector3 &center) {
    radial_center_ = center;
    mark_dirty();
  }

  void set_radial_radius(float radius) {
    if (radial_radius_ != radius) {
      radial_radius_ = radius;
      mark_dirty();
    }
  }

  void set_angle_step(float degrees) {
    if (angle_step_ != degrees) {
      angle_step_ = degrees;
      mark_dirty();
    }
  }

  // Grid array configuration
  void set_grid_size(int width, int height) {
    grid_size_ = core::Vector2i(width, height);
    mark_dirty();
  }

  void set_grid_spacing(float x_spacing, float y_spacing) {
    grid_spacing_ = Eigen::Vector2f(x_spacing, y_spacing);
    mark_dirty();
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
   * @brief Execute the array operation (SOPNode override)
   */
  std::shared_ptr<GeometryData> execute() override;

private:
  std::optional<core::Mesh> create_linear_array(const core::Mesh &input_mesh);
  std::optional<core::Mesh> create_radial_array(const core::Mesh &input_mesh);
  std::optional<core::Mesh> create_grid_array(const core::Mesh &input_mesh);

  void add_instance_attributes(std::shared_ptr<GeometryData> geo_data,
                                size_t verts_per_instance,
                                size_t faces_per_instance,
                                int instance_count);
};

} // namespace nodeflux::sop
