#pragma once

#include "../core/geometry_container.hpp"
#include "../processing/parameterization.hpp"
#include "sop_node.hpp"

#include <fmt/core.h>

namespace nodo::sop {

/**
 * @brief Parameterize SOP - UV parameterization for meshes
 *
 * Computes UV coordinates for meshes using different parameterization methods:
 * - Harmonic: Discrete harmonic parameterization (works on general polygon
 * meshes)
 * - LSCM: Least Squares Conformal Maps (triangle meshes only, better quality)
 *
 * Requirements:
 * - Input mesh must have at least one boundary (open mesh)
 * - For LSCM, mesh must be triangulated
 *
 * Note: Closed meshes (sphere, cube, etc.) need to be cut open first.
 * Workflow for closed meshes:
 * 1. Use Blast or Split node to delete some faces and create a boundary
 * 2. Connect to Parameterize node
 *
 * Output:
 * - Creates "uv" point attribute (Vec2f) with UV coordinates
 *
 * Useful for:
 * - Texture mapping preparation
 * - 2D pattern generation
 * - Surface analysis
 * - Flattening 3D geometry for processing
 */
class ParameterizeSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit ParameterizeSOP(const std::string& node_name = "parameterize") : SOPNode(node_name, "Parameterize") {
    // Add input port
    input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

    // Parameterization method
    register_parameter(define_int_parameter("method", 0)
                           .label("Method")
                           .options({"Harmonic", "LSCM"})
                           .category("Parameterization")
                           .description("UV parameterization method")
                           .build());

    // Use uniform weights (Harmonic only)
    register_parameter(define_bool_parameter("use_uniform_weights", false)
                           .label("Use Uniform Weights")
                           .category("Parameterization")
                           .description("Use uniform Laplacian weights instead "
                                        "of cotangent (Harmonic only)")
                           .build());

    // UV attribute name
    register_parameter(define_string_parameter("uv_attribute", "uv")
                           .label("UV Attribute")
                           .category("Output")
                           .description("Name of the UV attribute to create")
                           .build());
  }

  InputConfig get_input_config() const override { return InputConfig(InputType::SINGLE, 1, 1, 0); }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    // Get input
    auto input_data = get_input_data(0);
    if (!input_data) {
      fmt::print("ParameterizeSOP: No input geometry\n");
      return nullptr;
    }

    // Get parameters
    processing::ParameterizationParams params;

    int method_index = get_parameter<int>("method", 0);
    switch (method_index) {
      case 0:
        params.method = processing::ParameterizationMethod::Harmonic;
        params.use_uniform_weights = get_parameter<bool>("use_uniform_weights", false);
        break;
      case 1:
        params.method = processing::ParameterizationMethod::LSCM;
        break;
      default:
        params.method = processing::ParameterizationMethod::Harmonic;
        break;
    }

    params.uv_attribute_name = get_parameter<std::string>("uv_attribute", "uv");

    // Compute parameterization
    std::string error;
    auto result = processing::Parameterization::parameterize(*input_data, params, &error);

    if (!result) {
      fmt::print("ParameterizeSOP: {}\n", error);
      return nullptr;
    }

    return std::make_shared<core::GeometryContainer>(std::move(*result));
  }
};

} // namespace nodo::sop
