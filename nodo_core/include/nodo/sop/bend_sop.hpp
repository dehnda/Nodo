#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief BendSOP - Bends geometry around an axis
 *
 * TODO: Full bend deformation implementation
 * Currently passes through geometry unchanged.
 */
class BendSOP : public SOPNode {
private:
  static constexpr int NODE_VERSION = 1;

public:
  explicit BendSOP(const std::string &name = "bend") : SOPNode(name, "Bend") {

    // Bend angle in degrees
    register_parameter(define_float_parameter("angle", 90.0F)
                           .label("Angle")
                           .range(-360.0F, 360.0F)
                           .category("Deformation")
                           .build());

    // Axis to bend around
    register_parameter(define_int_parameter("axis", 1)
                           .label("Axis")
                           .options({"X", "Y", "Z"})
                           .category("Deformation")
                           .build());

    // Capture origin
    register_parameter(define_float_parameter("capture_origin", 0.0F)
                           .label("Capture Origin")
                           .range(-10.0F, 10.0F)
                           .category("Capture")
                           .build());

    // Capture length
    register_parameter(define_float_parameter("capture_length", 1.0F)
                           .label("Capture Length")
                           .range(0.0F, 10.0F)
                           .category("Capture")
                           .build());
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    auto input_geo = get_input_data("geometry");
    if (!input_geo) {
      set_error("BendSOP requires input geometry");
      return nullptr;
    }

    // TODO: Implement bend deformation algorithm
    // For now, pass through unchanged
    return std::make_shared<core::GeometryContainer>(input_geo->clone());
  }
};

} // namespace nodo::sop
