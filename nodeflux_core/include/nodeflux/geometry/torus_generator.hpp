#pragma once

#include "../core/error.hpp"
#include "../core/geometry_container.hpp"
#include <optional>

namespace nodeflux::geometry {

/// @brief Generates torus (donut-shaped) geometry
class TorusGenerator {
public:
  // Default parameter constants
  static constexpr double DEFAULT_MAJOR_RADIUS = 1.0;
  static constexpr double DEFAULT_MINOR_RADIUS = 0.3;
  static constexpr int DEFAULT_MAJOR_SEGMENTS = 48;
  static constexpr int DEFAULT_MINOR_SEGMENTS = 12;

  /// @brief Generate a torus geometry
  /// @param major_radius The radius from the center of the torus to the center
  /// of the tube
  /// @param minor_radius The radius of the tube
  /// @param major_segments Number of segments around the major circumference
  /// @param minor_segments Number of segments around the minor circumference
  /// (tube)
  /// @return Generated torus as GeometryContainer or nullopt on error
  static std::optional<core::GeometryContainer>
  generate(double major_radius = DEFAULT_MAJOR_RADIUS,
           double minor_radius = DEFAULT_MINOR_RADIUS,
           int major_segments = DEFAULT_MAJOR_SEGMENTS,
           int minor_segments = DEFAULT_MINOR_SEGMENTS);

  /// @brief Get the last error that occurred
  /// @return Reference to the last error
  static const core::Error &last_error();

private:
  static thread_local core::Error last_error_;
};

} // namespace nodeflux::geometry
