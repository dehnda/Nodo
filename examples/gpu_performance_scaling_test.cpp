#include "../include/nodeflux/gpu/gpu_mesh_generator.hpp"
#include "../include/nodeflux/gpu/compute_device.hpp"
#include "../include/nodeflux/gpu/gl_context.hpp"
#include "../include/nodeflux/io/obj_exporter.hpp"
#include "../include/nodeflux/geometry/sphere_generator.hpp"
#include <iostream>
#include <chrono>

using namespace nodeflux;

int main() {
    std::cout << "\n🔥 NodeFlux GPU Large Mesh Performance Test\n";
    std::cout << "==========================================\n\n";
    
    // Initialize GPU
    if (!gpu::GLContext::initialize() || !gpu::ComputeDevice::initialize() || !gpu::GPUMeshGenerator::initialize()) {
        std::cerr << "❌ Failed to initialize GPU systems\n";
        return 1;
    }
    
    std::cout << "✅ GPU systems ready\n\n";
    
    // Test with very large meshes where GPU should dominate
    const std::vector<std::pair<int, std::string>> test_sizes = {
        {32, "Small (32x32)"},
        {64, "Medium (64x64)"},
        {128, "Large (128x128)"},
        {256, "XLarge (256x256)"},
        {512, "XXLarge (512x512)"}
    };
    
    std::cout << "🏃‍♂️ Sphere Generation Performance Scaling\n";
    std::cout << "==========================================\n\n";
    
    for (const auto& [segments, description] : test_sizes) {
        std::cout << description << " - " << segments*segments << " vertices expected:\n";
        
        // CPU timing
        auto cpu_start = std::chrono::high_resolution_clock::now();
        auto cpu_sphere = geometry::SphereGenerator::generate_uv_sphere(1.0, segments, segments);
        auto cpu_end = std::chrono::high_resolution_clock::now();
        auto cpu_time = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();
        
        // GPU timing
        auto gpu_start = std::chrono::high_resolution_clock::now();
        auto gpu_sphere = gpu::GPUMeshGenerator::generate_sphere(1.0, segments, segments);
        auto gpu_end = std::chrono::high_resolution_clock::now();
        auto gpu_time = std::chrono::duration<double, std::milli>(gpu_end - gpu_start).count();
        
        if (cpu_sphere.has_value() && gpu_sphere.has_value()) {
            const double speedup = (gpu_time > 0) ? cpu_time / gpu_time : std::numeric_limits<double>::infinity();
            const auto cpu_verts = cpu_sphere->vertices().rows();
            const auto gpu_verts = gpu_sphere->vertices().rows();
            
            std::cout << "   CPU: " << cpu_time << " ms (" << cpu_verts << " vertices)\n";
            std::cout << "   GPU: " << gpu_time << " ms (" << gpu_verts << " vertices)\n";
            std::cout << "   🏃‍♂️ Speedup: " << speedup << "x";
            
            if (speedup > 1.0) {
                std::cout << " 🚀 GPU WINS!";
            } else if (speedup > 0.5) {
                std::cout << " ⚡ Getting closer...";
            } else {
                std::cout << " 🐢 CPU still faster";
            }
            std::cout << "\n\n";
            
            // Export largest successful mesh
            if (segments >= 128) {
                const std::string filename = "gpu_sphere_" + std::to_string(segments) + ".obj";
                io::ObjExporter::export_mesh(*gpu_sphere, filename);
                std::cout << "   💾 Exported: " << filename << "\n\n";
            }
        } else {
            std::cout << "   ❌ Generation failed\n\n";
        }
        
        // Early exit if GPU outperforms significantly
        if (cpu_sphere.has_value() && gpu_sphere.has_value() && cpu_time / gpu_time > 2.0) {
            std::cout << "🎉 GPU achieved significant speedup! Continuing with remaining tests...\n\n";
        }
    }
    
    std::cout << "📈 Performance Summary\n";
    std::cout << "=====================\n";
    std::cout << "GPU compute shaders excel at highly parallel workloads.\n";
    std::cout << "For small meshes, CPU overhead dominates.\n";
    std::cout << "For large meshes (>10k vertices), GPU should provide significant speedups.\n\n";
    
    // Cleanup
    gpu::GPUMeshGenerator::shutdown();
    gpu::ComputeDevice::shutdown();
    gpu::GLContext::shutdown();
    
    return 0;
}
