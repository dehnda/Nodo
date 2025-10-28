#pragma once

#include "nodo/core/error.hpp"
#include "nodo/core/geometry_container.hpp"
#include "nodo/core/mesh.hpp"
#include <optional>


namespace nodo::geometry {

/**
 * @brief Factory class for generating primitive geometry
 *
 * Provides clean, efficient generation of common 3D primitives
 * with configurable resolution and proper topology using GeometryContainer.
 * Uses std::optional for simple error handling in C++20.
 */
class MeshGenerator {
public:
  /**
   * @brief Generate a box geometry
   * @param min_corner Minimum corner coordinates
   * @param max_corner Maximum corner coordinates
   * @return Generated box as GeometryContainer with P and N attributes
   */
  static core::GeometryContainer box(const Eigen::Vector3d &min_corner,
                                     const Eigen::Vector3d &max_corner);

  /**
   * @brief Generate a sphere geometry
   * @param center Center point of the sphere
   * @param radius Radius of the sphere
   * @param subdivisions Number of subdivisions (controls resolution)
   * @return Optional GeometryContainer, nullopt on failure
   */
  static std::optional<core::GeometryContainer>
  sphere(const Eigen::Vector3d &center, double radius, int subdivisions = 3);

  /**
   * @brief Generate a cylinder geometry
   * @param bottom_center Center of the bottom face
   * @param top_center Center of the top face
   * @param radius Radius of the cylinder
   * @param segments Number of segments around the circumference
   * @return Optional GeometryContainer, nullopt on failure
   */
  static std::optional<core::GeometryContainer>
  cylinder(const Eigen::Vector3d &bottom_center,
           const Eigen::Vector3d &top_center, double radius, int segments = 16);

  /**
   * @brief Get the last error that occurred
   * @return Error information for the last failed operation
   */
  static const core::Error &last_error();

private:
  /// Generate icosphere geometry (for sphere implementation)
  static core::GeometryContainer
  generate_icosphere(const Eigen::Vector3d &center, double radius,
                     int subdivisions);

  /// Generate cylinder geometry
  static core::GeometryContainer
  generate_cylinder_geometry(const Eigen::Vector3d &bottom_center,
                             const Eigen::Vector3d &top_center, double radius,
                             int segments);

  /// Validate sphere parameters
  static bool validate_sphere_params(double radius, int subdivisions);

  /// Validate cylinder parameters
  static bool validate_cylinder_params(double radius, int segments);

  /// Set the last error for error reporting
  static void set_last_error(const core::Error &error);

  /// Thread-local storage for last error
  static thread_local core::Error last_error_;
};

} // namespace nodo::geometry
