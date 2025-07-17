#include "../../include/nodeflux/geometry/mesh_generator.hpp"
#include "../../include/nodeflux/geometry/sphere_generator.hpp"
#include <array>
#include <cmath>

namespace nodeflux::geometry {

// Thread-local storage for error reporting
thread_local core::Error MeshGenerator::last_error_{
    core::ErrorCategory::Unknown, 
    core::ErrorCode::Unknown, 
    "No error"
};

core::Mesh MeshGenerator::box(const Eigen::Vector3d& min_corner, 
                             const Eigen::Vector3d& max_corner) {
    // Create vertices for a box (8 vertices)
    core::Mesh::Vertices vertices(8, 3);
    
    // Define the 8 corners of the box
    vertices.row(0) = Eigen::Vector3d(min_corner.x(), min_corner.y(), min_corner.z());
    vertices.row(1) = Eigen::Vector3d(max_corner.x(), min_corner.y(), min_corner.z());
    vertices.row(2) = Eigen::Vector3d(max_corner.x(), max_corner.y(), min_corner.z());
    vertices.row(3) = Eigen::Vector3d(min_corner.x(), max_corner.y(), min_corner.z());
    vertices.row(4) = Eigen::Vector3d(min_corner.x(), min_corner.y(), max_corner.z());
    vertices.row(5) = Eigen::Vector3d(max_corner.x(), min_corner.y(), max_corner.z());
    vertices.row(6) = Eigen::Vector3d(max_corner.x(), max_corner.y(), max_corner.z());
    vertices.row(7) = Eigen::Vector3d(min_corner.x(), max_corner.y(), max_corner.z());
    
    // Create faces for a box (12 triangular faces, 2 per face of the cube)
    core::Mesh::Faces faces(12, 3);
    
    // Front face (z = min_corner.z)
    faces.row(0) = Eigen::Vector3i(0, 1, 2);
    faces.row(1) = Eigen::Vector3i(0, 2, 3);
    
    // Back face (z = max_corner.z)
    faces.row(2) = Eigen::Vector3i(4, 7, 6);
    faces.row(3) = Eigen::Vector3i(4, 6, 5);
    
    // Left face (x = min_corner.x)
    faces.row(4) = Eigen::Vector3i(0, 3, 7);
    faces.row(5) = Eigen::Vector3i(0, 7, 4);
    
    // Right face (x = max_corner.x)
    faces.row(6) = Eigen::Vector3i(1, 5, 6);
    faces.row(7) = Eigen::Vector3i(1, 6, 2);
    
    // Bottom face (y = min_corner.y)
    faces.row(8) = Eigen::Vector3i(0, 4, 5);
    faces.row(9) = Eigen::Vector3i(0, 5, 1);
    
    // Top face (y = max_corner.y)
    faces.row(10) = Eigen::Vector3i(3, 2, 6);
    faces.row(11) = Eigen::Vector3i(3, 6, 7);
    
    return core::Mesh(std::move(vertices), std::move(faces));
}

std::optional<core::Mesh> MeshGenerator::sphere(const Eigen::Vector3d& center, 
                                               double radius, 
                                               int subdivisions) {
    if (!validate_sphere_params(radius, subdivisions)) {
        return std::nullopt;
    }
    
    // Use the proper SphereGenerator for icosphere generation
    auto result = SphereGenerator::generate_icosphere(radius, subdivisions);
    if (!result.has_value()) {
        return std::nullopt;
    }
    
    // Translate the sphere to the desired center if needed
    if (center != Eigen::Vector3d::Zero()) {
        core::Mesh mesh = result.value();
        for (int i = 0; i < mesh.vertices().rows(); ++i) {
            mesh.vertices().row(i) += center.transpose();
        }
        return mesh;
    }
    
    return result;
}

std::optional<core::Mesh> MeshGenerator::cylinder(const Eigen::Vector3d& bottom_center,
                                                 const Eigen::Vector3d& top_center,
                                                 double radius,
                                                 int segments) {
    if (!validate_cylinder_params(radius, segments)) {
        return std::nullopt;
    }
    
    return generate_cylinder_geometry(bottom_center, top_center, radius, segments);
}

const core::Error& MeshGenerator::last_error() {
    return last_error_;
}

core::Mesh MeshGenerator::generate_icosphere(const Eigen::Vector3d& center,
                                            double radius, 
                                            int subdivisions) {
    // Simple sphere approximation - create an octahedron and project vertices to sphere
    // This is a simplified implementation
    
    const int num_vertices = 6; // octahedron has 6 vertices
    const int num_faces = 8;    // octahedron has 8 faces
    
    core::Mesh::Vertices vertices(num_vertices, 3);
    core::Mesh::Faces faces(num_faces, 3);
    
    // Octahedron vertices
    vertices.row(0) = center + Eigen::Vector3d(radius, 0, 0);    // +X
    vertices.row(1) = center + Eigen::Vector3d(-radius, 0, 0);   // -X
    vertices.row(2) = center + Eigen::Vector3d(0, radius, 0);    // +Y
    vertices.row(3) = center + Eigen::Vector3d(0, -radius, 0);   // -Y
    vertices.row(4) = center + Eigen::Vector3d(0, 0, radius);    // +Z
    vertices.row(5) = center + Eigen::Vector3d(0, 0, -radius);   // -Z
    
    // Octahedron faces
    faces.row(0) = Eigen::Vector3i(0, 2, 4); // +X, +Y, +Z
    faces.row(1) = Eigen::Vector3i(0, 4, 3); // +X, +Z, -Y
    faces.row(2) = Eigen::Vector3i(0, 3, 5); // +X, -Y, -Z
    faces.row(3) = Eigen::Vector3i(0, 5, 2); // +X, -Z, +Y
    faces.row(4) = Eigen::Vector3i(1, 4, 2); // -X, +Z, +Y
    faces.row(5) = Eigen::Vector3i(1, 3, 4); // -X, -Y, +Z
    faces.row(6) = Eigen::Vector3i(1, 5, 3); // -X, -Z, -Y
    faces.row(7) = Eigen::Vector3i(1, 2, 5); // -X, +Y, -Z
    
    return core::Mesh(std::move(vertices), std::move(faces));
}

core::Mesh MeshGenerator::generate_cylinder_geometry(const Eigen::Vector3d& bottom_center,
                                                    const Eigen::Vector3d& top_center,
                                                    double radius,
                                                    int segments) {
    // Simple cylinder implementation
    const int num_vertices = segments * 2 + 2; // bottom ring + top ring + 2 centers
    const int num_faces = segments * 4; // sides + bottom cap + top cap
    
    core::Mesh::Vertices vertices(num_vertices, 3);
    core::Mesh::Faces faces(num_faces, 3);
    
    const Eigen::Vector3d axis = (top_center - bottom_center).normalized();
    const double height = (top_center - bottom_center).norm();
    
    // Generate vertices
    int vertex_idx = 0;
    
    // Bottom center
    vertices.row(vertex_idx++) = bottom_center;
    
    // Top center  
    vertices.row(vertex_idx++) = top_center;
    
    // Bottom ring
    for (int i = 0; i < segments; ++i) {
        double angle = 2.0 * M_PI * i / segments;
        Eigen::Vector3d offset(radius * cos(angle), radius * sin(angle), 0);
        vertices.row(vertex_idx++) = bottom_center + offset;
    }
    
    // Top ring
    for (int i = 0; i < segments; ++i) {
        double angle = 2.0 * M_PI * i / segments;
        Eigen::Vector3d offset(radius * cos(angle), radius * sin(angle), 0);
        vertices.row(vertex_idx++) = top_center + offset;
    }
    
    // Generate faces (simplified)
    int face_idx = 0;
    
    // Bottom cap
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        faces.row(face_idx++) = Eigen::Vector3i(0, 2 + i, 2 + next);
    }
    
