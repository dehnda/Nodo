#pragma once

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/sop/geometry_data.hpp"
#include "nodeflux/sop/sop_node.hpp"
#include <Eigen/Dense>
#include <memory>
#include <string>

namespace nodeflux::sop {

/**
 * @brief Extrude faces along their normals or a specified direction
 *
 * ExtrudeSOP creates geometric extrusions by moving faces along vectors.
 * Supports multiple extrusion modes for procedural modeling workflows.
 */
class ExtrudeSOP : public SOPNode {
public:
  /// Extrusion modes for different geometric operations
  enum class ExtrusionMode {
    FACE_NORMALS,      ///< Extrude along individual face normals
    UNIFORM_DIRECTION, ///< Extrude all faces in the same direction
    REGION_NORMALS     ///< Extrude faces along averaged region normals
  };

  /**
   * @brief Construct extrude operator with name
   * @param name Unique identifier for this SOP instance
   */
  explicit ExtrudeSOP(const std::string &name = "extrude");

  /**
   * @brief Set extrusion distance
   * @param distance How far to extrude (can be negative for inward)
   */
  void set_distance(double distance) {
    if (distance_ != distance) {
      distance_ = distance;
      mark_dirty();
    }
  }

  /**
   * @brief Get extrusion distance
   */
  double get_distance() const { return distance_; }

  /**
   * @brief Set extrusion direction for uniform mode
   * @param direction Unit vector specifying extrusion direction
   */
  void set_direction(const Eigen::Vector3d &direction) {
    if (!direction_.isApprox(direction)) {
      direction_ = direction.normalized();
      mark_dirty();
    }
  }

  /**
   * @brief Get extrusion direction
   */
  const Eigen::Vector3d &get_direction() const { return direction_; }

  /**
   * @brief Set extrusion mode
   * @param mode Type of extrusion to perform
   */
  void set_mode(ExtrusionMode mode) {
    if (mode_ != mode) {
      mode_ = mode;
      mark_dirty();
    }
  }

  /**
   * @brief Get extrusion mode
   */
  ExtrusionMode get_mode() const { return mode_; }

  /**
   * @brief Set inset amount for creating border around extruded faces
   * @param inset Inset distance (0.0 = no inset)
   */
  void set_inset(double inset) {
    if (inset_ != inset) {
      inset_ = inset;
      mark_dirty();
    }
  }

  /**
   * @brief Get inset amount
   */
  double get_inset() const { return inset_; }

protected:
  /**
   * @brief Execute the extrusion operation (SOPNode override)
   */
  std::shared_ptr<GeometryData> execute() override;

private:
  /// Extrude faces along their individual normals
  core::Mesh extrude_face_normals(const core::Mesh &input);

  /// Extrude all faces in uniform direction
  core::Mesh extrude_uniform_direction(const core::Mesh &input);

  /// Extrude faces along averaged region normals
  core::Mesh extrude_region_normals(const core::Mesh &input);

  /// Calculate face normals for the mesh
  Eigen::MatrixXd calculate_face_normals(const core::Mesh &mesh);

  /// Create inset faces if inset > 0
  void apply_inset(core::Mesh &mesh, double inset_amount);

  // Extrusion parameters
  double distance_ = 1.0;                             ///< Extrusion distance
  Eigen::Vector3d direction_{0.0, 0.0, 1.0};          ///< Uniform direction
  ExtrusionMode mode_ = ExtrusionMode::FACE_NORMALS;  ///< Extrusion algorithm
  double inset_ = 0.0;                                ///< Inset amount
};

} // namespace nodeflux::sop
