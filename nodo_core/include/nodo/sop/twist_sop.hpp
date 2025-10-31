#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief TwistSOP - Twists geometry around an axis
 *
 * TODO: Full twist deformation implementation
 * Currently passes through geometry unchanged.
 */
class TwistSOP : public SOPNode {
private:
  static constexpr int NODE_VERSION = 1;

public:
  explicit TwistSOP(const std::string &name = "twist")
      : SOPNode(name, "Twist") {

    // Twist angle in degrees per unit
    register_parameter(define_float_parameter("angle", 90.0F)
                           .label("Angle")
                           .range(-360.0F, 360.0F)
                           .category("Deformation")
                           .build());

    // Axis to twist around
    register_parameter(define_int_parameter("axis", 1)
                           .label("Axis")
                           .options({"X", "Y", "Z"})
                           .category("Deformation")
                           .build());

    // Twist origin
    register_parameter(define_float_parameter("origin", 0.0F)
                           .label("Origin")
                           .range(-10.0F, 10.0F)
                           .category("Deformation")
                           .build());

    // Twist rate
    register_parameter(define_int_parameter("rate", 0)
                           .label("Rate")
                           .options({"Linear", "Squared"})
                           .category("Deformation")
                           .build());
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    auto input_geo = get_input_data("geometry");
    if (!input_geo) {
      set_error("TwistSOP requires input geometry");
      return nullptr;
    }

    // TODO: Implement twist deformation algorithm
    // For now, pass through unchanged
    return std::make_shared<core::GeometryContainer>(input_geo->clone());
  }
};

} // namespace nodo::sop
