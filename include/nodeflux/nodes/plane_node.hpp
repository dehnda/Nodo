#pragma once

#include "../core/mesh.hpp"
#include "../core/error.hpp"
#include "../geometry/plane_generator.hpp"
#include <optional>

namespace nodeflux::nodes {

/// @brief Node for generating plane/grid meshes
class PlaneNode {
public:
    /// @brief Create a plane node
    /// @param width The width of the plane
    /// @param height The height of the plane
    /// @param width_segments Number of segments along the width
    /// @param height_segments Number of segments along the height
    PlaneNode(
        double width = 2.0,
        double height = 2.0,
        int width_segments = 1,
        int height_segments = 1
    );

    /// @brief Generate the plane mesh
    /// @return Generated mesh or nullopt on error
    std::optional<core::Mesh> generate() const;

    /// @brief Get the last error that occurred
    /// @return Reference to the last error
    static const core::Error& last_error();

    // Parameter setters
    void set_width(double width) { width_ = width; }
    void set_height(double height) { height_ = height; }
    void set_width_segments(int segments) { width_segments_ = segments; }
    void set_height_segments(int segments) { height_segments_ = segments; }

    // Parameter getters
    double width() const { return width_; }
    double height() const { return height_; }
    int width_segments() const { return width_segments_; }
    int height_segments() const { return height_segments_; }

private:
    double width_;
    double height_;
    int width_segments_;
    int height_segments_;
};

} // namespace nodeflux::nodes
