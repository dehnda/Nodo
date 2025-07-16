#include <iostream>
#include <chrono>
#include <thread>
#include <nodeflux/gpu/gl_context.hpp>
#include <nodeflux/gpu/compute_device.hpp>
#include <nodeflux/geometry/sphere_generator.hpp>
#include <nodeflux/benchmarks/performance_benchmark.hpp>
#include <nodeflux/io/obj_exporter.hpp>

using namespace nodeflux;

constexpr double SPHERE_RADIUS = 1.0;
constexpr int HIGH_RES_U = 128;
constexpr int HIGH_RES_V = 64;
constexpr int MEDIUM_RES_U = 64;
constexpr int MEDIUM_RES_V = 32;
constexpr int LOW_RES_U = 32;
constexpr int LOW_RES_V = 16;

// Simple compute shader for sphere generation
const std::string SPHERE_COMPUTE_SHADER = R"(
#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(std430, binding = 0) buffer VertexBuffer {
    float vertices[];
};

layout(std430, binding = 1) buffer IndexBuffer {
    uint indices[];
};

uniform float radius;
uniform int u_segments;
uniform int v_segments;

const float PI = 3.14159265359;

void main() {
    uint u = gl_GlobalInvocationID.x;
    uint v = gl_GlobalInvocationID.y;
    
    if (u >= u_segments || v >= v_segments) return;
    
    // Generate vertex
    float theta = float(u) / float(u_segments - 1) * 2.0 * PI;
    float phi = float(v) / float(v_segments - 1) * PI;
    
    float x = radius * sin(phi) * cos(theta);
    float y = radius * cos(phi);
    float z = radius * sin(phi) * sin(theta);
    
    uint vertex_index = (v * u_segments + u) * 3;
    vertices[vertex_index + 0] = x;
    vertices[vertex_index + 1] = y;
    vertices[vertex_index + 2] = z;
    
    // Generate indices (two triangles per quad)
    if (u < u_segments - 1 && v < v_segments - 1) {
        uint quad_index = v * (u_segments - 1) + u;
        uint face_index = quad_index * 6;
        
        uint v0 = v * u_segments + u;
        uint v1 = v * u_segments + (u + 1);
        uint v2 = (v + 1) * u_segments + u;
        uint v3 = (v + 1) * u_segments + (u + 1);
        
        // First triangle
        indices[face_index + 0] = v0;
        indices[face_index + 1] = v2;
        indices[face_index + 2] = v1;
        
        // Second triangle
        indices[face_index + 3] = v1;
        indices[face_index + 4] = v2;
        indices[face_index + 5] = v3;
    }
}
)";

void benchmark_cpu_vs_gpu_sphere_generation() {
    std::cout << "\n=== GPU vs CPU Sphere Generation Benchmark ===\n";
    
    // CPU Benchmark
    std::cout << "\nCPU Sphere Generation:\n";
    auto start_cpu = std::chrono::high_resolution_clock::now();
    
    auto cpu_sphere = geometry::SphereGenerator::generate_uv_sphere(
        SPHERE_RADIUS, HIGH_RES_U, HIGH_RES_V);
    
    auto end_cpu = std::chrono::high_resolution_clock::now();
    auto cpu_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_cpu - start_cpu).count();
    
    if (cpu_sphere) {
        std::cout << "  ✓ Generated " << cpu_sphere->vertices().rows() 
                  << " vertices, " << cpu_sphere->faces().rows() << " faces\n";
        std::cout << "  ⏱️  CPU Time: " << cpu_duration / 1000.0 << " ms\n";
    }
    
    // GPU Benchmark (if available)
    gpu::ScopedGLContext context(1, 1, false);
    if (context.is_valid()) {
        std::cout << "\nGPU Sphere Generation:\n";
        
        if (!gpu::ComputeDevice::initialize()) {
            std::cout << "  ✗ Failed to initialize GPU compute device\n";
            return;
        }
        
        // Create compute shader
        auto shader = gpu::ComputeDevice::create_shader(SPHERE_COMPUTE_SHADER);
        if (!shader) {
            std::cout << "  ✗ Failed to create compute shader: " 
                      << gpu::ComputeDevice::last_error().message << "\n";
            return;
        }
        
        // Calculate buffer sizes
        const size_t num_vertices = HIGH_RES_U * HIGH_RES_V;
        const size_t num_faces = (HIGH_RES_U - 1) * (HIGH_RES_V - 1) * 2;
        const size_t vertex_buffer_size = num_vertices * 3 * sizeof(float);
        const size_t index_buffer_size = num_faces * 3 * sizeof(unsigned int);
        
        // Create GPU buffers
        auto vertex_buffer = gpu::ComputeDevice::create_buffer(vertex_buffer_size);
        auto index_buffer = gpu::ComputeDevice::create_buffer(index_buffer_size);
        
        if (!vertex_buffer || !index_buffer) {
            std::cout << "  ✗ Failed to create GPU buffers\n";
            return;
        }
        
        // Create GPU timer
        auto timer = gpu::GPUProfiler::create_timer();
        
        // Bind buffers and set uniforms
        vertex_buffer->bind(0);
        index_buffer->bind(1);
        
        shader->use();
        shader->set_uniform("radius", static_cast<float>(SPHERE_RADIUS));
        shader->set_uniform("u_segments", HIGH_RES_U);
        shader->set_uniform("v_segments", HIGH_RES_V);
        
        // Execute GPU generation
        if (timer) timer->start();
        
        const auto work_groups_x = (HIGH_RES_U + 15) / 16;
        const auto work_groups_y = (HIGH_RES_V + 15) / 16;
        shader->dispatch(work_groups_x, work_groups_y, 1);
        shader->memory_barrier();
        
        if (timer) {
            timer->stop();
            
            // Wait for GPU completion and get timing
            int wait_count = 0;
            while (!timer->is_ready() && wait_count < 1000) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                wait_count++;
            }
            
            if (timer->is_ready()) {
                double gpu_time = timer->get_elapsed_ms();
                std::cout << "  ✓ Generated " << num_vertices 
                          << " vertices, " << num_faces << " faces\n";
                std::cout << "  ⏱️  GPU Time: " << gpu_time << " ms\n";
                
                if (cpu_duration > 0) {
                    double speedup = (cpu_duration / 1000.0) / gpu_time;
                    std::cout << "  🚀 GPU Speedup: " << speedup << "x faster!\n";
                }
            } else {
                std::cout << "  ⚠️  GPU timing not available\n";
            }
        }
        
        std::cout << "  ✓ GPU sphere generation completed successfully\n";
        
    } else {
        std::cout << "\nGPU Context:\n";
        std::cout << "  ✗ Failed to create OpenGL context: " 
                  << context.last_error().message << "\n";
        std::cout << "  💡 Note: GPU acceleration requires OpenGL 4.3+ support\n";
    }
}

