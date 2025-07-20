#include "nodeflux/core/mesh.hpp"
#include "nodeflux/gpu/compute_device.hpp"
#include "nodeflux/gpu/gl_context.hpp"
#include "nodeflux/gpu/gpu_mesh_generator.hpp"
#include "nodeflux/io/obj_exporter.hpp"
#include "nodeflux/sop/array_sop.hpp"
#include "nodeflux/sop/noise_displacement_sop.hpp"
#include "nodeflux/sop/subdivisions_sop.hpp"
#include <iostream>

using namespace nodeflux;

// Demo constants
constexpr float DEFAULT_SPHERE_RADIUS = 1.0F;
constexpr int DEFAULT_SPHERE_SUBDIVISIONS = 32;
constexpr int DEFAULT_SPHERE_RINGS = 16;
constexpr float LINEAR_ARRAY_OFFSET = 3.0F;
constexpr int RADIAL_ARRAY_COUNT = 6;
constexpr float RADIAL_ARRAY_RADIUS = 4.0F;
constexpr float RADIAL_ARRAY_ANGLE_STEP = 60.0F;
constexpr float NOISE_AMPLITUDE = 0.3F;
constexpr float NOISE_FREQUENCY = 2.0F;
constexpr int NOISE_SEED = 42;

/**
 * @brief Demonstration of the new header-based SOP system
 *
 * Shows ArraySOP, NoiseDisplacementSOP, and SubdivisionSOP as reusable
 * components
 */
