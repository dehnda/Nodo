#pragma once

#include "nodo/core/geometry_container.hpp"
#include "nodo/sop/sop_node.hpp"
#include <Eigen/Dense>
#include <memory>
#include <string>

namespace nodo::sop {

/**
 * @brief Transform SOP - Apply translation, rotation, and scale to geometry
 *
 * Transforms input geometry using standard 3D transformations.
 * Order of operations: Scale -> Rotate -> Translate
 */
class TransformSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

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
    set_parameter("translate",
                  Eigen::Vector3f(static_cast<float>(x), static_cast<float>(y),
                                  static_cast<float>(z)));
  }

  /**
   * @brief Set rotation in degrees
   * @param x Rotation around X axis (degrees)
   * @param y Rotation around Y axis (degrees)
   * @param z Rotation around Z axis (degrees)
   */
  void set_rotation(double x, double y, double z) {
    set_parameter("rotate",
                  Eigen::Vector3f(static_cast<float>(x), static_cast<float>(y),
                                  static_cast<float>(z)));
  }

  /**
   * @brief Set scale
   * @param x Scale along X axis
   * @param y Scale along Y axis
   * @param z Scale along Z axis
   */
  void set_scale(double x, double y, double z) {
    set_parameter("scale",
                  Eigen::Vector3f(static_cast<float>(x), static_cast<float>(y),
                                  static_cast<float>(z)));
  }

  /**
   * @brief Set uniform scale
   * @param scale Uniform scale factor
   */
  void set_uniform_scale(double scale) { set_scale(scale, scale, scale); }

  // Getters
  Eigen::Vector3d get_translation() const {
    auto vec = get_parameter<Eigen::Vector3f>(
        "translate", Eigen::Vector3f(0.0F, 0.0F, 0.0F));
    return {vec.x(), vec.y(), vec.z()};
  }
  Eigen::Vector3d get_rotation() const {
    auto vec = get_parameter<Eigen::Vector3f>(
        "rotate", Eigen::Vector3f(0.0F, 0.0F, 0.0F));
    return {vec.x(), vec.y(), vec.z()};
  }
  Eigen::Vector3d get_scale() const {
    auto vec = get_parameter<Eigen::Vector3f>(
        "scale", Eigen::Vector3f(1.0F, 1.0F, 1.0F));
    return {vec.x(), vec.y(), vec.z()};
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

} // namespace nodo::sop
