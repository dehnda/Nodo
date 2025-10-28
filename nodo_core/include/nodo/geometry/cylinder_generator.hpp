#pragma once

#include "../core/error.hpp"
#include "../core/geometry_container.hpp"
#include <optional>

namespace nodo::geometry {

/// @brief Generates cylinder meshes with configurable caps
class CylinderGenerator {
public:
  /// @brief Generate a cylinder mesh as GeometryContainer
  /// @param radius The radius of the cylinder
  /// @param height The height of the cylinder
  /// @param radial_segments Number of segments around the circumference
  /// @param height_segments Number of vertical segments
  /// @param top_cap Whether to include the top cap
  /// @param bottom_cap Whether to include the bottom cap
  /// @return Generated cylinder as GeometryContainer or nullopt on error
  static std::optional<core::GeometryContainer>
  generate(double radius = 1.0, double height = 2.0, int radial_segments = 32,
           int height_segments = 1, bool top_cap = true,
           bool bottom_cap = true);

  /// @brief Get the last error that occurred
  /// @return Reference to the last error
  static const core::Error &last_error();

private:
  static void set_last_error(const core::Error &error);
  static thread_local core::Error last_error_;
};

} // namespace nodo::geometry
