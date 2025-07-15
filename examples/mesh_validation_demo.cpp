#include <iostream>
#include <nodeflux/core/mesh.hpp>
#include <nodeflux/geometry/box_generator.hpp>
#include <nodeflux/geometry/sphere_generator.hpp>
#include <nodeflux/geometry/boolean_ops.hpp>
#include <nodeflux/geometry/mesh_validator.hpp>
#include <nodeflux/geometry/mesh_repairer.hpp>
#include <nodeflux/io/obj_exporter.hpp>

using namespace nodeflux;

// Constants for mesh generation
constexpr int VERTEX_COUNT = 6;
constexpr int FACE_COUNT = 3;
constexpr double HALF_UNIT = 0.5;
constexpr double TWO_UNITS = 2.0;
constexpr int VERTEX_INDEX_5 = 5;
constexpr double BOX_SIZE = 2.0;
constexpr double SPHERE1_RADIUS = 1.0;
constexpr double SPHERE2_RADIUS = 0.8;
constexpr int UV_SUBDIVISIONS_16 = 16;
constexpr int UV_SUBDIVISIONS_12 = 12;
constexpr double TRANSLATION_OFFSET = 0.5;
constexpr int ICOSPHERE_SUBDIVISIONS = 2;

void create_problematic_mesh(core::Mesh& mesh) {
    // Create a mesh with known issues for testing
    
    Eigen::MatrixXd vertices(VERTEX_COUNT, 3);
    vertices << 
        0.0, 0.0, 0.0,   // v0
        1.0, 0.0, 0.0,   // v1
        HALF_UNIT, 1.0, 0.0,   // v2
        0.0, 0.0, 0.0,   // v3 (duplicate of v0)
        TWO_UNITS, 0.0, 0.0,   // v4 (unreferenced)
        1.0, 0.0, 0.0;   // v5 (duplicate of v1)
    
    Eigen::MatrixXi faces(FACE_COUNT, 3);
    faces <<
        0, 1, 2,  // valid triangle
        0, 1, 1,  // degenerate triangle (duplicate vertex)
        3, VERTEX_INDEX_5, 2;  // triangle using duplicate vertices
    
    mesh.vertices() = vertices;
    mesh.faces() = faces;
}

void test_validation_system() {
    std::cout << "\n=== Testing Mesh Validation System ===\n";
    
    // Test with a clean mesh first
    std::cout << "\n1. Testing with clean box mesh:\n";
    auto box_result = geometry::BoxGenerator::generate(BOX_SIZE, BOX_SIZE, BOX_SIZE);
    if (box_result) {
        auto box_validation = geometry::MeshValidator::validate(*box_result);
        std::cout << box_validation.summary() << "\n";
    }
    
    // Test with a problematic mesh
    std::cout << "\n2. Testing with problematic mesh:\n";
    core::Mesh problematic_mesh;
    create_problematic_mesh(problematic_mesh);
    
    auto validation = geometry::MeshValidator::validate(problematic_mesh);
    std::cout << validation.summary() << "\n";
    
    // Test specific validation functions
    std::cout << "\n3. Detailed validation tests:\n";
    
    auto degenerate_faces = geometry::MeshValidator::find_degenerate_faces(problematic_mesh);
    std::cout << "Degenerate faces found: " << degenerate_faces.size() << "\n";
    for (int face_idx : degenerate_faces) {
        std::cout << "  Face " << face_idx << ": [" 
                  << problematic_mesh.faces()(face_idx, 0) << ", "
                  << problematic_mesh.faces()(face_idx, 1) << ", "
                  << problematic_mesh.faces()(face_idx, 2) << "]\n";
    }
    
    auto unreferenced = geometry::MeshValidator::find_unreferenced_vertices(problematic_mesh);
    std::cout << "Unreferenced vertices: " << unreferenced.size() << "\n";
    for (int vertex_idx : unreferenced) {
        std::cout << "  Vertex " << vertex_idx << ": ["
                  << problematic_mesh.vertices()(vertex_idx, 0) << ", "
                  << problematic_mesh.vertices()(vertex_idx, 1) << ", "
                  << problematic_mesh.vertices()(vertex_idx, 2) << "]\n";
    }
}

void test_repair_system() {
    std::cout << "\n=== Testing Mesh Repair System ===\n";
    
    // Create a problematic mesh
    core::Mesh mesh;
    create_problematic_mesh(mesh);
    
    std::cout << "\nOriginal mesh info:\n";
    std::cout << "Vertices: " << mesh.vertices().rows() << "\n";
    std::cout << "Faces: " << mesh.faces().rows() << "\n";
    
    // Test repair with verbose output
    geometry::MeshRepairer::RepairOptions options;
    options.verbose = true;
    options.remove_degenerate_faces = true;
    options.merge_duplicate_vertices = true;
    options.remove_unreferenced_vertices = true;
    options.fix_face_orientation = true;
    options.vertex_merge_tolerance = geometry::DEFAULT_VERTEX_MERGE_TOLERANCE;
    
    auto repair_result = geometry::MeshRepairer::repair(mesh, options);
    
    std::cout << "\n" << repair_result.summary() << "\n";
    
    std::cout << "\nRepaired mesh info:\n";
    std::cout << "Vertices: " << mesh.vertices().rows() << "\n";
    std::cout << "Faces: " << mesh.faces().rows() << "\n";
}

