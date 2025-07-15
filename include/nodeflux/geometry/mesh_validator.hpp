#pragma once

#include "../core/mesh.hpp"
#include "../core/error.hpp"
#include <optional>
#include <vector>
#include <set>

namespace nodeflux::geometry {

/// @brief Mesh validation results with detailed diagnostics
struct ValidationReport {
    bool is_valid = true;
    bool is_manifold = true;
    bool is_closed = true;
    bool has_self_intersections = false;
    bool has_degenerate_faces = false;
    bool has_duplicate_vertices = false;
    bool has_unreferenced_vertices = false;
    
    int num_vertices = 0;
    int num_faces = 0;
    int num_edges = 0;
    int num_boundary_edges = 0;
    int num_non_manifold_edges = 0;
    int num_isolated_vertices = 0;
    
    std::vector<int> degenerate_face_indices;
    std::vector<int> duplicate_vertex_indices;
    std::vector<int> unreferenced_vertex_indices;
    std::vector<int> non_manifold_edge_indices;
    
    /// @brief Get a summary message of validation results
    std::string summary() const;
    
    /// @brief Get detailed diagnostic information
    std::string detailed_report() const;
};

/// @brief Comprehensive mesh validation and repair tools
class MeshValidator {
public:
    /// @brief Validate a mesh comprehensively
    /// @param mesh The mesh to validate
    /// @return Detailed validation report
    static ValidationReport validate(const core::Mesh& mesh);
    
    /// @brief Quick check if mesh is valid for boolean operations
    /// @param mesh The mesh to check
    /// @return True if mesh is suitable for boolean operations
    static bool is_boolean_ready(const core::Mesh& mesh);
    
    /// @brief Check if mesh is manifold
    /// @param mesh The mesh to check
    /// @return True if mesh is manifold
    static bool is_manifold(const core::Mesh& mesh);
    
    /// @brief Check if mesh is closed (watertight)
    /// @param mesh The mesh to check
    /// @return True if mesh is closed
    static bool is_closed(const core::Mesh& mesh);
    
    /// @brief Check for degenerate faces (zero area or invalid indices)
    /// @param mesh The mesh to check
    /// @return Indices of degenerate faces
    static std::vector<int> find_degenerate_faces(const core::Mesh& mesh);
    
    /// @brief Check for duplicate vertices
    /// @param mesh The mesh to check
    /// @param tolerance Distance tolerance for considering vertices duplicates
    /// @return Indices of duplicate vertices
    static std::vector<int> find_duplicate_vertices(const core::Mesh& mesh, double tolerance = 1e-10);
    
    /// @brief Check for unreferenced vertices
    /// @param mesh The mesh to check
    /// @return Indices of vertices not referenced by any face
    static std::vector<int> find_unreferenced_vertices(const core::Mesh& mesh);
    
    /// @brief Find non-manifold edges (shared by more than 2 faces)
    /// @param mesh The mesh to check
    /// @return Indices of faces containing non-manifold edges
    static std::vector<int> find_non_manifold_edges(const core::Mesh& mesh);
    
    /// @brief Calculate mesh statistics
    /// @param mesh The mesh to analyze
    /// @param report Output validation report to fill
    static void calculate_statistics(const core::Mesh& mesh, ValidationReport& report);
    
    /// @brief Get the last error that occurred
    /// @return Reference to the last error
    static const core::Error& last_error();

private:
    static void set_last_error(const core::Error& error);
    static thread_local core::Error last_error_;
    
    /// @brief Edge representation for manifold checking
    struct Edge {
        int vertex1, vertex2;
        int face_count = 0;
        
        Edge(int v1, int v2) : vertex1(std::min(v1, v2)), vertex2(std::max(v1, v2)) {}
        
        bool operator<(const Edge& other) const {
            return std::tie(vertex1, vertex2) < std::tie(other.vertex1, other.vertex2);
        }
    };
    
    /// @brief Calculate face area for degeneracy checking
    static double calculate_face_area(const core::Mesh& mesh, int face_index);
    
    /// @brief Check if three vertices are collinear
    static bool are_collinear(const Eigen::Vector3d& v1, const Eigen::Vector3d& v2, 
                              const Eigen::Vector3d& v3, double tolerance = 1e-10);
};

} // namespace nodeflux::geometry
