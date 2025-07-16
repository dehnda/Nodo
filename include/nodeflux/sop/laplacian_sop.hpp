#pragma once

#include "nodeflux/core/mesh.hpp"
#include <memory>
#include <string>
#include <chrono>
#include <Eigen/Dense>
#include <Eigen/Sparse>

namespace nodeflux::sop {

/**
 * @brief Laplacian mesh smoothing operator
 * 
 * LaplacianSOP applies Laplacian smoothing to reduce mesh noise and 
 * create smoother surfaces while preserving overall shape.
 */
class LaplacianSOP {
public:
    /// Smoothing algorithm variants
    enum class SmoothingMethod {
        UNIFORM,          ///< Uniform Laplacian (simple average)
        COTANGENT,        ///< Cotangent-weighted Laplacian (angle-aware)
        TAUBIN           ///< Taubin smoothing (prevents shrinkage)
    };

    /**
     * @brief Construct Laplacian smoother with name
     * @param name Unique identifier for this SOP instance
     */
    explicit LaplacianSOP(const std::string& name);

    /**
     * @brief Set the input mesh to smooth
     * @param mesh Input mesh to apply smoothing
     */
    void set_input_mesh(std::shared_ptr<core::Mesh> mesh);

    /**
     * @brief Set number of smoothing iterations
     * @param iterations Number of smoothing passes (default: 1)
     */
    void set_iterations(int iterations);

    /**
     * @brief Set smoothing strength factor
     * @param lambda Smoothing factor (0.0 = no change, 1.0 = full smoothing)
     */
    void set_lambda(double lambda);

    /**
     * @brief Set smoothing method
     * @param method Algorithm variant to use
     */
    void set_method(SmoothingMethod method);

    /**
     * @brief Set shrinkage prevention factor (for Taubin method)
     * @param mu Negative smoothing factor to prevent volume loss
     */
    void set_mu(double mu);

    /**
     * @brief Set boundary vertex handling
     * @param preserve If true, boundary vertices remain fixed
     */
    void set_preserve_boundaries(bool preserve);

    /**
     * @brief Execute smoothing with intelligent caching
     * @return Smoothed mesh, or nullptr on failure
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
    /// Execute the actual smoothing operation
    std::shared_ptr<core::Mesh> execute();

    /// Apply uniform Laplacian smoothing
    Eigen::MatrixXd apply_uniform_laplacian(const Eigen::MatrixXd& vertices,
                                           const Eigen::MatrixXi& faces);

    /// Apply cotangent-weighted Laplacian smoothing
    Eigen::MatrixXd apply_cotangent_laplacian(const Eigen::MatrixXd& vertices,
                                             const Eigen::MatrixXi& faces);

    /// Apply Taubin smoothing (lambda-mu scheme)
    Eigen::MatrixXd apply_taubin_smoothing(const Eigen::MatrixXd& vertices,
                                          const Eigen::MatrixXi& faces);

    /// Build uniform Laplacian matrix
    Eigen::SparseMatrix<double> build_uniform_laplacian(const Eigen::MatrixXi& faces,
                                                        int num_vertices);

    /// Build cotangent-weighted Laplacian matrix
    Eigen::SparseMatrix<double> build_cotangent_laplacian(const Eigen::MatrixXd& vertices,
                                                          const Eigen::MatrixXi& faces);

    /// Find boundary vertices
    std::vector<bool> find_boundary_vertices(const Eigen::MatrixXi& faces,
                                             int num_vertices);

    /// Calculate cotangent weights for an edge
    double calculate_cotangent_weight(const Eigen::Vector3d& v1,
                                     const Eigen::Vector3d& v2,
                                     const Eigen::Vector3d& opposite);

    /// Validate smoothing parameters
    bool validate_parameters() const;

    /// Check if recalculation is needed
    bool needs_recalculation() const;

    std::string name_;                              ///< SOP identifier
    std::shared_ptr<core::Mesh> input_mesh_;      ///< Input geometry
    std::shared_ptr<core::Mesh> cached_result_;   ///< Cached output
    
    // Smoothing parameters
    int iterations_;                               ///< Number of smoothing passes
    double lambda_;                               ///< Smoothing strength
    double mu_;                                   ///< Shrinkage prevention factor
    SmoothingMethod method_;                      ///< Smoothing algorithm
    bool preserve_boundaries_;                    ///< Keep boundary vertices fixed
    
    // Performance tracking
    std::chrono::milliseconds last_cook_time_;    ///< Last execution duration
    bool cache_valid_;                            ///< Cache state flag
    size_t last_input_hash_;                      ///< Input change detection
};

} // namespace nodeflux::sop
