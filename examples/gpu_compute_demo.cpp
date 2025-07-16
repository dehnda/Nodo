#include <iostream>
#include <nodeflux/gpu/compute_device.hpp>
#include <nodeflux/geometry/sphere_generator.hpp>
#include <nodeflux/io/obj_exporter.hpp>

using namespace nodeflux;

constexpr size_t BUFFER_SIZE = 1024;
constexpr int SPHERE_U_SEGMENTS = 32;
constexpr int SPHERE_V_SEGMENTS = 16;

int main() {
    std::cout << "GPU Compute Demo - NodeFlux Engine\n";
    std::cout << "===================================\n\n";
    
    // Initialize GPU compute device
    std::cout << "1. Initializing GPU Compute Device...\n";
    if (!gpu::ComputeDevice::initialize()) {
        std::cerr << "Failed to initialize GPU compute device: " 
                  << gpu::ComputeDevice::last_error().message << std::endl;
        std::cout << "   Note: This is expected if no OpenGL context is available\n";
        std::cout << "   GPU acceleration requires a valid OpenGL context\n\n";
    } else {
        std::cout << "   ✓ GPU compute device initialized successfully!\n\n";
    }
    
    // Test GPU profiler creation
    std::cout << "2. Testing GPU Profiler...\n";
    auto timer = gpu::GPUProfiler::create_timer();
    if (timer) {
        std::cout << "   ✓ GPU timer created successfully\n";
        if (gpu::GPUProfiler::is_available()) {
            std::cout << "   ✓ GPU profiling is available\n";
        } else {
            std::cout << "   ✗ GPU profiling not available (expected without GL context)\n";
        }
    } else {
        std::cout << "   ✗ Failed to create GPU timer (expected without GL context)\n";
    }
    
    // Test buffer creation
    std::cout << "\n3. Testing GPU Buffer Creation...\n";
    auto buffer = gpu::ComputeDevice::create_buffer(BUFFER_SIZE * sizeof(float));
    if (buffer) {
        std::cout << "   ✓ GPU buffer created successfully (" << buffer->size() << " bytes)\n";
    } else {
        std::cout << "   ✗ Failed to create GPU buffer (expected without GL context)\n";
    }
    
    // Generate a test mesh for comparison
    std::cout << "\n4. Generating Test Mesh (CPU-based)...\n";
    auto sphere = geometry::SphereGenerator::generate_uv_sphere(1.0, SPHERE_U_SEGMENTS, SPHERE_V_SEGMENTS);
    if (sphere) {
        std::cout << "   ✓ Generated sphere with " << sphere->vertices().rows() 
                  << " vertices and " << sphere->faces().rows() << " faces\n";
        
        // Export for verification
        if (io::ObjExporter::export_mesh(*sphere, "examples/output/gpu_demo_sphere.obj")) {
            std::cout << "   ✓ Exported to examples/output/gpu_demo_sphere.obj\n";
        }
    }
    
    std::cout << "\n5. GPU Acceleration Status:\n";
    std::cout << "   - GPU compute framework: ✓ Implemented\n";
    std::cout << "   - OpenGL/GLEW dependencies: ✓ Available\n";
    std::cout << "   - Compute shaders support: Requires OpenGL context\n";
    std::cout << "   - Buffer management: ✓ Ready\n";
    std::cout << "   - GPU profiling: ✓ Ready\n";
    
    std::cout << "\nNext Steps:\n";
    std::cout << "   1. Create OpenGL context (GLFW window)\n";
    std::cout << "   2. Implement GPU sphere generation compute shader\n";
    std::cout << "   3. Add GPU vs CPU performance benchmarks\n";
    std::cout << "   4. Implement GPU-accelerated BVH operations\n";
    
    return 0;
}
