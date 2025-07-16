#pragma once

#include "../core/mesh.hpp"
#include "../core/error.hpp"
#include <GL/glew.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace nodeflux::gpu {

/// @brief GPU compute device abstraction
class ComputeDevice {
public:
    /// @brief GPU buffer for storing mesh data
    class Buffer {
    public:
        Buffer(size_t size, GLenum usage = GL_STATIC_DRAW);
        ~Buffer();
        
        /// @brief Upload data to GPU buffer
        /// @param data Pointer to data
        /// @param size Size in bytes
        void upload(const void* data, size_t size);
        
        /// @brief Download data from GPU buffer
        /// @param data Pointer to destination
        /// @param size Size in bytes
        void download(void* data, size_t size);
        
        /// @brief Bind buffer to binding point
        /// @param binding_point Shader storage buffer binding point
        void bind(GLuint binding_point);
        
        /// @brief Get buffer size
        /// @return Size in bytes
        size_t size() const { return size_; }
        
        /// @brief Get OpenGL buffer ID
        /// @return Buffer ID
        GLuint id() const { return buffer_id_; }
        
    private:
        GLuint buffer_id_;
        size_t size_;
        GLenum usage_;
    };
    
    /// @brief Compute shader program
    class ComputeShader {
    public:
        ComputeShader(const std::string& source);
        ~ComputeShader();
        
        /// @brief Compile the compute shader
        /// @return True if compilation successful
        bool compile();
        
        /// @brief Use this shader program
        void use();
        
        /// @brief Dispatch compute work groups
        /// @param num_groups_x Number of work groups in X dimension
        /// @param num_groups_y Number of work groups in Y dimension  
        /// @param num_groups_z Number of work groups in Z dimension
        void dispatch(GLuint num_groups_x, GLuint num_groups_y = 1, GLuint num_groups_z = 1);
        
        /// @brief Wait for compute shader to complete
        static void memory_barrier();
        
        /// @brief Set uniform values
        void set_uniform(const std::string& name, int value);
        void set_uniform(const std::string& name, float value);
        void set_uniform(const std::string& name, const std::vector<float>& values);
        
        /// @brief Get compilation error log
        /// @return Error message
        std::string get_error_log() const { return error_log_; }
        
        /// @brief Check if shader is valid
        /// @return True if compiled successfully
        bool is_valid() const { return program_id_ != 0; }
        
    private:
        GLuint program_id_;
        GLuint shader_id_;
        std::string source_;
        std::string error_log_;
        std::unordered_map<std::string, GLint> uniform_locations_;
        
        GLint get_uniform_location(const std::string& name);
    };

public:
    /// @brief Initialize GPU compute device
    /// @return True if initialization successful
    static bool initialize();
    
    /// @brief Shutdown GPU compute device
    static void shutdown();
    
    /// @brief Check if GPU compute is available
    /// @return True if compute shaders are supported
    static bool is_available();
    
    /// @brief Get device information
    /// @return Device info string
    static std::string get_device_info();
    
    /// @brief Get maximum work group size
    /// @return Maximum work group dimensions
    static std::vector<int> get_max_work_group_size();
    
    /// @brief Get maximum work group invocations
    /// @return Maximum total invocations per work group
    static int get_max_work_group_invocations();
    
    /// @brief Create a compute buffer
    /// @param size Size in bytes
    /// @param usage Buffer usage pattern
    /// @return Unique pointer to buffer
    static std::unique_ptr<Buffer> create_buffer(size_t size, GLenum usage = GL_STATIC_DRAW);
    
    /// @brief Create a compute shader
    /// @param source GLSL compute shader source
    /// @return Unique pointer to shader
    static std::unique_ptr<ComputeShader> create_shader(const std::string& source);
    
    /// @brief Load compute shader from file
    /// @param filename Path to shader file
    /// @return Unique pointer to shader
    static std::unique_ptr<ComputeShader> load_shader_from_file(const std::string& filename);
    
    /// @brief Get last error
    /// @return Error information
    static const core::Error& last_error();

private:
    static bool initialized_;
    static thread_local core::Error last_error_;
    
    static void set_last_error(const core::Error& error);
    static bool check_gl_error(const std::string& operation);
};

/// @brief GPU-accelerated mesh generation
class GPUMeshGenerator {
public:
    /// @brief Generate sphere mesh on GPU
    /// @param center Sphere center
    /// @param radius Sphere radius
    /// @param subdivisions Number of subdivisions
    /// @return Generated mesh or nullopt on failure
    static std::optional<core::Mesh> gpu_sphere(
        const Eigen::Vector3d& center,
        double radius,
        int subdivisions
    );
    
    /// @brief Generate box mesh on GPU
    /// @param min_corner Minimum corner
    /// @param max_corner Maximum corner
    /// @return Generated mesh
    static core::Mesh gpu_box(
        const Eigen::Vector3d& min_corner,
        const Eigen::Vector3d& max_corner
    );
    
    /// @brief Generate cylinder mesh on GPU
    /// @param bottom_center Bottom center
    /// @param top_center Top center
    /// @param radius Cylinder radius
    /// @param segments Number of segments
    /// @return Generated mesh or nullopt on failure
    static std::optional<core::Mesh> gpu_cylinder(
        const Eigen::Vector3d& bottom_center,
        const Eigen::Vector3d& top_center,
        double radius,
        int segments
    );
    
    /// @brief Subdivide mesh on GPU
    /// @param mesh Input mesh
    /// @param levels Number of subdivision levels
    /// @return Subdivided mesh or nullopt on failure
    static std::optional<core::Mesh> gpu_subdivide(
        const core::Mesh& mesh,
        int levels
    );
    
    /// @brief Check if GPU mesh generation is available
    /// @return True if GPU is available and initialized
    static bool is_available();

private:
    static std::unique_ptr<ComputeDevice::ComputeShader> sphere_shader_;
    static std::unique_ptr<ComputeDevice::ComputeShader> box_shader_;
    static std::unique_ptr<ComputeDevice::ComputeShader> subdivision_shader_;
    
    /// @brief Initialize GPU shaders
    /// @return True if initialization successful
    static bool initialize_shaders();
    
    /// @brief Get sphere generation compute shader source
    /// @return GLSL source code
    static std::string get_sphere_shader_source();
    
    /// @brief Get box generation compute shader source
    /// @return GLSL source code
    static std::string get_box_shader_source();
};

/// @brief Performance monitoring for GPU operations
class GPUProfiler {
public:
    /// @brief GPU timing query
    class Timer {
    public:
        Timer();
        ~Timer();
        
        /// @brief Start timing
        void start();
        
        /// @brief Stop timing
        void stop();
        
        /// @brief Get elapsed time in milliseconds
        /// @return Elapsed time, or -1 if not ready
        double get_elapsed_ms();
        
        /// @brief Check if result is available
        /// @return True if timing result is ready
        bool is_ready();
        
    private:
        GLuint query_ids_[2]; // start and stop queries
        bool timing_active_;
    };
    
    /// @brief Create a GPU timer
    /// @return Unique pointer to timer
    static std::unique_ptr<Timer> create_timer();
    
    /// @brief Check if GPU profiling is available
    /// @return True if timer queries are supported
    static bool is_available();
};

} // namespace nodeflux::gpu
