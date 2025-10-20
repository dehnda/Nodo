#pragma once

#include "../core/error.hpp"
#include "../core/mesh.hpp"
#include <optional>

namespace nodeflux::geometry {

/// @brief Generates plane/grid meshes
class PlaneGenerator {
public:
  /// @brief Generate a plane mesh
  /// @param width The width of the plane
  /// @param height The height of the plane
  /// @param width_segments Number of segments along the width
  /// @param height_segments Number of segments along the height
  /// @return Generated plane mesh or nullopt on error
  static std::optional<core::Mesh> generate(double width = 2.0,
                                            double height = 2.0,
                                            int width_segments = 1,
                                            int height_segments = 1);

  /// @brief Get the last error that occurred
  /// @return Reference to the last error
  static const core::Error &last_error();

private:
  static void set_last_error(const core::Error &error);
  static thread_local core::Error last_error_;
};

} // namespace nodeflux::geometry
