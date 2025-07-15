#pragma once

#include "../core/error.hpp"
#include "../core/mesh.hpp"
#include "../geometry/torus_generator.hpp"
#include <optional>

namespace nodeflux::nodes {

/// @brief Node for generating torus meshes
class TorusNode {
public:
  /// @brief Create a torus node
  /// @param major_radius The radius from the center of the torus to the center of the tube
  /// @param minor_radius The radius of the tube
  /// @param major_segments Number of segments around the major circumference
  /// @param minor_segments Number of segments around the minor circumference (tube)
  TorusNode(double major_radius = geometry::TorusGenerator::DEFAULT_MAJOR_RADIUS,
           double minor_radius = geometry::TorusGenerator::DEFAULT_MINOR_RADIUS,
           int major_segments = geometry::TorusGenerator::DEFAULT_MAJOR_SEGMENTS,
           int minor_segments = geometry::TorusGenerator::DEFAULT_MINOR_SEGMENTS);

  /// @brief Generate the torus mesh
  /// @return Generated mesh or nullopt on error
  std::optional<core::Mesh> generate() const;

  /// @brief Get the last error that occurred
  /// @return Reference to the last error
  const core::Error &last_error() const;

  // Parameter setters
  void set_major_radius(double radius) { major_radius_ = radius; }
  void set_minor_radius(double radius) { minor_radius_ = radius; }
  void set_major_segments(int segments) { major_segments_ = segments; }
  void set_minor_segments(int segments) { minor_segments_ = segments; }

  // Parameter getters
  double major_radius() const { return major_radius_; }
  double minor_radius() const { return minor_radius_; }
  int major_segments() const { return major_segments_; }
  int minor_segments() const { return minor_segments_; }

private:
  double major_radius_;
  double minor_radius_;
  int major_segments_;
  int minor_segments_;
};

} // namespace nodeflux::nodes
