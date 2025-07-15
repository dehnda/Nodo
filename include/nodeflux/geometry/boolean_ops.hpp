#pragma once

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/core/error.hpp"
#include <optional>
#include <memory>

namespace nodeflux::geometry {

/**
 * @brief Clean, modern interface for CGAL boolean operations
 * 
 * This class provides direct CGAL integration with simple C++20 patterns.
 * Operations return std::optional for success/failure, with separate error reporting.
 */
class BooleanOps {
public:
    /**
     * @brief Perform union of two meshes
     * @param a First mesh
     * @param b Second mesh
     * @return Optional mesh containing the union, or nullopt on failure
     */
    static std::optional<core::Mesh> union_meshes(const core::Mesh& a, const core::Mesh& b);
    
    /**
     * @brief Perform intersection of two meshes
     * @param a First mesh
     * @param b Second mesh
     * @return Optional mesh containing the intersection, or nullopt on failure
     */
    static std::optional<core::Mesh> intersect_meshes(const core::Mesh& a, const core::Mesh& b);
    
    /**
     * @brief Perform difference of two meshes (a - b)
     * @param a First mesh (minuend)
     * @param b Second mesh (subtrahend)
     * @return Optional mesh containing the difference, or nullopt on failure
     */
    static std::optional<core::Mesh> difference_meshes(const core::Mesh& a, const core::Mesh& b);
    
    /**
     * @brief Get the last error that occurred
     * @return Error information for the last failed operation
     */
    static const core::Error& last_error();
    
    /**
     * @brief Check if meshes are compatible for boolean operations
     * @param a First mesh
     * @param b Second mesh
     * @return true if meshes can be used for boolean operations
     */
    static bool are_compatible(const core::Mesh& a, const core::Mesh& b) noexcept;
    
    /**
     * @brief Validate a single mesh for boolean operations
     * @param mesh Mesh to validate
     * @return true if mesh is valid for boolean operations
     */
    static bool validate_mesh(const core::Mesh& mesh);

private:
    /// Internal CGAL boolean operation implementation
    static std::optional<core::Mesh> cgal_boolean_operation(
        const core::Mesh& a, 
        const core::Mesh& b, 
        int operation_type
    );
    
    /// Set the last error for error reporting
    static void set_last_error(const core::Error& error);
    
    /// Thread-local storage for last error
    static thread_local core::Error last_error_;
};

} // namespace nodeflux::geometry
