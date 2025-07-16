#pragma once

#include "../core/mesh.hpp"
#include "../core/error.hpp"
#include "compute_device.hpp"
#include <optional>
#include <string>

namespace nodeflux::gpu {

/// @brief GPU-accelerated mesh generation for all primitive types
class GPUMeshGenerator {
public:
    /// @brief Initialize GPU mesh generation shaders
    /// @return True if initialization successful
    static bool initialize();
    
    /// @brief Shutdown and cleanup GPU resources
    static void shutdown();
    
    /// @brief Check if GPU mesh generation is available
    /// @return True if GPU compute is ready
    static bool is_available();
    
    /// @brief Generate sphere mesh on GPU
    /// @param radius Sphere radius
    /// @param u_segments Number of horizontal segments
    /// @param v_segments Number of vertical segments
    /// @return Generated mesh or nullopt on failure
    static std::optional<core::Mesh> generate_sphere(double radius, int u_segments, int v_segments);
    
    /// @brief Generate box mesh on GPU
    /// @param width Box width (X dimension)
    /// @param height Box height (Y dimension) 
    /// @param depth Box depth (Z dimension)
    /// @param width_segments Width subdivisions
    /// @param height_segments Height subdivisions
    /// @param depth_segments Depth subdivisions
    /// @return Generated mesh or nullopt on failure
    static std::optional<core::Mesh> generate_box(double width, double height, double depth,
                                                   int width_segments = 1, int height_segments = 1, int depth_segments = 1);
    
    /// @brief Generate cylinder mesh on GPU
    /// @param radius Cylinder radius
    /// @param height Cylinder height
    /// @param radial_segments Number of radial segments
    /// @param height_segments Number of height segments
    /// @param open_ended Whether cylinder should be open (no caps)
    /// @return Generated mesh or nullopt on failure
    static std::optional<core::Mesh> generate_cylinder(double radius, double height, 
                                                        int radial_segments, int height_segments = 1, 
                                                        bool open_ended = false);
    
    /// @brief Generate plane mesh on GPU
    /// @param width Plane width
    /// @param height Plane height
    /// @param width_segments Width subdivisions
    /// @param height_segments Height subdivisions
    /// @return Generated mesh or nullopt on failure
    static std::optional<core::Mesh> generate_plane(double width, double height, 
                                                     int width_segments, int height_segments);
    
    /// @brief Generate torus mesh on GPU
    /// @param major_radius Major radius (center to tube center)
    /// @param minor_radius Minor radius (tube radius)
    /// @param major_segments Number of segments around major radius
    /// @param minor_segments Number of segments around minor radius
    /// @return Generated mesh or nullopt on failure
    static std::optional<core::Mesh> generate_torus(double major_radius, double minor_radius,
                                                     int major_segments, int minor_segments);
    
    /// @brief Get performance statistics
    /// @return GPU generation timing info
    static std::string get_performance_stats();
    
    /// @brief Get last error
    /// @return Error information
    static const core::Error& last_error();

private:
    static bool initialized_;
    static std::unique_ptr<ComputeDevice::ComputeShader> sphere_shader_;
    static std::unique_ptr<ComputeDevice::ComputeShader> box_shader_;
    static std::unique_ptr<ComputeDevice::ComputeShader> cylinder_shader_;
    static std::unique_ptr<ComputeDevice::ComputeShader> plane_shader_;
    static std::unique_ptr<ComputeDevice::ComputeShader> torus_shader_;
    static thread_local core::Error last_error_;
    
    /// @brief Load and compile all compute shaders
    static bool load_shaders();
    
    /// @brief Set last error
    static void set_last_error(const core::Error& error);
    
    /// @brief Convert GPU buffer data to mesh
    static std::optional<core::Mesh> buffer_to_mesh(
        const ComputeDevice::Buffer& vertex_buffer, const ComputeDevice::Buffer& index_buffer,
        size_t num_vertices, size_t num_faces);
    
    /// @brief Get sphere compute shader source
    static std::string get_sphere_shader_source();
    
    /// @brief Get box compute shader source
    static std::string get_box_shader_source();
    
    /// @brief Get cylinder compute shader source  
    static std::string get_cylinder_shader_source();
    
    /// @brief Get plane compute shader source
    static std::string get_plane_shader_source();
    
    /// @brief Get torus compute shader source
    static std::string get_torus_shader_source();
};

} // namespace nodeflux::gpu
