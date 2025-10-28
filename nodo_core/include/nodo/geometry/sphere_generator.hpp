#pragma once

#include "../core/error.hpp"
#include "../core/geometry_container.hpp"
#include "../core/mesh.hpp"
#include <optional>


namespace nodo::geometry {

/// @brief Generates sphere meshes using UV sphere algorithm
class SphereGenerator {
public:
  /// @brief Generate a UV sphere mesh as GeometryContainer
  /// @param radius The radius of the sphere
  /// @param u_segments Number of horizontal segments (longitude divisions)
  /// @param v_segments Number of vertical segments (latitude divisions)
  /// @return Generated sphere as GeometryContainer or nullopt on error
  static std::optional<core::GeometryContainer>
  generate_uv_sphere(double radius = 1.0, int u_segments = 32,
                     int v_segments = 16);

  /// @brief Generate an icosphere mesh (geodesic sphere) as GeometryContainer
  /// @param radius The radius of the sphere
  /// @param subdivisions Number of subdivision levels (0-4 recommended)
  /// @return Generated icosphere as GeometryContainer or nullopt on error
  static std::optional<core::GeometryContainer>
  generate_icosphere(double radius = 1.0, int subdivisions = 2);

  /// @brief Get the last error that occurred
  /// @return Reference to the last error
  static const core::Error &last_error();

private:
  static void set_last_error(const core::Error &error);
  static thread_local core::Error last_error_;

  // Helper functions for icosphere generation
  static void subdivide_triangle(const Eigen::Vector3d &v1,
                                 const Eigen::Vector3d &v2,
                                 const Eigen::Vector3d &v3,
                                 std::vector<Eigen::Vector3d> &vertices,
                                 std::vector<std::array<int, 3>> &faces,
                                 int level);

  static Eigen::Vector3d normalize_vertex(const Eigen::Vector3d &vertex,
                                          double radius);
};

} // namespace nodo::geometry
