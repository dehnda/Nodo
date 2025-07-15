#pragma once

#include "../core/mesh.hpp"
#include "../core/error.hpp"
#include "../geometry/cylinder_generator.hpp"
#include <optional>

namespace nodeflux::nodes {

/// @brief Node for generating cylinder meshes
class CylinderNode {
public:
    /// @brief Create a cylinder node
    /// @param radius The radius of the cylinder
    /// @param height The height of the cylinder
    /// @param radial_segments Number of segments around the circumference
    /// @param height_segments Number of vertical segments
    /// @param top_cap Whether to include the top cap
    /// @param bottom_cap Whether to include the bottom cap
    CylinderNode(
        double radius = 1.0,
        double height = 2.0,
        int radial_segments = 32,
        int height_segments = 1,
        bool top_cap = true,
        bool bottom_cap = true
    );

    /// @brief Generate the cylinder mesh
    /// @return Generated mesh or nullopt on error
    std::optional<core::Mesh> generate() const;

    /// @brief Get the last error that occurred
    /// @return Reference to the last error
    static const core::Error& last_error();

    // Parameter setters
    void set_radius(double radius) { radius_ = radius; }
    void set_height(double height) { height_ = height; }
    void set_radial_segments(int segments) { radial_segments_ = segments; }
    void set_height_segments(int segments) { height_segments_ = segments; }
    void set_top_cap(bool enable) { top_cap_ = enable; }
    void set_bottom_cap(bool enable) { bottom_cap_ = enable; }

    // Parameter getters
    double radius() const { return radius_; }
    double height() const { return height_; }
    int radial_segments() const { return radial_segments_; }
    int height_segments() const { return height_segments_; }
    bool top_cap() const { return top_cap_; }
    bool bottom_cap() const { return bottom_cap_; }

private:
    double radius_;
    double height_;
    int radial_segments_;
    int height_segments_;
    bool top_cap_;
    bool bottom_cap_;
};

} // namespace nodeflux::nodes
