#pragma once

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/core/error.hpp"
#include <optional>

namespace nodeflux::geometry {

/**
 * @brief Factory class for generating primitive meshes
 * 
 * Provides clean, efficient generation of common 3D primitives
 * with configurable resolution and proper mesh topology.
 * Uses std::optional for simple error handling in C++20.
 */
class MeshGenerator {
public:
    /**
     * @brief Generate a box mesh
     * @param min_corner Minimum corner coordinates
     * @param max_corner Maximum corner coordinates
     * @return Generated box mesh
     */
    static core::Mesh box(const Eigen::Vector3d& min_corner, 
                         const Eigen::Vector3d& max_corner);
    
    /**
     * @brief Generate a sphere mesh
     * @param center Center point of the sphere
     * @param radius Radius of the sphere
     * @param subdivisions Number of subdivisions (controls resolution)
     * @return Optional mesh, nullopt on failure
     */
    static std::optional<core::Mesh> sphere(const Eigen::Vector3d& center,
                                           double radius, 
                                           int subdivisions = 3);
    
    /**
     * @brief Generate a cylinder mesh
     * @param bottom_center Center of the bottom face
     * @param top_center Center of the top face
     * @param radius Radius of the cylinder
     * @param segments Number of segments around the circumference
     * @return Optional mesh, nullopt on failure
     */
    static std::optional<core::Mesh> cylinder(const Eigen::Vector3d& bottom_center,
                                             const Eigen::Vector3d& top_center,
                                             double radius,
                                             int segments = 16);
    
    /**
     * @brief Get the last error that occurred
     * @return Error information for the last failed operation
     */
    static const core::Error& last_error();

private:
    /// Generate icosphere mesh (for sphere implementation)
    static core::Mesh generate_icosphere(const Eigen::Vector3d& center,
                                        double radius, 
                                        int subdivisions);
    
    /// Generate cylinder geometry
    static core::Mesh generate_cylinder_geometry(const Eigen::Vector3d& bottom_center,
                                                const Eigen::Vector3d& top_center,
                                                double radius,
                                                int segments);
    
    /// Validate sphere parameters
    static bool validate_sphere_params(double radius, int subdivisions);
    
    /// Validate cylinder parameters  
    static bool validate_cylinder_params(double radius, int segments);
    
    /// Set the last error for error reporting
    static void set_last_error(const core::Error& error);
    
    /// Thread-local storage for last error
    static thread_local core::Error last_error_;
};

} // namespace nodeflux::geometry
