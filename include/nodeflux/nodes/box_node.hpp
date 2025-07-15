#pragma once

#include "../core/mesh.hpp"
#include "../core/error.hpp"
#include "../geometry/box_generator.hpp"
#include <optional>

namespace nodeflux::nodes {

/// @brief Node for generating box/cube meshes
class BoxNode {
public:
    /// @brief Create a box node
    /// @param width The width of the box (X dimension)
    /// @param height The height of the box (Y dimension)
    /// @param depth The depth of the box (Z dimension)
    /// @param width_segments Number of subdivisions along width
    /// @param height_segments Number of subdivisions along height
    /// @param depth_segments Number of subdivisions along depth
    BoxNode(
        double width = 2.0,
        double height = 2.0,
        double depth = 2.0,
        int width_segments = 1,
        int height_segments = 1,
        int depth_segments = 1
    );

    /// @brief Create a box node from min/max corners
    /// @param min_corner Minimum corner coordinates
    /// @param max_corner Maximum corner coordinates
    /// @param width_segments Number of subdivisions along width
    /// @param height_segments Number of subdivisions along height
    /// @param depth_segments Number of subdivisions along depth
    /// @return BoxNode configured for the specified bounds
    static BoxNode create_from_bounds(
        const Eigen::Vector3d& min_corner,
        const Eigen::Vector3d& max_corner,
        int width_segments = 1,
        int height_segments = 1,
        int depth_segments = 1
    );

    /// @brief Generate the box mesh
    /// @return Generated mesh or nullopt on error
    std::optional<core::Mesh> generate() const;

    /// @brief Get the last error that occurred
    /// @return Reference to the last error
    static const core::Error& last_error();

    // Parameter setters
    void set_width(double width) { width_ = width; }
    void set_height(double height) { height_ = height; }
    void set_depth(double depth) { depth_ = depth; }
    void set_width_segments(int segments) { width_segments_ = segments; }
    void set_height_segments(int segments) { height_segments_ = segments; }
    void set_depth_segments(int segments) { depth_segments_ = segments; }
    void set_bounds(const Eigen::Vector3d& min_corner, const Eigen::Vector3d& max_corner);

    // Parameter getters
    double width() const { return width_; }
    double height() const { return height_; }
    double depth() const { return depth_; }
    int width_segments() const { return width_segments_; }
    int height_segments() const { return height_segments_; }
    int depth_segments() const { return depth_segments_; }
    bool use_bounds() const { return use_bounds_; }
    Eigen::Vector3d min_corner() const { return min_corner_; }
    Eigen::Vector3d max_corner() const { return max_corner_; }

private:
    double width_;
    double height_;
    double depth_;
    int width_segments_;
    int height_segments_;
    int depth_segments_;
    bool use_bounds_;
    Eigen::Vector3d min_corner_;
    Eigen::Vector3d max_corner_;
};

} // namespace nodeflux::nodes
