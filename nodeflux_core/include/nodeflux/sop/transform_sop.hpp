#pragma once

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/sop/geometry_data.hpp"
#include "nodeflux/sop/sop_node.hpp"
#include <Eigen/Dense>
#include <memory>
#include <string>

namespace nodeflux::sop {

/**
 * @brief Transform SOP - Apply translation, rotation, and scale to geometry
 *
 * Transforms input geometry using standard 3D transformations.
 * Order of operations: Scale -> Rotate -> Translate
 */
class TransformSOP : public SOPNode {
public:
  /**
   * @brief Construct transform operator with name
   * @param name Unique identifier for this SOP instance
   */
  explicit TransformSOP(const std::string &name = "transform");

  /**
   * @brief Set translation
   * @param x Translation along X axis
   * @param y Translation along Y axis
   * @param z Translation along Z axis
   */
  void set_translation(double x, double y, double z) {
    if (translate_x_ != x || translate_y_ != y || translate_z_ != z) {
      translate_x_ = x;
      translate_y_ = y;
      translate_z_ = z;
      mark_dirty();
    }
  }

  /**
   * @brief Set rotation in degrees
   * @param x Rotation around X axis (degrees)
   * @param y Rotation around Y axis (degrees)
   * @param z Rotation around Z axis (degrees)
   */
  void set_rotation(double x, double y, double z) {
    if (rotate_x_ != x || rotate_y_ != y || rotate_z_ != z) {
      rotate_x_ = x;
      rotate_y_ = y;
      rotate_z_ = z;
      mark_dirty();
    }
  }

  /**
   * @brief Set scale
   * @param x Scale along X axis
   * @param y Scale along Y axis
   * @param z Scale along Z axis
   */
  void set_scale(double x, double y, double z) {
    if (scale_x_ != x || scale_y_ != y || scale_z_ != z) {
      scale_x_ = x;
      scale_y_ = y;
      scale_z_ = z;
      mark_dirty();
    }
  }

  /**
   * @brief Set uniform scale
   * @param scale Uniform scale factor
   */
  void set_uniform_scale(double scale) {
    set_scale(scale, scale, scale);
  }

  // Getters
  Eigen::Vector3d get_translation() const { return {translate_x_, translate_y_, translate_z_}; }
  Eigen::Vector3d get_rotation() const { return {rotate_x_, rotate_y_, rotate_z_}; }
  Eigen::Vector3d get_scale() const { return {scale_x_, scale_y_, scale_z_}; }

protected:
  /**
   * @brief Execute the transform operation (SOPNode override)
   */
  std::shared_ptr<GeometryData> execute() override;

private:
  /**
   * @brief Build 4x4 transformation matrix
   */
  Eigen::Matrix4d build_transform_matrix() const;

  // Transform parameters
  double translate_x_ = 0.0;
  double translate_y_ = 0.0;
  double translate_z_ = 0.0;

  double rotate_x_ = 0.0; // degrees
  double rotate_y_ = 0.0; // degrees
  double rotate_z_ = 0.0; // degrees

  double scale_x_ = 1.0;
  double scale_y_ = 1.0;
  double scale_z_ = 1.0;
};

} // namespace nodeflux::sop
