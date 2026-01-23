#pragma once

#include "../core/geometry_container.hpp"
#include "../processing/curvature.hpp"
#include "sop_node.hpp"

#include <fmt/core.h>

namespace nodo::sop {

/**
 * @brief Curvature SOP - Analyzes mesh curvature
 *
 * Computes various curvature types and stores them as point attributes.
 * Uses PMP library's curvature analysis algorithms.
 *
 * Output attributes:
 * - mean_curvature: Average of principal curvatures (H = (k1 + k2) / 2)
 * - gaussian_curvature: Product of principal curvatures (K = k1 * k2)
 * - min_curvature: Minimum principal curvature (k1)
 * - max_curvature: Maximum principal curvature (k2)
 *
 * Useful for:
 * - Visualization (colored by curvature)
 * - Adaptive operations (denser sampling in high curvature areas)
 * - Feature detection (high curvature = sharp features)
 * - Procedural texturing (curvature-based patterns)
 */
class CurvatureSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit CurvatureSOP(const std::string& node_name = "curvature") : SOPNode(node_name, "Curvature") {
    // Add input port
    input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

    // Curvature type
    register_parameter(define_int_parameter("curvature_type", 0)
                           .label("Curvature Type")
                           .options({"Mean", "Gaussian", "Min", "Max", "All"})
                           .category("Curvature")
                           .description("Type of curvature to compute")
                           .build());

    // Use absolute values
    register_parameter(define_bool_parameter("use_absolute", false)
                           .label("Absolute Values")
                           .category("Curvature")
                           .description("Use absolute curvature values (easier to visualize)")
                           .build());

    // Smooth curvature
    register_parameter(define_bool_parameter("smooth", true)
                           .label("Smooth")
                           .category("Curvature")
                           .description("Smooth curvature values for better quality")
                           .build());

    // Smoothing iterations
    register_parameter(define_int_parameter("smoothing_iterations", 2)
                           .label("Smoothing Iterations")
                           .range(0, 10)
                           .category("Curvature")
                           .description("Number of smoothing iterations (if smooth enabled)")
                           .build());
  }

  InputConfig get_input_config() const override { return InputConfig(InputType::SINGLE, 1, 1, 0); }

protected:
  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override {
    auto filter_result = apply_group_filter(0, core::ElementClass::POINT, false);
    if (!filter_result.is_success()) {
      return {"CurvatureSOP: No input geometry."};
    }
    const auto& input_data = filter_result.get_value();

    // Get parameters
    processing::CurvatureParams params;

    int type_index = get_parameter<int>("curvature_type", 0);
    switch (type_index) {
      case 0:
        params.type = processing::CurvatureType::MEAN;
        break;
      case 1:
        params.type = processing::CurvatureType::GAUSSIAN;
        break;
      case 2:
        params.type = processing::CurvatureType::MIN;
        break;
      case 3:
        params.type = processing::CurvatureType::MAX;
        break;
      case 4:
        params.type = processing::CurvatureType::ALL;
        break;
      default:
        params.type = processing::CurvatureType::MEAN;
    }

    params.use_absolute = get_parameter<bool>("use_absolute", false);
    params.smooth = get_parameter<bool>("smooth", true);
    params.smoothing_iterations = get_parameter<int>("smoothing_iterations", 2);

    fmt::print("CurvatureSOP: Computing curvature (type={}, absolute={}, smooth={})\n", type_index, params.use_absolute,
               params.smooth);

    // Compute curvature
    auto result = processing::Curvature::compute(*input_data, params);

    if (result) {
      fmt::print("CurvatureSOP: Curvature computation complete\n");
      return std::make_shared<core::GeometryContainer>(std::move(*result));
    } else {
      fmt::print("CurvatureSOP: Curvature computation failed\n");
      return {"Curvature computation failed"};
    }
  }
};

} // namespace nodo::sop