void test_boolean_operations_with_validation() {
    std::cout << "\n=== Testing Boolean Operations with Validation ===\n";
    
    // Create two spheres for boolean operations
    auto sphere1_result = geometry::SphereGenerator::generate_uv_sphere(SPHERE1_RADIUS, UV_SUBDIVISIONS_16, UV_SUBDIVISIONS_16);
    auto sphere2_result = geometry::SphereGenerator::generate_uv_sphere(SPHERE2_RADIUS, UV_SUBDIVISIONS_12, UV_SUBDIVISIONS_12);
    
    if (!sphere1_result || !sphere2_result) {
        std::cout << "Failed to generate spheres\n";
        return;
    }
    
    auto& sphere1 = *sphere1_result;
    auto& sphere2 = *sphere2_result;
    
    // Translate second sphere
    for (int i = 0; i < sphere2.vertices().rows(); ++i) {
        sphere2.vertices().row(i) += Eigen::Vector3d(TRANSLATION_OFFSET, 0.0, 0.0);
    }
    
    std::cout << "\nValidating input spheres:\n";
    auto validation1 = geometry::MeshValidator::validate(sphere1);
    auto validation2 = geometry::MeshValidator::validate(sphere2);
    
    std::cout << "Sphere 1: " << (validation1.is_valid ? "VALID" : "INVALID") << "\n";
    std::cout << "Sphere 2: " << (validation2.is_valid ? "VALID" : "INVALID") << "\n";
    
    // Perform boolean union
    std::cout << "\nPerforming boolean union...\n";
    auto union_result = geometry::BooleanOps::union_meshes(sphere1, sphere2);
    
    if (union_result) {
        std::cout << "Union successful!\n";
        auto union_validation = geometry::MeshValidator::validate(*union_result);
        std::cout << "Union result validation:\n" << union_validation.summary() << "\n";
        
        // If validation shows issues, try to repair
        if (!union_validation.is_valid) {
            std::cout << "\nAttempting to repair union result...\n";
            geometry::MeshRepairer::RepairOptions repair_options;
            repair_options.verbose = true;
            auto repair_result = geometry::MeshRepairer::repair(*union_result, repair_options);
            std::cout << repair_result.summary() << "\n";
        }
        
        // Export the result
        if (io::ObjExporter::export_mesh(*union_result, "validated_union.obj")) {
            std::cout << "Union result exported to validated_union.obj\n";
        }
    } else {
        std::cout << "Union failed: " << geometry::BooleanOps::last_error().message << "\n";
    }
}

void test_manifold_checking() {
    std::cout << "\n=== Testing Manifold Checking ===\n";
    
    // Create a simple box and check if it's manifold
    auto box_result = geometry::BoxGenerator::generate(1.0, 1.0, 1.0);
    if (box_result) {
        bool is_manifold = geometry::MeshValidator::is_manifold(*box_result);
        std::cout << "Box is manifold: " << (is_manifold ? "YES" : "NO") << "\n";
        
        // Check edge manifoldness
        auto non_manifold_edges = geometry::MeshValidator::find_non_manifold_edges(*box_result);
        std::cout << "Non-manifold edges found: " << non_manifold_edges.size() << "\n";
    }
    
    // Create a sphere and check manifoldness
    auto sphere_result = geometry::SphereGenerator::generate_uv_sphere(SPHERE1_RADIUS, UV_SUBDIVISIONS_16, UV_SUBDIVISIONS_16);
    if (sphere_result) {
        bool is_manifold = geometry::MeshValidator::is_manifold(*sphere_result);
        std::cout << "UV Sphere is manifold: " << (is_manifold ? "YES" : "NO") << "\n";
    }
    
    // Test icosphere
    auto icosphere_result = geometry::SphereGenerator::generate_icosphere(SPHERE1_RADIUS, ICOSPHERE_SUBDIVISIONS);
    if (icosphere_result) {
        bool is_manifold = geometry::MeshValidator::is_manifold(*icosphere_result);
        std::cout << "Icosphere is manifold: " << (is_manifold ? "YES" : "NO") << "\n";
    }
}

int main() {
    std::cout << "NodeFlux Engine - Mesh Validation & Repair Demo\n";
    std::cout << "================================================\n";
    
    try {
        test_validation_system();
        test_repair_system();
        test_boolean_operations_with_validation();
        test_manifold_checking();
        
        std::cout << "\n=== Demo Complete ===\n";
        std::cout << "All mesh validation and repair systems tested successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
