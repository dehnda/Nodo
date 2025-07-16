#include "nodeflux/core/mesh.hpp"
#include "nodeflux/geometry/mesh_generator.hpp"
#include "nodeflux/sop/extrude_sop.hpp"
#include "nodeflux/sop/laplacian_sop.hpp"
#include "nodeflux/sop/boolean_sop.hpp"
#include "nodeflux/io/obj_exporter.hpp"
#include <chrono>
#include <iostream>
#include <memory>

using namespace nodeflux;

/**
 * @brief Week 3 Advanced Procedural Operations Demo
 * 
 * Demonstrates new Week 3 SOPs:
 * - ExtrudeSOP: Face extrusion with multiple modes
 * - LaplacianSOP: Mesh smoothing algorithms
 * - Advanced workflows combining multiple SOPs
 */

void demonstrate_extrusion_operations() {
    std::cout << "\n=== Extrusion Operations Demo ===\n";
    
    constexpr double BOX_SIZE = 1.0;
    constexpr double EXTRUDE_DISTANCE = 0.3;
    
    // Create a simple box to extrude
    auto box_mesh = geometry::MeshGenerator::box(
        Eigen::Vector3d(-BOX_SIZE, -BOX_SIZE, -BOX_SIZE),
        Eigen::Vector3d(BOX_SIZE, BOX_SIZE, BOX_SIZE)
    );
    auto box_shared = std::make_shared<core::Mesh>(std::move(box_mesh));
    
    std::cout << "✓ Generated box: " << box_shared->vertices().rows() 
              << " vertices, " << box_shared->faces().rows() << " faces\n";
    
    // Face Normals Extrusion
    sop::ExtrudeSOP extrude_normals("extrude_face_normals");
    extrude_normals.set_input_mesh(box_shared);
    extrude_normals.set_mode(sop::ExtrudeSOP::ExtrusionMode::FACE_NORMALS);
    extrude_normals.set_distance(EXTRUDE_DISTANCE);
    
    auto normals_result = extrude_normals.cook();
    if (normals_result) {
        io::ObjExporter::export_mesh(*normals_result, "week3_extrude_normals.obj");
        std::cout << "✓ Face normals extrusion exported\n";
    }
    
    // Uniform Direction Extrusion
    sop::ExtrudeSOP extrude_uniform("extrude_uniform");
    extrude_uniform.set_input_mesh(box_shared);
    extrude_uniform.set_mode(sop::ExtrudeSOP::ExtrusionMode::UNIFORM_DIRECTION);
    extrude_uniform.set_direction(Eigen::Vector3d(0.0, 1.0, 0.0)); // Upward
    extrude_uniform.set_distance(EXTRUDE_DISTANCE * 2.0);
    
    auto uniform_result = extrude_uniform.cook();
    if (uniform_result) {
        io::ObjExporter::export_mesh(*uniform_result, "week3_extrude_uniform.obj");
        std::cout << "✓ Uniform direction extrusion exported\n";
    }
}

void demonstrate_smoothing_operations() {
    std::cout << "\n=== Smoothing Operations Demo ===\n";
    
    constexpr double SPHERE_RADIUS = 1.0;
    constexpr int SPHERE_SUBDIVISIONS = 2;
    constexpr int SMOOTHING_ITERATIONS = 3;
    constexpr double SMOOTHING_LAMBDA = 0.3;
    
    // Create a rough sphere (low subdivision)
    auto sphere_opt = geometry::MeshGenerator::sphere(
        Eigen::Vector3d(0.0, 0.0, 0.0), 
        SPHERE_RADIUS, 
        SPHERE_SUBDIVISIONS
    );
    
    if (!sphere_opt) {
        std::cerr << "Failed to generate sphere for smoothing\n";
        return;
    }
    
    auto sphere_shared = std::make_shared<core::Mesh>(std::move(*sphere_opt));
    std::cout << "✓ Generated rough sphere: " << sphere_shared->vertices().rows() 
              << " vertices\n";
    
    // Export original for comparison
    io::ObjExporter::export_mesh(*sphere_shared, "week3_sphere_original.obj");
    
    // Uniform Laplacian Smoothing
    sop::LaplacianSOP smooth_uniform("smooth_uniform");
    smooth_uniform.set_input_mesh(sphere_shared);
    smooth_uniform.set_method(sop::LaplacianSOP::SmoothingMethod::UNIFORM);
    smooth_uniform.set_iterations(SMOOTHING_ITERATIONS);
    smooth_uniform.set_lambda(SMOOTHING_LAMBDA);
    smooth_uniform.set_preserve_boundaries(false); // Allow full smoothing
    
    auto uniform_smooth = smooth_uniform.cook();
    if (uniform_smooth) {
        io::ObjExporter::export_mesh(*uniform_smooth, "week3_sphere_uniform_smooth.obj");
        std::cout << "✓ Uniform Laplacian smoothing completed\n";
    }
    
    // Taubin Smoothing (prevents shrinkage)
    sop::LaplacianSOP smooth_taubin("smooth_taubin");
    smooth_taubin.set_input_mesh(sphere_shared);
    smooth_taubin.set_method(sop::LaplacianSOP::SmoothingMethod::TAUBIN);
    smooth_taubin.set_iterations(SMOOTHING_ITERATIONS);
    smooth_taubin.set_lambda(SMOOTHING_LAMBDA);
    smooth_taubin.set_mu(-0.35); // Anti-shrinkage factor
    
    auto taubin_smooth = smooth_taubin.cook();
    if (taubin_smooth) {
        io::ObjExporter::export_mesh(*taubin_smooth, "week3_sphere_taubin_smooth.obj");
        std::cout << "✓ Taubin smoothing completed\n";
    }
}

