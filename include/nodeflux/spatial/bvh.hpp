#pragma once

#include "../core/mesh.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <memory>
#include <vector>
#include <optional>

namespace nodeflux::spatial {

/**
 * @brief Axis-Aligned Bounding Box for spatial queries
 */
struct AABB {
    Eigen::Vector3d min_point;
    Eigen::Vector3d max_point;

    AABB() = default;
    AABB(const Eigen::Vector3d& min_pt, const Eigen::Vector3d& max_pt);

    /// @brief Create AABB from a set of points
    static AABB from_points(const std::vector<Eigen::Vector3d>& points);
    
    /// @brief Create AABB from mesh vertices
    static AABB from_mesh(const core::Mesh& mesh);

    /// @brief Check if this AABB intersects with another
    bool intersects(const AABB& other) const;

    /// @brief Check if this AABB contains a point
    bool contains(const Eigen::Vector3d& point) const;

    /// @brief Expand AABB to include another AABB
    void expand(const AABB& other);

    /// @brief Expand AABB to include a point
    void expand(const Eigen::Vector3d& point);

    /// @brief Get the center of the AABB
    Eigen::Vector3d center() const;

    /// @brief Get the size (extent) of the AABB
    Eigen::Vector3d size() const;

    /// @brief Get the surface area of the AABB
    double surface_area() const;

    /// @brief Get the volume of the AABB
    double volume() const;

    /// @brief Check if AABB is valid (min <= max)
    bool is_valid() const;
};

/**
 * @brief BVH Node for hierarchical spatial partitioning
 */
struct BVHNode {
    AABB bounding_box;
    std::unique_ptr<BVHNode> left_child;
    std::unique_ptr<BVHNode> right_child;
    std::vector<int> triangle_indices;  // Only filled for leaf nodes
    bool is_leaf;

    BVHNode() : is_leaf(false) {}
    explicit BVHNode(const AABB& bbox) : bounding_box(bbox), is_leaf(false) {}
};

/**
 * @brief Bounding Volume Hierarchy for fast spatial queries
 * 
 * This BVH implementation provides efficient ray-mesh intersection,
 * point queries, and mesh-mesh collision detection for boolean operations.
 */
class BVH {
public:
    /// @brief Construction parameters for BVH
    struct BuildParams {
        static constexpr int DEFAULT_MAX_TRIANGLES_PER_LEAF = 10;
        static constexpr int DEFAULT_MAX_DEPTH = 20;
        
        int max_triangles_per_leaf = DEFAULT_MAX_TRIANGLES_PER_LEAF;
        int max_depth = DEFAULT_MAX_DEPTH;
        bool use_sah = true;  // Surface Area Heuristic
    };

    /// @brief Ray for intersection queries
    struct Ray {
        Eigen::Vector3d origin;
        Eigen::Vector3d direction;
        double t_min = 0.0;
        double t_max = std::numeric_limits<double>::infinity();

        Ray(const Eigen::Vector3d& orig, const Eigen::Vector3d& dir)
            : origin(orig), direction(dir.normalized()) {}
    };

    /// @brief Hit information for ray queries
    struct RayHit {
        double t;                    // Distance along ray
        int triangle_index;          // Index of hit triangle
        Eigen::Vector3d point;       // Hit point in world space
        Eigen::Vector3d normal;      // Surface normal at hit point
        Eigen::Vector2d barycentric; // Barycentric coordinates

        RayHit() : t(std::numeric_limits<double>::infinity()), triangle_index(-1) {}
    };

    /// @brief Default constructor
    BVH() = default;

    /// @brief Build BVH from mesh
    /// @param mesh The mesh to build BVH for
    /// @param params Build parameters
    /// @return true if BVH was built successfully
    bool build(const core::Mesh& mesh, const BuildParams& params);

    /// @brief Query ray intersection with mesh
    /// @param ray The ray to test
    /// @return Hit information, or nullopt if no intersection
    std::optional<RayHit> intersect_ray(const Ray& ray) const;

    /// @brief Find all triangles intersecting with AABB
    /// @param aabb The bounding box to query
    /// @return Vector of triangle indices
    std::vector<int> query_aabb(const AABB& aabb) const;

    /// @brief Find closest point on mesh surface to given point
    /// @param point The query point
    /// @return Closest point and triangle index, or nullopt if mesh is empty
    std::optional<std::pair<Eigen::Vector3d, int>> closest_point(const Eigen::Vector3d& point) const;

    /// @brief Check if BVH has been built
    bool is_built() const { return root_ != nullptr; }

    /// @brief Get the root bounding box
    const AABB& root_bounds() const { return root_->bounding_box; }

    /// @brief Clear the BVH
    void clear();

    /// @brief Get build statistics
    struct Stats {
        int total_nodes = 0;
        int leaf_nodes = 0;
        int max_depth = 0;
        double build_time_ms = 0.0;
    };
    const Stats& stats() const { return stats_; }

private:
    std::unique_ptr<BVHNode> root_;
    const core::Mesh* mesh_ = nullptr;  // Reference to mesh (not owned)
    BuildParams params_;
    Stats stats_;

    /// @brief Recursive BVH build function
    std::unique_ptr<BVHNode> build_recursive(
        const std::vector<int>& triangle_indices,
        const AABB& node_bounds,
        int depth
    );

    /// @brief Calculate bounding box for a set of triangles
    AABB calculate_triangle_bounds(const std::vector<int>& triangle_indices) const;

    /// @brief Split triangles using Surface Area Heuristic
    std::pair<std::vector<int>, std::vector<int>> split_triangles_sah(
        const std::vector<int>& triangle_indices,
        const AABB& node_bounds
    ) const;

    /// @brief Simple midpoint split
    std::pair<std::vector<int>, std::vector<int>> split_triangles_midpoint(
        const std::vector<int>& triangle_indices,
        const AABB& node_bounds
    ) const;

    /// @brief Ray-AABB intersection test
    bool ray_aabb_intersect(const Ray& ray, const AABB& aabb, double& t_near, double& t_far) const;

    /// @brief Ray-triangle intersection test
    bool ray_triangle_intersect(const Ray& ray, int triangle_index, RayHit& hit) const;

    /// @brief Recursive ray intersection
    bool intersect_ray_recursive(const BVHNode* node, const Ray& ray, RayHit& closest_hit) const;

    /// @brief Recursive AABB query
    void query_aabb_recursive(const BVHNode* node, const AABB& query_aabb, std::vector<int>& results) const;
};

} // namespace nodeflux::spatial
