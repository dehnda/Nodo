#pragma once

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/core/types.hpp"

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
class MirrorSOP {
public:
  enum class MirrorPlane {
    XY,    ///< Mirror across XY plane (Z=0)
    XZ,    ///< Mirror across XZ plane (Y=0)
    YZ,    ///< Mirror across YZ plane (X=0)
    CUSTOM ///< Mirror across custom plane
  };

private:
  std::string name_;
  MirrorPlane plane_ = MirrorPlane::YZ;
  core::Vector3 custom_point_ = core::Vector3::Zero();
  core::Vector3 custom_normal_ = core::Vector3::UnitY();
  bool keep_original_ = true;
  bool is_dirty_ = true;
  std::shared_ptr<core::Mesh> cached_result_;
  std::shared_ptr<core::Mesh> input_mesh_;

public:
  /**
   * @brief Construct a new Mirror SOP
   * @param name Node name for debugging
   */
  explicit MirrorSOP(std::string name);

  /**
   * @brief Set the mirror plane
   * @param plane The plane to mirror across
   */
  void set_plane(MirrorPlane plane);

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
                        const core::Vector3 &normal);

  /**
   * @brief Set whether to keep the original geometry
   * @param keep_original If true, result includes both original and mirrored
   */
  void set_keep_original(bool keep_original);

  /**
   * @brief Get whether original geometry is kept
   * @return True if original is kept
   */
  bool get_keep_original() const { return keep_original_; }

  /**
   * @brief Set the input mesh
   * @param mesh The mesh to mirror
   */
  void set_input_mesh(std::shared_ptr<core::Mesh> mesh);

  /**
   * @brief Execute the mirror operation
   * @return Result mesh or nullopt on failure
   */
  std::optional<core::Mesh> execute();

  /**
   * @brief Get cached result or compute if dirty
   * @return Result mesh or nullopt on failure
   */
  std::shared_ptr<core::Mesh> cook();

  /**
   * @brief Mark node as needing recomputation
   */
  void mark_dirty() {
    is_dirty_ = true;
    cached_result_.reset();
  }

  /**
   * @brief Get node name
   * @return The node name
   */
  const std::string &get_name() const { return name_; }

  /**
   * @brief Convert mirror plane to string
   * @param plane The mirror plane
   * @return String representation
   */
  static std::string plane_to_string(MirrorPlane plane);

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
