#pragma once

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/core/types.hpp"
#include "nodeflux/sop/geometry_data.hpp"
#include "nodeflux/sop/sop_node.hpp"

#include <Eigen/Dense>
#include <memory>
#include <optional>
#include <string>

namespace nodeflux::sop {

/**
 * @brief Mirror SOP - Creates mirrored copies of geometry
 *
 * Mirrors geometry across specified planes (XY, XZ, YZ) or custom planes.
 * Can create single mirrors or bilateral symmetry.
 */
class MirrorSOP : public SOPNode {
public:
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
  void set_plane(MirrorPlane plane) {
    if (plane_ != plane) {
      plane_ = plane;
      mark_dirty();
    }
  }

  /**
   * @brief Get the current mirror plane
   * @return The current plane
   */
  MirrorPlane get_plane() const { return plane_; }

  /**
   * @brief Set custom mirror plane (for CUSTOM plane type)
   * @param point A point on the plane
   * @param normal The plane normal vector
   */
  void set_custom_plane(const core::Vector3 &point,
                        const core::Vector3 &normal) {
    custom_point_ = point;
    custom_normal_ = normal;
    mark_dirty();
  }

  /**
   * @brief Set whether to keep the original geometry
   * @param keep_original If true, result includes both original and mirrored
   */
  void set_keep_original(bool keep_original) {
    if (keep_original_ != keep_original) {
      keep_original_ = keep_original;
      mark_dirty();
    }
  }

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
  std::shared_ptr<GeometryData> execute() override;

private:
  /**
   * @brief Mirror vertices across specified plane
   * @param vertices Input vertices
   * @param plane_point Point on the mirror plane
   * @param plane_normal Normal of the mirror plane
   * @return Mirrored vertices
   */
  std::vector<core::Vector3>
  mirror_vertices(const std::vector<core::Vector3> &vertices,
                  const core::Vector3 &plane_point,
                  const core::Vector3 &plane_normal) const;
};

} // namespace nodeflux::sop
