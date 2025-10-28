#pragma once

#include "../core/mesh.hpp"
#include "../core/error.hpp"
#include "mesh_validator.hpp"
#include <optional>

namespace nodo::geometry {

/// @brief Default tolerance for vertex merging operations
constexpr double DEFAULT_VERTEX_MERGE_TOLERANCE = 1e-10;

/// @brief Mesh repair operations for fixing common issues
class MeshRepairer {
public:
    /// @brief Repair options for mesh fixing
    struct RepairOptions {
        bool remove_degenerate_faces;
        bool merge_duplicate_vertices;
        bool remove_unreferenced_vertices;
        bool fix_face_orientation;
        double vertex_merge_tolerance;
        bool verbose;
        
        RepairOptions() : 
            remove_degenerate_faces(true),
            merge_duplicate_vertices(true),
            remove_unreferenced_vertices(true),
            fix_face_orientation(true),
            vertex_merge_tolerance(DEFAULT_VERTEX_MERGE_TOLERANCE),
            verbose(false) {}
    };
    
    /// @brief Repair result with statistics
    struct RepairResult {
        bool success;
        std::string message;
        int faces_removed;
        int vertices_merged;
        int vertices_removed;
        int faces_reoriented;
        ValidationReport final_report;
        
        RepairResult() : 
            success(false),
            faces_removed(0),
            vertices_merged(0),
            vertices_removed(0),
            faces_reoriented(0) {}
        
        /// @brief Get summary of repair operations
        std::string summary() const;
    };
    
    /// @brief Attempt to repair a mesh automatically
    /// @param mesh The mesh to repair (will be modified in place)
    /// @param options Repair options
    /// @return Repair result with statistics
    static RepairResult repair(core::Mesh& mesh, const RepairOptions& options = RepairOptions{});
    
    /// @brief Remove degenerate faces from mesh
    /// @param mesh The mesh to repair (will be modified in place)
    /// @return Number of faces removed
    static int remove_degenerate_faces(core::Mesh& mesh);
    
    /// @brief Merge duplicate vertices
    /// @param mesh The mesh to repair (will be modified in place)
    /// @param tolerance Distance tolerance for merging
    /// @return Number of vertices merged
    static int merge_duplicate_vertices(core::Mesh& mesh, double tolerance = DEFAULT_VERTEX_MERGE_TOLERANCE);
    
    /// @brief Remove unreferenced vertices
    /// @param mesh The mesh to repair (will be modified in place)
    /// @return Number of vertices removed
    static int remove_unreferenced_vertices(core::Mesh& mesh);
    
    /// @brief Attempt to fix face orientation for consistent normals
    /// @param mesh The mesh to repair (will be modified in place)
    /// @return Number of faces reoriented
    static int fix_face_orientation(core::Mesh& mesh);
    
    /// @brief Create a manifold mesh by removing non-manifold elements
    /// @param mesh The mesh to repair (will be modified in place)
    /// @return True if mesh is now manifold
    static bool make_manifold(core::Mesh& mesh);
    
    /// @brief Recalculate face normals (ensure consistent winding)
    /// @param mesh The mesh to process
    /// @return Number of faces with flipped normals
    static int recalculate_normals(core::Mesh& mesh);
    
    /// @brief Get the last error that occurred
    /// @return Reference to the last error
    static const core::Error& last_error();

private:
    static void set_last_error(const core::Error& error);
    static thread_local core::Error last_error_;
    
    /// @brief Compact mesh by removing unused vertices and reindexing faces
    static void compact_mesh(core::Mesh& mesh, const std::vector<bool>& vertex_keep_mask);
    
    /// @brief Build vertex mapping after merging duplicates
    static std::vector<int> build_vertex_mapping(const core::Mesh& mesh, double tolerance);
};

} // namespace nodo::geometry