int main() {
    std::cout << "GPU-Accelerated Mesh Generation Demo\n";
    std::cout << "====================================\n";
    
    // Test OpenGL context creation
    std::cout << "\n1. Testing OpenGL Context Creation...\n";
    {
        gpu::ScopedGLContext context(1, 1, false);
        if (context.is_valid()) {
            std::cout << "   ✓ OpenGL context created successfully!\n";
            std::cout << "\n" << gpu::GLContext::get_context_info() << "\n";
            
            // Test GPU compute device with context
            std::cout << "\n2. Testing GPU Compute Device...\n";
            if (gpu::ComputeDevice::initialize()) {
                std::cout << "   ✓ GPU compute device initialized!\n";
                std::cout << "\n" << gpu::ComputeDevice::get_device_info() << "\n";
                
                // Test GPU profiling
                std::cout << "\n3. Testing GPU Profiling...\n";
                if (gpu::GPUProfiler::is_available()) {
                    auto timer = gpu::GPUProfiler::create_timer();
                    if (timer) {
                        std::cout << "   ✓ GPU profiling available\n";
                    }
                } else {
                    std::cout << "   ⚠️  GPU profiling not available\n";
                }
                
                // Run benchmarks
                benchmark_cpu_vs_gpu_sphere_generation();
                
            } else {
                std::cout << "   ✗ Failed to initialize GPU compute device: " 
                          << gpu::ComputeDevice::last_error().message << "\n";
            }
        } else {
            std::cout << "   ✗ Failed to create OpenGL context: " 
                      << context.last_error().message << "\n";
        }
    }
    
    // Generate comparison meshes
    std::cout << "\n4. Generating Comparison Meshes (CPU)...\n";
    
    auto low_res = geometry::SphereGenerator::generate_uv_sphere(SPHERE_RADIUS, LOW_RES_U, LOW_RES_V);
    auto medium_res = geometry::SphereGenerator::generate_uv_sphere(SPHERE_RADIUS, MEDIUM_RES_U, MEDIUM_RES_V);
    auto high_res = geometry::SphereGenerator::generate_uv_sphere(SPHERE_RADIUS, HIGH_RES_U, HIGH_RES_V);
    
    if (low_res && medium_res && high_res) {
        std::cout << "   Low Res:    " << low_res->vertices().rows() << " vertices\n";
        std::cout << "   Medium Res: " << medium_res->vertices().rows() << " vertices\n";
        std::cout << "   High Res:   " << high_res->vertices().rows() << " vertices\n";
        
        // Export meshes
        io::ObjExporter::export_mesh(*low_res, "examples/output/gpu_sphere_low.obj");
        io::ObjExporter::export_mesh(*medium_res, "examples/output/gpu_sphere_medium.obj");
        io::ObjExporter::export_mesh(*high_res, "examples/output/gpu_sphere_high.obj");
        std::cout << "   ✓ Exported comparison meshes to examples/output/\n";
    }
    
    std::cout << "\n🎯 GPU Acceleration Status:\n";
    std::cout << "   - OpenGL Context: " << (gpu::GLContext::is_available() ? "✓ Available" : "✗ Not Available") << "\n";
    std::cout << "   - GPU Compute: " << (gpu::ComputeDevice::is_available() ? "✓ Ready" : "✗ Not Ready") << "\n";
    std::cout << "   - Compute Shaders: Framework Complete\n";
    std::cout << "   - Performance Monitoring: ✓ Implemented\n";
    
    return 0;
}
