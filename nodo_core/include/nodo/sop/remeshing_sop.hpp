#pragma once

#include "../core/geometry_container.hpp"
#include "../processing/remeshing.hpp"
#include "sop_node.hpp"

namespace nodo {
namespace sop {

/**
 * @brief Remeshing SOP node
 *
 * Creates uniform or adaptive remeshing of input geometry.
 * Uses PMP library's remeshing algorithms.
 */
class RemeshingSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit RemeshingSOP(const std::string& node_name = "remesh")
      : SOPNode(node_name, "Remesh") {
    // Single input
    // Output is defined in base class

    // Remeshing mode
    register_parameter(
        define_int_parameter("mode", 0, {"Uniform", "Adaptive"})
            .label("Mode")
            .category("Remeshing")
            .description("Uniform: constant edge length, Adaptive: adjust to "
                         "curvature")
            .build());

    // Target edge length (uniform mode)
    register_parameter(
        define_float_parameter("target_edge_length", 1.0F)
            .label("Target Edge Length")
            .category("Remeshing")
            .description("Desired edge length for uniform remeshing")
            .range(0.01F, 10.0F)
            .build());

    // Min edge length (adaptive mode)
    register_parameter(
        define_float_parameter("min_edge_length", 0.1F)
            .label("Min Edge Length")
            .category("Remeshing")
            .description("Minimum edge length for adaptive remeshing")
            .range(0.01F, 5.0F)
            .build());

    // Max edge length (adaptive mode)
    register_parameter(
        define_float_parameter("max_edge_length", 2.0F)
            .label("Max Edge Length")
            .category("Remeshing")
            .description("Maximum edge length for adaptive remeshing")
            .range(0.1F, 10.0F)
            .build());

    // Approximation error (adaptive mode)
    register_parameter(
        define_float_parameter("approx_error", 0.01F)
            .label("Approximation Error")
            .category("Remeshing")
            .description("Maximum approximation error for adaptive "
                         "remeshing")
            .range(0.001F, 1.0F)
            .build());

    // Iterations
    register_parameter(define_int_parameter("iterations", 10)
                           .label("Iterations")
                           .category("Remeshing")
                           .description("Number of remeshing iterations")
                           .range(1, 50)
                           .build());

    // Smoothing iterations
    register_parameter(define_int_parameter("smoothing_iterations", 10)
                           .label("Smoothing Iterations")
                           .category("Remeshing")
                           .description("Number of smoothing iterations")
                           .range(0, 50)
                           .build());

    // Preserve boundaries
    register_parameter(
        define_bool_parameter("preserve_boundaries", true)
            .label("Preserve Boundaries")
            .category("Remeshing")
            .description("Preserve mesh boundaries during remeshing")
            .build());
  }

  InputConfig get_input_config() const override {
    return InputConfig(InputType::SINGLE, 1, 1, 0);
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    // Get input geometry
    auto input = get_input_geometry(0);
    if (!input) {
      set_error("No input geometry");
      return nullptr;
    }

    // Get parameters
    processing::RemeshingParams params;
    params.use_adaptive = (get_parameter<int>("mode", 0) == 1);
    params.target_edge_length =
        get_parameter<float>("target_edge_length", 1.0F);
    params.min_edge_length = get_parameter<float>("min_edge_length", 0.1F);
    params.max_edge_length = get_parameter<float>("max_edge_length", 2.0F);
    params.approx_error = get_parameter<float>("approx_error", 0.01F);
    params.iterations = get_parameter<int>("iterations", 10);
    params.smoothing_iterations =
        get_parameter<int>("smoothing_iterations", 10);
    params.preserve_boundaries =
        get_parameter<bool>("preserve_boundaries", true);

    // Perform remeshing
    std::string error;
    auto result = processing::Remeshing::remesh(*input, params, &error);

    if (!result) {
      set_error(error);
      return nullptr;
    }

    return std::make_shared<core::GeometryContainer>(std::move(*result));
  }
};

} // namespace sop
} // namespace nodo
