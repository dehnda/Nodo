#pragma once

#include "../core/geometry_container.hpp"
#include "../processing/smoothing.hpp"
#include "sop_node.hpp"

#include <fmt/core.h>

namespace nodo::sop {

/**
 * @brief Smooth SOP - Smooths mesh surfaces using various algorithms
 *
 * Provides three smoothing methods:
 * - Explicit: Fast iterative Laplacian smoothing
 * - Implicit: Higher quality smoothing via linear system solver
 * - Fairing: Minimizes curvature variation (highest quality)
 *
 * Useful for:
 * - Reducing surface noise
 * - Creating organic shapes
 * - Mesh fairing and cleanup
 */
class SmoothSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit SmoothSOP(const std::string& node_name = "smooth") : SOPNode(node_name, "Smooth") {
    // Add input port
    input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

    // Smoothing method
    register_parameter(define_int_parameter("method", 0)
                           .label("Method")
                           .options({"Explicit", "Implicit", "Fairing"})
                           .category("Smoothing")
                           .description("Explicit = fast, Implicit = quality, "
                                        "Fairing = minimize curvature")
                           .build());

    // Iterations
    register_parameter(define_int_parameter("iterations", 10)
                           .label("Iterations")
                           .range(1, 100)
                           .category("Smoothing")
                           .description("Number of smoothing iterations")
                           .build());

    // Laplacian type
    register_parameter(define_int_parameter("laplace_type", 0)
                           .label("Laplacian Type")
                           .options({"Cotangent", "Uniform"})
                           .category("Advanced")
                           .description("Cotangent = geometry-aware, Uniform = simple average")
                           .build());

    // Timestep (implicit only)
    register_parameter(define_float_parameter("timestep", 0.001F)
                           .label("Timestep")
                           .range(0.0001F, 0.1F)
                           .category("Advanced")
                           .visible_when("method", 1)
                           .description("Time step for implicit smoothing (smaller = more stable)")
                           .build());

    // Rescale (implicit only)
    register_parameter(define_int_parameter("rescale", 1)
                           .label("Rescale")
                           .options({"Off", "On"})
                           .category("Advanced")
                           .visible_when("method", 1)
                           .description("Re-center and re-scale mesh after smoothing")
                           .build());
  }

  InputConfig get_input_config() const override { return InputConfig(InputType::SINGLE, 1, 1, 0); }

protected:
  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override {
    // Get input geometry
    auto input = get_input_data(0);

    if (!input) {
      set_error("No input geometry");
      return {(std::string) "No input geometry"};
    }

    // Get parameters
    processing::SmoothingParams params;
    params.method = get_parameter<int>("method", 0);
    params.iterations = get_parameter<int>("iterations", 10);
    params.use_uniform_laplace = (get_parameter<int>("laplace_type", 0) == 1);
    params.timestep = get_parameter<float>("timestep", 0.001F);
    params.rescale = (get_parameter<int>("rescale", 1) == 1);

    // Perform smoothing
    std::string error;
    auto result = processing::Smoothing::smooth(*input, params, &error);

    if (!result) {
      set_error(error);
      return {(std::string)error};
    }

    return std::make_shared<core::GeometryContainer>(std::move(*result));
  }
};

} // namespace nodo::sop