int main() {
  std::cout << "=== NodeFluxEngine: Header-Based SOP System Demo ===\n\n";

  // Initialize GPU systems
  std::cout << "Initializing GPU systems...\n";
  if (!gpu::GLContext::initialize()) {
    std::cerr << "❌ Failed to initialize OpenGL context\n";
    return 1;
  }

  if (!gpu::ComputeDevice::initialize()) {
    std::cerr << "❌ Failed to initialize GPU compute device\n";
    return 1;
  }

  if (!gpu::GPUMeshGenerator::initialize()) {
    std::cerr << "❌ Failed to initialize GPU mesh generator\n";
    return 1;
  }

  std::cout << "✅ All GPU systems ready!\n\n";

  try {
    // Generate base geometry
    std::cout << "=== Generating Base Geometry ===\n";
    auto sphere_opt = gpu::GPUMeshGenerator::generate_sphere(
        DEFAULT_SPHERE_RADIUS, DEFAULT_SPHERE_SUBDIVISIONS,
        DEFAULT_SPHERE_RINGS);
    if (!sphere_opt) {
      std::cerr << "❌ Failed to generate sphere\n";
      return 1;
    }

    auto sphere_mesh = core::Mesh(std::move(*sphere_opt));
    std::cout << "✓ Generated sphere: " << sphere_mesh.vertices().rows()
              << " vertices, " << sphere_mesh.faces().rows() << " faces\n";

    // === ArraySOP Demo ===
    std::cout << "\n=== ArraySOP Demonstration ===\n";

    // Linear array
    sop::ArraySOP linear_array("linear_array");
    linear_array.set_array_type(sop::ArraySOP::ArrayType::LINEAR);
    linear_array.set_count(4);
    linear_array.set_linear_offset(
        core::Vector3(LINEAR_ARRAY_OFFSET, 0.0F, 0.0F));

    auto linear_result = linear_array.process(sphere_mesh);
    if (linear_result) {
      std::cout << "✓ Linear array: " << linear_result->vertices().rows()
                << " vertices, " << linear_result->faces().rows() << " faces\n";

      io::ObjExporter exporter;
      io::ObjExporter::export_mesh(*linear_result, "sop_linear_array.obj");
      std::cout << "  Exported: sop_linear_array.obj\n";
    }

    // Radial array
    sop::ArraySOP radial_array("radial_array");
    radial_array.set_array_type(sop::ArraySOP::ArrayType::RADIAL);
    radial_array.set_count(RADIAL_ARRAY_COUNT);
    radial_array.set_radial_radius(RADIAL_ARRAY_RADIUS);
    radial_array.set_angle_step(RADIAL_ARRAY_ANGLE_STEP);

    auto radial_result = radial_array.process(sphere_mesh);
    if (radial_result) {
      std::cout << "✓ Radial array: " << radial_result->vertices().rows()
                << " vertices, " << radial_result->faces().rows() << " faces\n";

      io::ObjExporter exporter;
      io::ObjExporter::export_mesh(*radial_result, "sop_radial_array.obj");
      std::cout << "  Exported: sop_radial_array.obj\n";
    }

    // === NoiseDisplacementSOP Demo ===
    std::cout << "\n=== NoiseDisplacementSOP Demonstration ===\n";

    sop::NoiseDisplacementSOP noise_displacement("noise_disp");
    noise_displacement.set_amplitude(NOISE_AMPLITUDE);
    noise_displacement.set_frequency(NOISE_FREQUENCY);
    noise_displacement.set_octaves(4);
    noise_displacement.set_seed(NOISE_SEED);

    auto noise_result = noise_displacement.process(sphere_mesh);
    if (noise_result) {
      std::cout << "✓ Noise displacement: " << noise_result->vertices().rows()
                << " vertices, " << noise_result->faces().rows() << " faces\n";

      io::ObjExporter exporter;
      io::ObjExporter::export_mesh(*noise_result, "sop_noise_displacement.obj");
      std::cout << "  Exported: sop_noise_displacement.obj\n";
    }

    // === SubdivisionSOP Demo ===
    std::cout << "\n=== SubdivisionSOP Demonstration ===\n";

    sop::SubdivisionSOP subdivision("subdivision");
    subdivision.set_subdivision_levels(2);
    subdivision.set_preserve_boundaries(true);

    auto subdivision_result = subdivision.process(sphere_mesh);
    if (subdivision_result) {
      std::cout << "✓ Subdivision: " << subdivision_result->vertices().rows()
                << " vertices, " << subdivision_result->faces().rows()
                << " faces\n";

      io::ObjExporter exporter;
      io::ObjExporter::export_mesh(*subdivision_result, "sop_subdivision.obj");
      std::cout << "  Exported: sop_subdivision.obj\n";
    }

    // === Complex Pipeline Demo ===
    std::cout << "\n=== Complex Pipeline: Noise -> Subdivision -> Array ===\n";

    // Apply noise displacement
    auto pipeline_mesh = sphere_mesh;
    auto step1 = noise_displacement.process(pipeline_mesh);
    if (!step1) {
      std::cerr << "❌ Pipeline step 1 (noise) failed\n";
      return 1;
    }

    // Apply subdivision
    auto step2 = subdivision.process(*step1);
    if (!step2) {
      std::cerr << "❌ Pipeline step 2 (subdivision) failed\n";
      return 1;
    }

    // Apply linear array
    auto step3 = linear_array.process(*step2);
    if (!step3) {
      std::cerr << "❌ Pipeline step 3 (array) failed\n";
      return 1;
    }

    std::cout << "✓ Complex pipeline result: " << step3->vertices().rows()
              << " vertices, " << step3->faces().rows() << " faces\n";

    io::ObjExporter exporter;
    io::ObjExporter::export_mesh(*step3, "sop_complex_pipeline.obj");
    std::cout << "  Exported: sop_complex_pipeline.obj\n";

    std::cout << "\n=== Header-Based SOP Demo Completed Successfully ===\n";
    std::cout << "Generated Files:\n";
    std::cout << "• sop_linear_array.obj - Linear array pattern\n";
    std::cout << "• sop_radial_array.obj - Radial array pattern\n";
    std::cout << "• sop_noise_displacement.obj - Fractal noise displacement\n";
    std::cout << "• sop_subdivision.obj - Subdivision surfaces\n";
    std::cout << "• sop_complex_pipeline.obj - Noise->Subdivision->Array "
                 "pipeline\n\n";

    std::cout << "Key Achievements:\n";
    std::cout << "✓ Reusable SOP header/implementation files\n";
    std::cout << "✓ Clean separation of concerns\n";
    std::cout << "✓ Pipeline composition with multiple SOPs\n";
    std::cout << "✓ Production-ready API design\n";
    std::cout << "✓ Integrated with existing GPU acceleration\n";

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  // Clean up GPU systems
  gpu::GPUMeshGenerator::shutdown();
  gpu::ComputeDevice::shutdown();
  gpu::GLContext::shutdown();

  return 0;
}
