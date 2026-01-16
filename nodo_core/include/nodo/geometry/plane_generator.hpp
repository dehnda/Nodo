#pragma once

#include "../core/error.hpp"
#include "../core/geometry_container.hpp"

#include <optional>

namespace nodo::geometry {

/// @brief Generates plane/grid geometry
class PlaneGenerator {
public:
  /// @brief Generate a plane geometry
  /// @param width The width of the plane
  /// @param height The height of the plane
  /// @param width_segments Number of segments along the width
  /// @param height_segments Number of segments along the height
  /// @return Generated plane as GeometryContainer or nullopt on error
  static std::optional<core::GeometryContainer> generate(double width = 2.0, double height = 2.0,
                                                         int width_segments = 1, int height_segments = 1);

  /// @brief Get the last error that occurred
  /// @return Reference to the last error
  static const core::Error& last_error();

private:
  static void set_last_error(const core::Error& error);
  static thread_local core::Error last_error_;
};

} // namespace nodo::geometry
