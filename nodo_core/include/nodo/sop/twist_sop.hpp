#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"
#include <cmath>

namespace nodo::sop {

/**
 * @brief TwistSOP - Twists geometry around an axis
 *
 * Applies a rotational twist deformation along an axis. Points are rotated
 * around the twist axis by an amount proportional to their distance along
 * that axis.
 */
class TwistSOP : public SOPNode {
private:
  static constexpr int NODE_VERSION = 1;
  static constexpr float M_PI_VAL = 3.14159265358979323846F;

public:
  explicit TwistSOP(const std::string &name = "twist")
      : SOPNode(name, "Twist") {

    // Twist angle in degrees per unit
    register_parameter(
        define_float_parameter("angle", 90.0F)
            .label("Angle")
            .range(-360.0F, 360.0F)
            .category("Deformation")
            .description("Twist angle in degrees per unit distance")
            .build());

    // Axis to twist around
    register_parameter(
        define_int_parameter("axis", 1)
            .label("Axis")
            .options({"X", "Y", "Z"})
            .category("Deformation")
            .description("Axis around which to twist the geometry")
            .build());

    // Twist origin
    register_parameter(
        define_float_parameter("origin", 0.0F)
            .label("Origin")
            .range(-10.0F, 10.0F)
            .category("Deformation")
            .description(
                "Position along axis where twist starts (zero rotation)")
            .build());

    // Twist rate
    register_parameter(
        define_int_parameter("rate", 0)
            .label("Rate")
            .options({"Linear", "Squared"})
            .category("Deformation")
            .description("Twist falloff (linear or squared distance)")
            .build());
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    auto input_geo = get_input_data("geometry");
    if (!input_geo) {
      set_error("TwistSOP requires input geometry");
      return nullptr;
    }

    // Get parameters
    const float angle_deg = get_parameter<float>("angle", 90.0F);
    const int axis = get_parameter<int>("axis", 1);
    const float origin = get_parameter<float>("origin", 0.0F);
    const int rate_mode = get_parameter<int>("rate", 0);

    // Convert to radians
    const float angle_rad = angle_deg * M_PI_VAL / 180.0F;

    // Clone geometry
    auto result = std::make_shared<core::GeometryContainer>(input_geo->clone());

    // Get group filtering
    const std::string group_name = get_parameter<std::string>("group", "");
    core::AttributeStorage<int> *group_attr = nullptr;
    const bool use_group = !group_name.empty();

    if (use_group && result->has_point_attribute(group_name)) {
      group_attr = result->get_point_attribute_typed<int>(group_name);
    }

    // Get positions
    auto *positions = result->get_point_attribute_typed<core::Vec3f>("P");
    if (positions == nullptr) {
      set_error("TwistSOP requires position attribute 'P'");
      return nullptr;
    }

    // Determine which axes to rotate
    // axis parameter: 0=X, 1=Y, 2=Z (twist around this axis)
    int twist_axis = axis;
    int axis_u = (twist_axis + 1) % 3; // First perpendicular axis
    int axis_v = (twist_axis + 2) % 3; // Second perpendicular axis

    // Apply twist to each point
    for (size_t i = 0; i < positions->size(); ++i) {
      // Check group filter
      if (use_group && group_attr != nullptr && (*group_attr)[i] == 0) {
        continue;
      }

      auto &pos = (*positions)[i];

      // Distance along twist axis from origin
      const float dist = pos[twist_axis] - origin;

      // Calculate twist amount based on rate
      float twist_amount = 0.0F;
      if (rate_mode == 1) {
        // Squared
        twist_amount = angle_rad * dist * dist;
      } else {
        // Linear (default)
        twist_amount = angle_rad * dist;
      }

      // Apply rotation in the plane perpendicular to twist axis
      const float cos_t = std::cos(twist_amount);
      const float sin_t = std::sin(twist_amount);

      const float u_val = pos[axis_u];
      const float v_val = pos[axis_v];

      pos[axis_u] = u_val * cos_t - v_val * sin_t;
      pos[axis_v] = u_val * sin_t + v_val * cos_t;
    }

    return result;
  }
};

} // namespace nodo::sop
