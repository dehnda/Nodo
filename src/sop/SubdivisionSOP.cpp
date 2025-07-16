#include "nodeflux/sop/SubdivisionSOP.hpp"
#include <vector>

namespace nodeflux::sop {

std::optional<core::Mesh> SubdivisionSOP::process(const core::Mesh& input_mesh) {
    if (subdivision_levels_ == 0) {
        return input_mesh; // No subdivision
    }

    // Apply subdivision iteratively
    core::Mesh result = input_mesh;
    for (int level = 0; level < subdivision_levels_; ++level) {
        auto subdivided = apply_catmull_clark_subdivision(result);
        if (!subdivided) {
            return std::nullopt; // Subdivision failed
        }
        result = std::move(*subdivided);
    }

    return result;
}

std::optional<core::Mesh> SubdivisionSOP::apply_catmull_clark_subdivision(const core::Mesh& mesh) {
    const auto& vertices = mesh.vertices();
    const auto& faces = mesh.faces();

    // Simple approach: refine each triangle by adding midpoints and face center
    std::vector<Eigen::Vector3d> new_vertices;
    std::vector<Eigen::Vector3i> new_faces;

    // Add original vertices
    for (int i = 0; i < vertices.rows(); ++i) {
        new_vertices.push_back(vertices.row(i));
    }

    // Process each face
    for (int face_idx = 0; face_idx < faces.rows(); ++face_idx) {
        const auto& face = faces.row(face_idx);
        
        // Get face vertices
        Eigen::Vector3d v0 = vertices.row(face(0));
        Eigen::Vector3d v1 = vertices.row(face(1));
        Eigen::Vector3d v2 = vertices.row(face(2));

        // Calculate face center
        Eigen::Vector3d face_center = (v0 + v1 + v2) / 3.0;
        int face_center_idx = static_cast<int>(new_vertices.size());
        new_vertices.push_back(face_center);

        // Calculate edge midpoints
        Eigen::Vector3d edge01_mid = (v0 + v1) / 2.0;
        Eigen::Vector3d edge12_mid = (v1 + v2) / 2.0;
        Eigen::Vector3d edge20_mid = (v2 + v0) / 2.0;

        int edge01_idx = static_cast<int>(new_vertices.size());
        int edge12_idx = static_cast<int>(new_vertices.size() + 1);
        int edge20_idx = static_cast<int>(new_vertices.size() + 2);

        new_vertices.push_back(edge01_mid);
        new_vertices.push_back(edge12_mid);
        new_vertices.push_back(edge20_mid);

        // Create new faces (split triangle into 6 triangles)
        new_faces.emplace_back(face(0), edge01_idx, edge20_idx);
        new_faces.emplace_back(edge01_idx, face(1), edge12_idx);
        new_faces.emplace_back(edge20_idx, edge12_idx, face(2));
        new_faces.emplace_back(edge01_idx, edge12_idx, face_center_idx);
        new_faces.emplace_back(edge12_idx, edge20_idx, face_center_idx);
        new_faces.emplace_back(edge20_idx, edge01_idx, face_center_idx);
    }

    // Convert to Eigen matrices
    core::Mesh::Vertices output_vertices(new_vertices.size(), 3);
    core::Mesh::Faces output_faces(new_faces.size(), 3);

    for (size_t i = 0; i < new_vertices.size(); ++i) {
        output_vertices.row(i) = new_vertices[i];
    }

    for (size_t i = 0; i < new_faces.size(); ++i) {
        output_faces.row(i) = new_faces[i];
    }

    return core::Mesh(std::move(output_vertices), std::move(output_faces));
}

} // namespace nodeflux::sop
