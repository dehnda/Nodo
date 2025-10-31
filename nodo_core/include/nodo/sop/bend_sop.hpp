#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"
#include <cmath>

namespace nodo::sop {

/**
 * @brief BendSOP - Bends geometry around an axis
 *
 * Applies a circular bend deformation to geometry. Points are transformed
 * from linear space into a circular arc based on their distance along
 * the bend axis.
 */
class BendSOP : public SOPNode {
private:
  static constexpr int NODE_VERSION = 1;
  static constexpr float PI = 3.14159265358979323846F;
  static constexpr float EPSILON = 0.0001F;

public:
  explicit BendSOP(const std::string &name = "bend") : SOPNode(name, "Bend") {

    // Bend angle in degrees
    register_parameter(
        define_float_parameter("angle", 90.0F)
            .label("Angle")
            .range(-360.0F, 360.0F)
            .category("Deformation")
            .description("Bend angle in degrees (positive bends geometry)")
            .build());

    // Axis to bend around
    register_parameter(
        define_int_parameter("axis", 1)
            .label("Axis")
            .options({"X", "Y", "Z"})
            .category("Deformation")
            .description("Axis around which to bend the geometry")
            .build());

    // Capture origin
    register_parameter(
        define_float_parameter("capture_origin", 0.0F)
            .label("Capture Origin")
            .range(-10.0F, 10.0F)
            .category("Capture")
            .description("Starting position along axis for bend region")
            .build());

    // Capture length
    register_parameter(define_float_parameter("capture_length", 1.0F)
                           .label("Capture Length")
                           .range(0.01F, 10.0F)
                           .category("Capture")
                           .description("Length of region along axis to bend")
                           .build());
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    auto input_geo = get_input_data("geometry");
    if (!input_geo) {
      set_error("BendSOP requires input geometry");
      return nullptr;
    }

    // Get parameters
    const float angle_deg = get_parameter<float>("angle", 90.0F);
    const int axis = get_parameter<int>("axis", 1);
    const float capture_origin = get_parameter<float>("capture_origin", 0.0F);
    const float capture_length = get_parameter<float>("capture_length", 1.0F);

    // Convert angle to radians
    const float angle_rad = angle_deg * PI / 180.0F;

    // Early exit if no bend
    if (std::abs(angle_rad) < EPSILON || capture_length < EPSILON) {
      return std::make_shared<core::GeometryContainer>(input_geo->clone());
    }

    // Calculate bend radius: arc_length = radius * angle
    // So radius = arc_length / angle
    const float radius = capture_length / angle_rad;

    // Clone geometry for modification
    auto result = std::make_shared<core::GeometryContainer>(input_geo->clone());

    // Get group filtering - groups are stored as INT attributes
    const std::string group_name = get_parameter<std::string>("group", "");
    core::AttributeStorage<int> *group_attr = nullptr;
    const bool use_group = !group_name.empty();

    if (use_group && result->has_point_attribute(group_name)) {
      group_attr = result->get_point_attribute_typed<int>(group_name);
    }

    // Get position attribute
    auto *positions = result->get_point_attribute_typed<core::Vec3f>("P");
    if (positions == nullptr) {
      set_error("BendSOP requires position attribute 'P'");
      return nullptr;
    }

    // Determine axis indices
    // bend_axis: axis we bend around (perpendicular to bend direction)
    // primary_axis: direction along which bend progresses
    // secondary_axis: offset perpendicular to both
    int primary_axis = 2;   // Z by default (direction of bend)
    int secondary_axis = 0; // X (distance from bend axis)
    // axis parameter (0=X, 1=Y, 2=Z) is the axis we bend around

    if (axis == 0) { // Bend around X
      primary_axis = 1;
      secondary_axis = 2;
    } else if (axis == 2) { // Bend around Z
      primary_axis = 0;
      secondary_axis = 1;
    }
    // axis == 1 (Y) uses the defaults

    // Apply bend to each point
    for (size_t i = 0; i < positions->size(); ++i) {
      // Check group filter
      if (use_group && group_attr != nullptr && (*group_attr)[i] == 0) {
        continue;
      }

      auto &pos = (*positions)[i];

      // Get position along primary axis (direction of bend)
      const float dist = pos[primary_axis] - capture_origin;

      // Only bend points within capture region
      if (dist < 0.0F || dist > capture_length) {
        continue;
      }

      // Calculate normalized position [0, 1]
      const float normalized_dist = dist / capture_length;

      // Calculate angle at this point
      const float local_angle = normalized_dist * angle_rad;

      // Get perpendicular offset (distance from bend axis)
      const float offset = pos[secondary_axis];

      // Calculate effective radius (radius + offset)
      const float effective_radius = radius + offset;

      // Apply circular bend transformation
      // Transform from linear (dist, offset) to circular (arc)
      if (std::abs(effective_radius) > EPSILON) {
        pos[primary_axis] =
            capture_origin + (effective_radius * std::sin(local_angle));
        pos[secondary_axis] = effective_radius * (1.0F - std::cos(local_angle));
      }
    }

    return result;
  }
};

} // namespace nodo::sop
