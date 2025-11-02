#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief Remesh SOP - Creates uniform mesh triangulation
 *
 * Remeshes input geometry to create uniform, well-shaped triangles.
 * This is a mesh optimization/repair tool useful for:
 * - Cleaning up imported meshes
 * - Preparing geometry for simulation
 * - Creating uniform tessellation for displacement
 *
 * Full implementation requires isotropic remeshing algorithm
 * (e.g., incremental remeshing with edge splits/collapses/flips)
 */
class RemeshSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit RemeshSOP(const std::string &node_name = "remesh")
      : SOPNode(node_name, "Remesh") {
    // Single geometry input
    input_ports_.add_port("0", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);

    // Target edge length
    register_parameter(
        define_float_parameter("target_edge_length", 0.1F)
            .label("Target Edge Length")
            .range(0.001F, 10.0F)
            .category("Remeshing")
            .description("Desired edge length for uniform triangulation")
            .build());

    // Iterations
    register_parameter(
        define_int_parameter("iterations", 10)
            .label("Iterations")
            .range(1, 100)
            .category("Remeshing")
            .description("Number of remeshing iterations (more = better "
                         "quality but slower)")
            .build());

    // Preserve boundaries
    register_parameter(
        define_int_parameter("preserve_boundaries", 1)
            .label("Preserve Boundaries")
            .options({"Off", "On"})
            .category("Options")
            .description("Keep boundary edges fixed during remeshing")
            .build());

    // Preserve sharp features
    register_parameter(define_int_parameter("preserve_sharp_edges", 1)
                           .label("Preserve Sharp Edges")
                           .options({"Off", "On"})
                           .category("Options")
                           .description("Detect and preserve sharp creases")
                           .build());

    // Feature angle
    register_parameter(
        define_float_parameter("feature_angle", 30.0F)
            .label("Feature Angle")
            .range(0.0F, 180.0F)
            .category("Options")
            .visible_when("preserve_sharp_edges", 1)
            .description("Angle threshold for detecting sharp edges (degrees)")
            .build());

    // Adaptive sizing
    register_parameter(
        define_int_parameter("adaptive", 0)
            .label("Adaptive")
            .options({"Off", "On"})
            .category("Advanced")
            .description("Use adaptive edge lengths based on curvature")
            .build());
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    auto input = get_input_data(0);
    if (!input) {
      set_error("Remesh requires input geometry");
      return nullptr;
    }

    // For Phase 1: Return simplified version
    // Full implementation requires sophisticated remeshing algorithm
    auto output = std::make_shared<core::GeometryContainer>(input->clone());

    // Get parameters
    const float target_length =
        get_parameter<float>("target_edge_length", 0.1F);
    const int iterations = get_parameter<int>("iterations", 10);
    const bool preserve_boundaries =
        get_parameter<int>("preserve_boundaries", 1) != 0;
    const bool preserve_sharp =
        get_parameter<int>("preserve_sharp_edges", 1) != 0;

    // TODO: Implement full remeshing algorithm
    // Phase 1: Return unchanged
    // Full remeshing requires:
    // 1. Edge split (too long edges)
    // 2. Edge collapse (too short edges)
    // 3. Edge flip (improve triangle quality)
    // 4. Vertex smoothing (Laplacian or angle-based)
    // 5. Iterate until convergence

    // Could integrate with external library like OpenMesh or libigl
    // for production-quality remeshing

    // Suppress unused variable warnings
    (void)target_length;
    (void)iterations;
    (void)preserve_boundaries;
    (void)preserve_sharp;

    return output;
  }
};

} // namespace nodo::sop
