#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

#include <algorithm>

namespace nodo::sop {

/**
 * @brief LatticeSOP - Deforms geometry using a 3D lattice
 *
 * Creates a 3D lattice that can deform geometry via trilinear interpolation.
 * Currently implements lattice bounds calculation and prepares for future
 * deformation support.
 *
 * Full implementation requires:
 * - Second input for deformed lattice points
 * - Interactive lattice manipulation UI
 * - Lattice point attribute storage
 */
class LatticeSOP : public SOPNode {
private:
  static constexpr int NODE_VERSION = 1;

public:
  explicit LatticeSOP(const std::string& name = "lattice")
      : SOPNode(name, "Lattice") {
    // Create input port
    input_ports_.add_port("0", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);

    // Lattice divisions
    register_parameter(
        define_int_parameter("divisions_x", 3)
            .label("Divisions X")
            .range(2, 20)
            .category("Lattice")
            .description("Number of lattice divisions along X axis")
            .build());

    register_parameter(
        define_int_parameter("divisions_y", 3)
            .label("Divisions Y")
            .range(2, 20)
            .category("Lattice")
            .description("Number of lattice divisions along Y axis")
            .build());

    register_parameter(
        define_int_parameter("divisions_z", 3)
            .label("Divisions Z")
            .range(2, 20)
            .category("Lattice")
            .description("Number of lattice divisions along Z axis")
            .build());

    // Auto bounds
    register_parameter(
        define_bool_parameter("auto_bounds", true)
            .label("Auto Bounds")
            .category("Lattice")
            .description("Automatically fit lattice to input geometry bounds")
            .build());

    // Interpolation mode
    register_parameter(define_int_parameter("mode", 0)
                           .label("Mode")
                           .options({"Trilinear", "Nearest"})
                           .category("Deformation")
                           .description("Interpolation method for deformation")
                           .build());
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    auto input_geo = get_input_data(0);
    if (!input_geo) {
      set_error("LatticeSOP requires input geometry");
      return nullptr;
    }

    // Get parameters
    const int div_x = get_parameter<int>("divisions_x", 3);
    const int div_y = get_parameter<int>("divisions_y", 3);
    const int div_z = get_parameter<int>("divisions_z", 3);
    const bool auto_bounds = get_parameter<bool>("auto_bounds", true);
    // const int mode = get_parameter<int>("mode", 0);

    // Clone geometry
    auto result = std::make_shared<core::GeometryContainer>(input_geo->clone());

    // Get positions
    auto* positions = result->get_point_attribute_typed<core::Vec3f>("P");
    if (positions == nullptr) {
      set_error("LatticeSOP requires position attribute 'P'");
      return nullptr;
    }

    // Calculate bounding box if auto bounds
    core::Vec3f min_bound(0.0F, 0.0F, 0.0F);
    core::Vec3f max_bound(1.0F, 1.0F, 1.0F);

    if (auto_bounds && positions->size() > 0) {
      min_bound = (*positions)[0];
      max_bound = (*positions)[0];

      for (size_t i = 1; i < positions->size(); ++i) {
        const auto& pos = (*positions)[i];
        min_bound.x() = std::min(min_bound.x(), pos.x());
        min_bound.y() = std::min(min_bound.y(), pos.y());
        min_bound.z() = std::min(min_bound.z(), pos.z());
        max_bound.x() = std::max(max_bound.x(), pos.x());
        max_bound.y() = std::max(max_bound.y(), pos.y());
        max_bound.z() = std::max(max_bound.z(), pos.z());
      }

      // Add 10% padding
      const core::Vec3f padding = (max_bound - min_bound) * 0.1F;
      min_bound -= padding;
      max_bound += padding;
    }

    // TODO: Implement lattice deformation
    //
    // Full implementation would:
    // 1. Create lattice grid: (div_x+1) * (div_y+1) * (div_z+1) control points
    // 2. Initialize lattice points in regular grid between min/max bounds
    // 3. If second input exists, use it for deformed lattice positions
    // 4. For each geometry point:
    //    a. Map to lattice coordinates [0, div_x] × [0, div_y] × [0, div_z]
    //    b. Find 8 surrounding lattice points
    //    c. Apply trilinear interpolation:
    //       - Interpolate along X for 4 edge pairs → 4 points
    //       - Interpolate along Y for 2 face pairs → 2 points
    //       - Interpolate along Z for final position → 1 point
    //    d. Apply displacement from lattice deformation
    //
    // Trilinear interpolation formula:
    //   P(u,v,w) = (1-u)(1-v)(1-w)P000 + u(1-v)(1-w)P100 +
    //              (1-u)v(1-w)P010 + uv(1-w)P110 +
    //              (1-u)(1-v)wP001 + u(1-v)wP101 +
    //              (1-u)vwP011 + uvwP111
    //   where u,v,w ∈ [0,1] are fractional lattice coordinates

    // Suppress unused parameter warnings
    static_cast<void>(div_x);
    static_cast<void>(div_y);
    static_cast<void>(div_z);
    static_cast<void>(min_bound);
    static_cast<void>(max_bound);

    // For now, pass through geometry unchanged
    // Actual deformation will be implemented when we have:
    // - UI for lattice manipulation
    // - Second input support for deformed lattice
    // - Attribute system for lattice state storage

    return result;
  }
};

} // namespace nodo::sop
