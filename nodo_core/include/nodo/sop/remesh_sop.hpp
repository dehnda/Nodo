#pragma once

#include "../core/geometry_container.hpp"
#include "../processing/remeshing.hpp"
#include "sop_node.hpp"

#include <fmt/core.h>

namespace nodo::sop {

/**
 * @brief Remesh SOP - Creates uniform or adaptive mesh triangulation
 *
 * Remeshes input geometry to create uniform, well-shaped triangles.
 * Uses PMP library's remeshing algorithms:
 * - Uniform remeshing: constant edge length
 * - Adaptive remeshing: adjust to curvature
 *
 * Useful for:
 * - Cleaning up imported meshes
 * - Preparing geometry for simulation
 * - Creating uniform tessellation for displacement
 */
class RemeshSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit RemeshSOP(const std::string& node_name = "remesh") : SOPNode(node_name, "Remesh") {
    // Add input port
    input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

    // Target edge length
    register_parameter(define_float_parameter("target_edge_length", 0.5F)
                           .label("Target Edge Length")
                           .range(0.001F, 10.0F)
                           .category("Remeshing")
                           .description("Desired edge length for uniform triangulation")
                           .build());

    // Iterations
    register_parameter(define_int_parameter("iterations", 10)
                           .label("Iterations")
                           .range(1, 100)
                           .category("Remeshing")
                           .description("Number of remeshing iterations (more = better "
                                        "quality but slower)")
                           .build());

    // Preserve boundaries
    register_parameter(define_int_parameter("preserve_boundaries", 1)
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
    register_parameter(define_float_parameter("feature_angle", 30.0F)
                           .label("Feature Angle")
                           .range(0.0F, 180.0F)
                           .category("Options")
                           .visible_when("preserve_sharp_edges", 1)
                           .description("Angle threshold for detecting sharp edges (degrees)")
                           .build());

    // Adaptive sizing
    register_parameter(define_int_parameter("adaptive", 0)
                           .label("Adaptive")
                           .options({"Off", "On"})
                           .category("Advanced")
                           .description("Use adaptive edge lengths based on curvature")
                           .build());
  }

  InputConfig get_input_config() const override { return InputConfig(InputType::SINGLE, 1, 1, 0); }

protected:
  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override {
    // Get input geometry
    auto input = get_input_data(0);

    // Debug output
    fmt::print("RemeshSOP::execute() - input pointer: {}\n", input ? "valid" : "nullptr");

    if (!input) {
      return {"No input geometry"};
    }

    fmt::print("RemeshSOP::execute() - input has {} points, {} prims\n", input->point_count(),
               input->primitive_count());

    // Get parameters
    processing::RemeshingParams params;
    params.use_adaptive = (get_parameter<int>("adaptive", 0) == 1);
    params.target_edge_length = get_parameter<float>("target_edge_length", 0.1F);
    params.iterations = get_parameter<int>("iterations", 10);
    params.preserve_boundaries = (get_parameter<int>("preserve_boundaries", 1) == 1);

    // For now, use default values for adaptive mode parameters
    // TODO: Add UI parameters for these if needed
    params.min_edge_length = params.target_edge_length * 0.5F;
    params.max_edge_length = params.target_edge_length * 2.0F;
    params.approx_error = 0.01F;
    params.smoothing_iterations = 10;

    // Perform remeshing
    std::string error;
    auto result = processing::Remeshing::remesh(*input, params, &error);

    if (!result) {
      set_error(error);
      return {(std::string)error};
    }

    return std::make_shared<core::GeometryContainer>(std::move(*result));
  }
};

} // namespace nodo::sop
