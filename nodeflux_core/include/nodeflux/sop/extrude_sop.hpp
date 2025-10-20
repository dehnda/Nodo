#pragma once

#include "nodeflux/core/mesh.hpp"
#include <memory>
#include <string>
#include <chrono>
#include <Eigen/Dense>

namespace nodeflux::sop {

/**
 * @brief Extrude faces along their normals or a specified direction
 * 
 * ExtrudeSOP creates geometric extrusions by moving faces along vectors.
 * Supports multiple extrusion modes for procedural modeling workflows.
 */
class ExtrudeSOP {
public:
    /// Extrusion modes for different geometric operations
    enum class ExtrusionMode {
        FACE_NORMALS,     ///< Extrude along individual face normals
        UNIFORM_DIRECTION, ///< Extrude all faces in the same direction
        REGION_NORMALS    ///< Extrude along averaged region normals
    };

    /**
     * @brief Construct extrude operator with name
     * @param name Unique identifier for this SOP instance
     */
    explicit ExtrudeSOP(const std::string& name);

    /**
     * @brief Set the input mesh to extrude
     * @param mesh Input mesh with faces to extrude
     */
    void set_input_mesh(std::shared_ptr<core::Mesh> mesh);

    /**
     * @brief Set extrusion distance
     * @param distance How far to extrude (can be negative for inward)
     */
    void set_distance(double distance);

    /**
     * @brief Set extrusion direction for uniform mode
     * @param direction Unit vector specifying extrusion direction
     */
    void set_direction(const Eigen::Vector3d& direction);

    /**
     * @brief Set extrusion mode
     * @param mode Type of extrusion to perform
     */
    void set_mode(ExtrusionMode mode);

    /**
     * @brief Set inset amount for creating border around extruded faces
     * @param inset Inset distance (0.0 = no inset)
     */
    void set_inset(double inset);

    /**
     * @brief Execute extrusion with intelligent caching
     * @return Extruded mesh, or nullptr on failure
     */
    std::shared_ptr<core::Mesh> cook();

    /**
     * @brief Get the name of this SOP
     * @return SOP identifier string
     */
    const std::string& name() const { return name_; }

    /**
     * @brief Get last execution time in milliseconds
     * @return Processing duration
     */
    std::chrono::milliseconds last_cook_time() const { return last_cook_time_; }

private:
    /// Execute the actual extrusion operation
    std::shared_ptr<core::Mesh> execute();

    /// Extrude faces along their individual normals
    core::Mesh extrude_face_normals(const core::Mesh& input);

    /// Extrude all faces in uniform direction
    core::Mesh extrude_uniform_direction(const core::Mesh& input);

    /// Extrude faces along averaged region normals  
    core::Mesh extrude_region_normals(const core::Mesh& input);

    /// Calculate face normals for the mesh
    Eigen::MatrixXd calculate_face_normals(const core::Mesh& mesh);

    /// Create inset faces if inset > 0
    void apply_inset(core::Mesh& mesh, double inset_amount);

    /// Validate extrusion parameters
    bool validate_parameters() const;

    /// Check if recalculation is needed
    bool needs_recalculation() const;

    std::string name_;                              ///< SOP identifier
    std::shared_ptr<core::Mesh> input_mesh_;      ///< Input geometry
    std::shared_ptr<core::Mesh> cached_result_;   ///< Cached output
    
    // Extrusion parameters
    double distance_;                              ///< Extrusion distance
    Eigen::Vector3d direction_;                   ///< Uniform extrusion direction
    ExtrusionMode mode_;                          ///< Extrusion algorithm
    double inset_;                                ///< Inset amount for borders
    
    // Performance tracking
    std::chrono::milliseconds last_cook_time_;    ///< Last execution duration
    bool cache_valid_;                            ///< Cache state flag
    size_t last_input_hash_;                      ///< Input change detection
};

} // namespace nodeflux::sop