    // Top cap  
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        faces.row(face_idx++) = Eigen::Vector3i(1, 2 + segments + next, 2 + segments + i);
    }
    
    // Side faces
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        int bottom_i = 2 + i;
        int bottom_next = 2 + next;
        int top_i = 2 + segments + i;
        int top_next = 2 + segments + next;
        
        // Two triangles per side
        faces.row(face_idx++) = Eigen::Vector3i(bottom_i, top_i, top_next);
        faces.row(face_idx++) = Eigen::Vector3i(bottom_i, top_next, bottom_next);
    }
    
    return core::Mesh(std::move(vertices), std::move(faces));
}

bool MeshGenerator::validate_sphere_params(double radius, int subdivisions) {
    if (radius <= 0.0) {
        set_last_error(core::Error{
            core::ErrorCategory::Validation, 
            core::ErrorCode::InvalidMesh, 
            "Sphere radius must be positive"
        });
        return false;
    }
    
    if (subdivisions < 0 || subdivisions > 5) {
        set_last_error(core::Error{
            core::ErrorCategory::Validation, 
            core::ErrorCode::InvalidMesh, 
            "Sphere subdivisions must be between 0 and 5"
        });
        return false;
    }
    
    return true;
}

bool MeshGenerator::validate_cylinder_params(double radius, int segments) {
    if (radius <= 0.0) {
        set_last_error(core::Error{
            core::ErrorCategory::Validation, 
            core::ErrorCode::InvalidMesh, 
            "Cylinder radius must be positive"
        });
        return false;
    }
    
    if (segments < 3) {
        set_last_error(core::Error{
            core::ErrorCategory::Validation, 
            core::ErrorCode::InvalidMesh, 
            "Cylinder must have at least 3 segments"
        });
        return false;
    }
    
    return true;
}

void MeshGenerator::set_last_error(const core::Error& error) {
    last_error_ = error;
}

} // namespace nodeflux::geometry
