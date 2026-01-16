#pragma once

#include "../core/error.hpp"
#include "../core/mesh.hpp"
#include "bvh.hpp"

#include <optional>

namespace nodo::spatial {

/**
 * @brief Enhanced boolean operations using spatial acceleration
 *
 * This class provides boolean operations optimized with BVH spatial structures
 * for better performance and improved mesh closure handling.
 */
class EnhancedBooleanOps {
public:
  /// @brief Parameters for boolean operations
  struct BooleanParams {
    static constexpr double DEFAULT_TOLERANCE = 1e-6;

    bool use_mesh_repair = true;          // Apply mesh repair before operations
    bool build_bvh = true;                // Use BVH for spatial acceleration
    double tolerance = DEFAULT_TOLERANCE; // Geometric tolerance
    bool validate_input = true;           // Validate input meshes
    bool ensure_manifold = true;          // Ensure result is manifold
  };

  /// @brief Enhanced union operation with spatial acceleration
  /// @param mesh_a First mesh
  /// @param mesh_b Second mesh
  /// @param params Operation parameters
  /// @return Result mesh or nullopt on failure
  static std::optional<core::Mesh> union_meshes(const core::Mesh& mesh_a, const core::Mesh& mesh_b,
                                                const BooleanParams& params);

  /// @brief Enhanced intersection operation with spatial acceleration
  /// @param mesh_a First mesh
  /// @param mesh_b Second mesh
  /// @param params Operation parameters
  /// @return Result mesh or nullopt on failure
  static std::optional<core::Mesh> intersect_meshes(const core::Mesh& mesh_a, const core::Mesh& mesh_b,
                                                    const BooleanParams& params);

  /// @brief Enhanced difference operation with spatial acceleration
  /// @param mesh_a First mesh (minuend)
  /// @param mesh_b Second mesh (subtrahend)
  /// @param params Operation parameters
  /// @return Result mesh or nullopt on failure
  static std::optional<core::Mesh> subtract_meshes(const core::Mesh& mesh_a, const core::Mesh& mesh_b,
                                                   const BooleanParams& params);

  /// @brief Check if two meshes intersect using BVH
  /// @param mesh_a First mesh
  /// @param mesh_b Second mesh
  /// @return true if meshes intersect
  static bool meshes_intersect(const core::Mesh& mesh_a, const core::Mesh& mesh_b);

  /// @brief Find intersection points between two meshes
  /// @param mesh_a First mesh
  /// @param mesh_b Second mesh
  /// @return Vector of intersection points
  static std::vector<Eigen::Vector3d> find_intersection_points(const core::Mesh& mesh_a, const core::Mesh& mesh_b);

  /// @brief Prepare mesh for boolean operations (repair + validation)
  /// @param mesh Input mesh
  /// @param params Operation parameters
  /// @return Prepared mesh or nullopt if preparation failed
  static std::optional<core::Mesh> prepare_mesh_for_boolean(const core::Mesh& mesh, const BooleanParams& params);

  /// @brief Get the last error that occurred
  /// @return Reference to the last error
  static const core::Error& last_error();

private:
  static thread_local core::Error last_error_;

  /// @brief Build BVH for mesh if enabled
  static std::unique_ptr<BVH> build_mesh_bvh(const core::Mesh& mesh, const BooleanParams& params);

  /// @brief Validate mesh for boolean operations
  static bool validate_mesh_for_boolean(const core::Mesh& mesh, const BooleanParams& params);

  /// @brief Post-process boolean operation result
  static std::optional<core::Mesh> post_process_result(const core::Mesh& result, const BooleanParams& params);
};

} // namespace nodo::spatial
