#pragma once

#include "nodo/core/geometry_container.hpp"
#include "nodo/core/types.hpp"
#include "nodo/sop/sop_node.hpp"

#include <Eigen/Dense>

#include <memory>
#include <optional>
#include <string>

namespace nodo::sop {

/**
 * @brief Mirror SOP - Creates mirrored copies of geometry
 *
 * Mirrors geometry across specified planes (XY, XZ, YZ) or custom planes.
 * Can create single mirrors or bilateral symmetry.
 */
class MirrorSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  enum class MirrorPlane {
    XY,    ///< Mirror across XY plane (Z=0)
    XZ,    ///< Mirror across XZ plane (Y=0)
    YZ,    ///< Mirror across YZ plane (X=0)
    CUSTOM ///< Mirror across custom plane
  };

private:
  MirrorPlane plane_ = MirrorPlane::YZ;
  core::Vector3 custom_point_ = core::Vector3::Zero();
  core::Vector3 custom_normal_ = core::Vector3::UnitY();
  bool keep_original_ = true;

public:
  /**
   * @brief Construct a new Mirror SOP
   * @param name Node name for debugging
   */
  explicit MirrorSOP(const std::string& name = "mirror");

  /**
   * @brief Set the mirror plane
   * @param plane The plane to mirror across
   */
  void set_plane(MirrorPlane plane) { set_parameter("plane", static_cast<int>(plane)); }

  /**
   * @brief Get the current mirror plane
   * @return The current plane
   */
  MirrorPlane get_plane() const { return static_cast<MirrorPlane>(get_parameter<int>("plane", 2)); }

  /**
   * @brief Set custom mirror plane (for CUSTOM plane type)
   * @param point A point on the plane
   * @param normal The plane normal vector
   */
  void set_custom_plane(const core::Vector3& point, const core::Vector3& normal) {
    set_parameter("custom_point_x", static_cast<float>(point.x()));
    set_parameter("custom_point_y", static_cast<float>(point.y()));
    set_parameter("custom_point_z", static_cast<float>(point.z()));
    set_parameter("custom_normal_x", static_cast<float>(normal.x()));
    set_parameter("custom_normal_y", static_cast<float>(normal.y()));
    set_parameter("custom_normal_z", static_cast<float>(normal.z()));
  }

  /**
   * @brief Set whether to keep the original geometry
   * @param keep_original If true, result includes both original and mirrored
   */
  void set_keep_original(bool keep_original) { set_parameter("keep_original", keep_original ? 1 : 0); }

  /**
   * @brief Get whether original geometry is kept
   * @return True if original is kept
   */
  bool get_keep_original() const { return keep_original_; }

  /**
   * @brief Convert mirror plane to string
   * @param plane The mirror plane
   * @return String representation
   */
  static std::string plane_to_string(MirrorPlane plane);

protected:
  /**
   * @brief Execute the mirror operation (SOPNode override)
   */
  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override;

private:
  /**
   * @brief Mirror vertices across specified plane
   * @param vertices Input vertices
   * @param plane_point Point on the mirror plane
   * @param plane_normal Normal of the mirror plane
   * @return Mirrored vertices
   */
  std::vector<core::Vector3> mirror_vertices(const std::vector<core::Vector3>& vertices,
                                             const core::Vector3& plane_point, const core::Vector3& plane_normal) const;
};

} // namespace nodo::sop
