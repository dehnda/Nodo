#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief LatticeSOP - Deforms geometry using a 3D lattice
 *
 * TODO: Full lattice deformation implementation
 * Currently passes through geometry unchanged.
 */
class LatticeSOP : public SOPNode {
private:
  static constexpr int NODE_VERSION = 1;

public:
  explicit LatticeSOP(const std::string &name = "lattice")
      : SOPNode(name, "Lattice") {

    // Lattice divisions
    register_parameter(define_int_parameter("divisions_x", 3)
                           .label("Divisions X")
                           .range(2, 20)
                           .category("Lattice")
                           .build());

    register_parameter(define_int_parameter("divisions_y", 3)
                           .label("Divisions Y")
                           .range(2, 20)
                           .category("Lattice")
                           .build());

    register_parameter(define_int_parameter("divisions_z", 3)
                           .label("Divisions Z")
                           .range(2, 20)
                           .category("Lattice")
                           .build());

    // Auto bounds
    register_parameter(define_bool_parameter("auto_bounds", true)
                           .label("Auto Bounds")
                           .category("Lattice")
                           .build());

    // Interpolation mode
    register_parameter(define_int_parameter("mode", 0)
                           .label("Mode")
                           .options({"Trilinear", "Nearest"})
                           .category("Deformation")
                           .build());
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    auto input_geo = get_input_data("geometry");
    if (!input_geo) {
      set_error("LatticeSOP requires input geometry");
      return nullptr;
    }

    // TODO: Implement lattice deformation algorithm
    // This requires:
    // 1. Creating a 3D lattice grid
    // 2. Mapping geometry points to lattice coordinates
    // 3. Applying trilinear interpolation
    // 4. Supporting lattice point manipulation (second input or attributes)

    // For now, pass through unchanged
    return std::make_shared<core::GeometryContainer>(input_geo->clone());
  }
};

} // namespace nodo::sop