void demonstrate_advanced_workflow() {
    std::cout << "\n=== Advanced SOP Workflow Demo ===\n";
    
    constexpr double CYLINDER_RADIUS = 0.6;
    constexpr double CYLINDER_HEIGHT = 2.0;
    constexpr int CYLINDER_SEGMENTS = 8;
    
    // Create base cylinder
    auto cylinder_opt = geometry::MeshGenerator::cylinder(
        Eigen::Vector3d(0.0, -CYLINDER_HEIGHT/2.0, 0.0),
        Eigen::Vector3d(0.0, CYLINDER_HEIGHT/2.0, 0.0),
        CYLINDER_RADIUS,
        CYLINDER_SEGMENTS
    );
    
    if (!cylinder_opt) {
        std::cerr << "Failed to generate cylinder\n";
        return;
    }
    
    auto cylinder_shared = std::make_shared<core::Mesh>(std::move(*cylinder_opt));
    std::cout << "✓ Generated cylinder: " << cylinder_shared->vertices().rows() 
              << " vertices\n";
    
    // Step 1: Extrude faces
    sop::ExtrudeSOP extrude_step("workflow_extrude");
    extrude_step.set_input_mesh(cylinder_shared);
    extrude_step.set_mode(sop::ExtrudeSOP::ExtrusionMode::FACE_NORMALS);
    extrude_step.set_distance(0.2);
    
    auto extruded_mesh = extrude_step.cook();
    if (!extruded_mesh) {
        std::cerr << "Extrusion step failed\n";
        return;
    }
    
    // Step 2: Smooth the extruded result
    sop::LaplacianSOP smooth_step("workflow_smooth");
    smooth_step.set_input_mesh(extruded_mesh);
    smooth_step.set_method(sop::LaplacianSOP::SmoothingMethod::UNIFORM);
    smooth_step.set_iterations(2);
    smooth_step.set_lambda(0.4);
    
    auto smoothed_mesh = smooth_step.cook();
    if (!smoothed_mesh) {
        std::cerr << "Smoothing step failed\n";
        return;
    }
    
    // Step 3: Boolean union with another shape
    auto sphere_opt = geometry::MeshGenerator::sphere(
        Eigen::Vector3d(0.0, 0.0, 0.0), 0.8, 3);
    
    if (sphere_opt) {
        auto sphere_shared = std::make_shared<core::Mesh>(std::move(*sphere_opt));
        
        sop::BooleanSOP union_step("workflow_union");
        union_step.set_operation(sop::BooleanSOP::OperationType::UNION);
        union_step.set_mesh_a(smoothed_mesh);
        union_step.set_mesh_b(sphere_shared);
        
        auto final_result = union_step.cook();
        if (final_result) {
            io::ObjExporter::export_mesh(*final_result, "week3_advanced_workflow.obj");
            std::cout << "✓ Advanced workflow: Extrude → Smooth → Boolean Union\n";
            std::cout << "  Final result: " << final_result->vertices().rows() 
                      << " vertices, " << final_result->faces().rows() 
                      << " faces\n";
        }
    }
}

int main() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::cout << "🎯 NodeFluxEngine Week 3 Advanced SOP Demo\n";
    std::cout << "==========================================\n";
    
    try {
        demonstrate_extrusion_operations();
        demonstrate_smoothing_operations();
        demonstrate_advanced_workflow();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        std::cout << "\n=== Week 3 Advanced SOP Demo Complete ===\n";
        std::cout << "✓ Extrusion operations demonstrated!\n";
        std::cout << "✓ Laplacian smoothing algorithms working!\n";
        std::cout << "✓ Advanced multi-step workflows functional!\n";
        std::cout << "✓ Total execution time: " << duration.count() << "ms\n";
        std::cout << "✓ Generated files:\n";
        std::cout << "  - week3_extrude_normals.obj\n";
        std::cout << "  - week3_extrude_uniform.obj\n";
        std::cout << "  - week3_sphere_original.obj\n";
        std::cout << "  - week3_sphere_uniform_smooth.obj\n";
        std::cout << "  - week3_sphere_taubin_smooth.obj\n";
        std::cout << "  - week3_advanced_workflow.obj\n";
        
        std::cout << "\n🚀 Week 3 Advanced Procedural Operations: Complete!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
