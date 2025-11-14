#include "nodo/spatial/enhanced_boolean_ops.hpp"

#include "nodo/geometry/boolean_ops.hpp"
#include "nodo/geometry/mesh_repairer.hpp"
#include "nodo/geometry/mesh_validator.hpp"

namespace nodo::spatial {

thread_local core::Error EnhancedBooleanOps::last_error_{
    core::ErrorCategory::Geometry, core::ErrorCode::Unknown, ""};

std::optional<core::Mesh>
EnhancedBooleanOps::union_meshes(const core::Mesh& mesh_a,
                                 const core::Mesh& mesh_b,
                                 const BooleanParams& params) {
  // Prepare meshes if requested
  auto prepared_a = params.use_mesh_repair
                        ? prepare_mesh_for_boolean(mesh_a, params)
                        : std::make_optional(mesh_a);

  auto prepared_b = params.use_mesh_repair
                        ? prepare_mesh_for_boolean(mesh_b, params)
                        : std::make_optional(mesh_b);

  if (!prepared_a || !prepared_b) {
    last_error_ = core::ErrorUtils::make_error(
        core::ErrorCategory::Geometry, core::ErrorCode::InvalidMesh,
        "Failed to prepare meshes for boolean operation",
        "EnhancedBooleanOps::union_meshes");
    return std::nullopt;
  }

  // Build BVH if requested (for future optimizations)
  std::unique_ptr<BVH> bvh_a;
  std::unique_ptr<BVH> bvh_b;
  if (params.build_bvh) {
    bvh_a = build_mesh_bvh(*prepared_a, params);
    bvh_b = build_mesh_bvh(*prepared_b, params);
  }

  // Perform the boolean operation using existing implementation
  auto result = geometry::BooleanOps::union_meshes(*prepared_a, *prepared_b);

  if (!result) {
    last_error_ = core::ErrorUtils::make_error(
        core::ErrorCategory::Geometry, core::ErrorCode::BooleanOperationFailed,
        "Boolean union operation failed", "EnhancedBooleanOps::union_meshes");
    return std::nullopt;
  }

  // Post-process result if requested
  if (params.ensure_manifold) {
    return post_process_result(*result, params);
  }

  return result;
}

std::optional<core::Mesh>
EnhancedBooleanOps::intersect_meshes(const core::Mesh& mesh_a,
                                     const core::Mesh& mesh_b,
                                     const BooleanParams& params) {
  // Prepare meshes if requested
  auto prepared_a = params.use_mesh_repair
                        ? prepare_mesh_for_boolean(mesh_a, params)
                        : std::make_optional(mesh_a);

  auto prepared_b = params.use_mesh_repair
                        ? prepare_mesh_for_boolean(mesh_b, params)
                        : std::make_optional(mesh_b);

  if (!prepared_a || !prepared_b) {
    last_error_ = core::ErrorUtils::make_error(
        core::ErrorCategory::Geometry, core::ErrorCode::InvalidMesh,
        "Failed to prepare meshes for boolean operation",
        "EnhancedBooleanOps::intersect_meshes");
    return std::nullopt;
  }

  // Build BVH if requested
  std::unique_ptr<BVH> bvh_a;
  std::unique_ptr<BVH> bvh_b;
  if (params.build_bvh) {
    bvh_a = build_mesh_bvh(*prepared_a, params);
    bvh_b = build_mesh_bvh(*prepared_b, params);
  }

  // Perform the boolean operation using existing implementation
  auto result =
      geometry::BooleanOps::intersect_meshes(*prepared_a, *prepared_b);

  if (!result) {
    last_error_ = core::ErrorUtils::make_error(
        core::ErrorCategory::Geometry, core::ErrorCode::BooleanOperationFailed,
        "Boolean intersection operation failed",
        "EnhancedBooleanOps::intersect_meshes");
    return std::nullopt;
  }

  // Post-process result if requested
  if (params.ensure_manifold) {
    return post_process_result(*result, params);
  }

  return result;
}

std::optional<core::Mesh>
EnhancedBooleanOps::subtract_meshes(const core::Mesh& mesh_a,
                                    const core::Mesh& mesh_b,
                                    const BooleanParams& params) {
  // Prepare meshes if requested
  auto prepared_a = params.use_mesh_repair
                        ? prepare_mesh_for_boolean(mesh_a, params)
                        : std::make_optional(mesh_a);

  auto prepared_b = params.use_mesh_repair
                        ? prepare_mesh_for_boolean(mesh_b, params)
                        : std::make_optional(mesh_b);

  if (!prepared_a || !prepared_b) {
    last_error_ = core::ErrorUtils::make_error(
        core::ErrorCategory::Geometry, core::ErrorCode::InvalidMesh,
        "Failed to prepare meshes for boolean operation",
        "EnhancedBooleanOps::subtract_meshes");
    return std::nullopt;
  }

  // Build BVH if requested
  std::unique_ptr<BVH> bvh_a, bvh_b;
  if (params.build_bvh) {
    bvh_a = build_mesh_bvh(*prepared_a, params);
    bvh_b = build_mesh_bvh(*prepared_b, params);
  }

  // Perform the boolean operation
  auto result =
      geometry::BooleanOps::difference_meshes(*prepared_a, *prepared_b);

  if (!result) {
    last_error_ = core::ErrorUtils::make_error(
        core::ErrorCategory::Geometry, core::ErrorCode::BooleanOperationFailed,
        "Boolean difference operation failed",
        "EnhancedBooleanOps::subtract_meshes");
    return std::nullopt;
  }

  // Post-process result if requested
  if (params.ensure_manifold) {
    return post_process_result(*result, params);
  }

  return result;
}

bool EnhancedBooleanOps::meshes_intersect(const core::Mesh& mesh_a,
                                          const core::Mesh& mesh_b) {
  // Simple AABB intersection test first
  AABB bounds_a = AABB::from_mesh(mesh_a);
  AABB bounds_b = AABB::from_mesh(mesh_b);

  if (!bounds_a.intersects(bounds_b)) {
    return false;
  }

  // For now, return true if AABBs intersect
  // This could be enhanced with detailed triangle-triangle intersection tests
  return true;
}

std::vector<Eigen::Vector3d>
EnhancedBooleanOps::find_intersection_points(const core::Mesh& mesh_a,
                                             const core::Mesh& mesh_b) {
  std::vector<Eigen::Vector3d> intersection_points;

  // This is a placeholder implementation
  // In a full implementation, this would use BVH to efficiently find
  // triangle-triangle intersections and compute intersection curves

  AABB bounds_a = AABB::from_mesh(mesh_a);
  AABB bounds_b = AABB::from_mesh(mesh_b);

  if (bounds_a.intersects(bounds_b)) {
    // Add the center of the intersection AABB as a simple example
    AABB intersection_bounds = bounds_a;
    intersection_bounds.expand(bounds_b);
    intersection_points.push_back(intersection_bounds.center());
  }

  return intersection_points;
}

std::optional<core::Mesh>
EnhancedBooleanOps::prepare_mesh_for_boolean(const core::Mesh& mesh,
                                             const BooleanParams& params) {
  core::Mesh prepared_mesh = mesh;

  if (params.validate_input) {
    auto validation_result = geometry::MeshValidator::validate(prepared_mesh);

    if (!validation_result.is_valid) {
      // Try to repair the mesh
      geometry::MeshRepairer::RepairOptions repair_options;
      repair_options.vertex_merge_tolerance = params.tolerance;

      auto repair_result =
          geometry::MeshRepairer::repair(prepared_mesh, repair_options);
      if (!repair_result.success) {
        last_error_ = core::ErrorUtils::make_error(
            core::ErrorCategory::Geometry, core::ErrorCode::InvalidMesh,
            "Failed to repair mesh for boolean operation",
            "EnhancedBooleanOps::prepare_mesh_for_boolean");
        return std::nullopt;
      }
    }
  }

  return prepared_mesh;
}

const core::Error& EnhancedBooleanOps::last_error() {
  return last_error_;
}

std::unique_ptr<BVH> EnhancedBooleanOps::build_mesh_bvh(
    const core::Mesh& mesh, [[maybe_unused]] const BooleanParams& params) {
  auto bvh = std::make_unique<BVH>();
  BVH::BuildParams build_params;

  if (bvh->build(mesh, build_params)) {
    return bvh;
  }

  return nullptr;
}

bool EnhancedBooleanOps::validate_mesh_for_boolean(
    const core::Mesh& mesh, const BooleanParams& params) {
  if (mesh.vertices().rows() == 0 || mesh.faces().rows() == 0) {
    return false;
  }

  if (params.validate_input) {
    geometry::MeshValidator validator;
    auto validation_result = validator.validate(mesh);
    return validation_result.is_valid;
  }

  return true;
}

std::optional<core::Mesh>
EnhancedBooleanOps::post_process_result(const core::Mesh& result,
                                        const BooleanParams& params) {
  core::Mesh processed_result = result;

  // Apply mesh repair to ensure manifold result
  geometry::MeshRepairer repairer;
  geometry::MeshRepairer::RepairOptions repair_options;
  repair_options.vertex_merge_tolerance = params.tolerance;

  auto repair_result = repairer.repair(processed_result, repair_options);
  if (!repair_result.success) {
    last_error_ = core::ErrorUtils::make_error(
        core::ErrorCategory::Geometry, core::ErrorCode::InvalidMesh,
        "Failed to post-process boolean operation result",
        "EnhancedBooleanOps::post_process_result");
    return std::nullopt;
  }

  return processed_result;
}

} // namespace nodo::spatial
