#pragma once

#include "../core/mesh.hpp"
#include "../core/error.hpp"
#include <optional>

namespace nodeflux::geometry {

/// @brief Generates box/cube meshes with configurable subdivisions
class BoxGenerator {
public:
    /// @brief Generate a box mesh
    /// @param width The width of the box (X dimension)
    /// @param height The height of the box (Y dimension)
    /// @param depth The depth of the box (Z dimension)
    /// @param width_segments Number of subdivisions along width
    /// @param height_segments Number of subdivisions along height
    /// @param depth_segments Number of subdivisions along depth
    /// @return Generated box mesh or nullopt on error
    static std::optional<core::Mesh> generate(
        double width = 2.0,
        double height = 2.0,
        double depth = 2.0,
        int width_segments = 1,
        int height_segments = 1,
        int depth_segments = 1
    );

    /// @brief Generate a box mesh from min/max corners
    /// @param min_corner Minimum corner coordinates
    /// @param max_corner Maximum corner coordinates
    /// @param width_segments Number of subdivisions along width
    /// @param height_segments Number of subdivisions along height
    /// @param depth_segments Number of subdivisions along depth
    /// @return Generated box mesh or nullopt on error
    static std::optional<core::Mesh> generate_from_bounds(
        const Eigen::Vector3d& min_corner,
        const Eigen::Vector3d& max_corner,
        int width_segments = 1,
        int height_segments = 1,
        int depth_segments = 1
    );

    /// @brief Get the last error that occurred
    /// @return Reference to the last error
    static const core::Error& last_error();

private:
    static void set_last_error(const core::Error& error);
    static thread_local core::Error last_error_;
    
    // Helper function to generate a subdivided quad face
    static void generate_face(
        core::Mesh& mesh,
        int& vertex_index,
        int& face_index,
        const Eigen::Vector3d& corner1,
        const Eigen::Vector3d& corner2,
        const Eigen::Vector3d& corner3,
        const Eigen::Vector3d& corner4,
        int u_segments,
        int v_segments,
        bool flip_normal = false
    );
};

} // namespace nodeflux::geometry
