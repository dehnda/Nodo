#pragma once

#include "nodeflux/core/geometry_container.hpp"
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
    set_parameter("translate_x", static_cast<float>(x));
    set_parameter("translate_y", static_cast<float>(y));
    set_parameter("translate_z", static_cast<float>(z));
  }

  /**
   * @brief Set rotation in degrees
   * @param x Rotation around X axis (degrees)
   * @param y Rotation around Y axis (degrees)
   * @param z Rotation around Z axis (degrees)
   */
  void set_rotation(double x, double y, double z) {
    set_parameter("rotate_x", static_cast<float>(x));
    set_parameter("rotate_y", static_cast<float>(y));
    set_parameter("rotate_z", static_cast<float>(z));
  }

  /**
   * @brief Set scale
   * @param x Scale along X axis
   * @param y Scale along Y axis
   * @param z Scale along Z axis
   */
  void set_scale(double x, double y, double z) {
    set_parameter("scale_x", static_cast<float>(x));
    set_parameter("scale_y", static_cast<float>(y));
    set_parameter("scale_z", static_cast<float>(z));
  }

  /**
   * @brief Set uniform scale
   * @param scale Uniform scale factor
   */
  void set_uniform_scale(double scale) { set_scale(scale, scale, scale); }

  // Getters
  Eigen::Vector3d get_translation() const {
    return {get_parameter<float>("translate_x", 0.0F),
            get_parameter<float>("translate_y", 0.0F),
            get_parameter<float>("translate_z", 0.0F)};
  }
  Eigen::Vector3d get_rotation() const {
    return {get_parameter<float>("rotate_x", 0.0F),
            get_parameter<float>("rotate_y", 0.0F),
            get_parameter<float>("rotate_z", 0.0F)};
  }
  Eigen::Vector3d get_scale() const {
    return {get_parameter<float>("scale_x", 1.0F),
            get_parameter<float>("scale_y", 1.0F),
            get_parameter<float>("scale_z", 1.0F)};
  }

protected:
  /**
   * @brief Execute the transform operation (SOPNode override)
   * @return Transformed geometry container
   */
  std::shared_ptr<core::GeometryContainer> execute() override;

private:
  /**
   * @brief Build 4x4 transformation matrix
   */
  Eigen::Matrix4d build_transform_matrix() const;

  // No private member variables needed - parameters stored in base class!
};

} // namespace nodeflux::sop
