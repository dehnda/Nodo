#pragma once

#include "nodeflux/core/mesh.hpp"
#include <Eigen/Dense>
#include <optional>
#include <string>
#include <memory>

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
    Eigen::Vector3d custom_point_ = Eigen::Vector3d::Zero();
    Eigen::Vector3d custom_normal_ = Eigen::Vector3d::UnitY();
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
    void set_custom_plane(const Eigen::Vector3d& point, const Eigen::Vector3d& normal);

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
    const std::string& get_name() const { return name_; }

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
    std::vector<Eigen::Vector3d> mirror_vertices(
        const std::vector<Eigen::Vector3d>& vertices,
        const Eigen::Vector3d& plane_point,
        const Eigen::Vector3d& plane_normal) const;
};

} // namespace nodeflux::sop
