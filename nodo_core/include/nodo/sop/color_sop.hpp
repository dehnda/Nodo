#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"
#include <random>

namespace nodo::sop {

/**
 * @brief Color SOP - Sets vertex/point colors
 *
 * Assigns color values to geometry elements. Supports:
 * - Constant color (uniform)
 * - Random colors (per element)
 * - Color ramp (along axis)
 *
 * Creates or modifies the "Cd" (color/diffuse) attribute
 */
class ColorSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit ColorSOP(const std::string &node_name = "color")
      : SOPNode(node_name, "Color") {
    // Single geometry input
    input_ports_.add_port("0", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);

    // Color mode
    register_parameter(
        define_int_parameter("color_mode", 0)
            .label("Color Mode")
            .options({"Constant", "Random", "Ramp"})
            .category("Color")
            .description(
                "Color assignment method (constant, random, or gradient ramp)")
            .build());

    // Universal class parameter (customized for color - only
    // Point/Vertex/Primitive)
    register_parameter(
        define_int_parameter("class", 0)
            .label("Class")
            .options({"Point", "Vertex", "Primitive"})
            .category("Color")
            .description("Geometry element type to assign colors to")
            .build());

    // Constant color
    register_parameter(define_float_parameter("color_r", 1.0F)
                           .label("Color R")
                           .range(0.0, 1.0)
                           .category("Constant")
                           .visible_when("color_mode", 0)
                           .description("Red component of constant color (0-1)")
                           .build());

    register_parameter(
        define_float_parameter("color_g", 1.0F)
            .label("Color G")
            .range(0.0, 1.0)
            .category("Constant")
            .visible_when("color_mode", 0)
            .description("Green component of constant color (0-1)")
            .build());

    register_parameter(
        define_float_parameter("color_b", 1.0F)
            .label("Color B")
            .range(0.0, 1.0)
            .category("Constant")
            .visible_when("color_mode", 0)
            .description("Blue component of constant color (0-1)")
            .build());

    // Random seed
    register_parameter(define_int_parameter("seed", 0)
                           .label("Seed")
                           .range(0, 10000)
                           .category("Random")
                           .visible_when("color_mode", 1)
                           .description("Random seed for color generation")
                           .build());

    // Ramp start color
    register_parameter(
        define_float_parameter("ramp_start_r", 0.0F)
            .label("Start R")
            .range(0.0, 1.0)
            .category("Ramp")
            .visible_when("color_mode", 2)
            .description("Red component of ramp start color (0-1)")
            .build());

    register_parameter(
        define_float_parameter("ramp_start_g", 0.0F)
            .label("Start G")
            .range(0.0, 1.0)
            .category("Ramp")
            .visible_when("color_mode", 2)
            .description("Green component of ramp start color (0-1)")
            .build());

    register_parameter(
        define_float_parameter("ramp_start_b", 1.0F)
            .label("Start B")
            .range(0.0, 1.0)
            .category("Ramp")
            .visible_when("color_mode", 2)
            .description("Blue component of ramp start color (0-1)")
            .build());

    // Ramp end color
    register_parameter(define_float_parameter("ramp_end_r", 1.0F)
                           .label("End R")
                           .range(0.0, 1.0)
                           .category("Ramp")
                           .visible_when("color_mode", 2)
                           .description("Red component of ramp end color (0-1)")
                           .build());

    register_parameter(
        define_float_parameter("ramp_end_g", 0.0F)
            .label("End G")
            .range(0.0, 1.0)
            .category("Ramp")
            .visible_when("color_mode", 2)
            .description("Green component of ramp end color (0-1)")
            .build());

    register_parameter(
        define_float_parameter("ramp_end_b", 0.0F)
            .label("End B")
            .range(0.0, 1.0)
            .category("Ramp")
            .visible_when("color_mode", 2)
            .description("Blue component of ramp end color (0-1)")
            .build());

    // Ramp axis
    register_parameter(
        define_int_parameter("ramp_axis", 1)
            .label("Ramp Axis")
            .options({"X", "Y", "Z"})
            .category("Ramp")
            .visible_when("color_mode", 2)
            .description("Axis along which to apply color gradient")
            .build());
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    auto input = get_input_data(0);
    if (!input) {
      set_error("Color node requires input geometry");
      return nullptr;
    }

    // Clone input
    auto output = std::make_shared<core::GeometryContainer>(input->clone());

    // Get parameters
    const int color_mode = get_parameter<int>("color_mode", 0);
    const int attr_class = get_parameter<int>("class", 0);

    // Ensure Cd attribute exists on the correct class
    switch (attr_class) {
    case 0: // Point
      if (!output->has_point_attribute("Cd")) {
        output->add_point_attribute("Cd", core::AttributeType::VEC3F);
      }
      break;
    case 1: // Vertex
      if (!output->has_vertex_attribute("Cd")) {
        output->add_vertex_attribute("Cd", core::AttributeType::VEC3F);
      }
      break;
    case 2: // Primitive
      if (!output->has_primitive_attribute("Cd")) {
        output->add_primitive_attribute("Cd", core::AttributeType::VEC3F);
      }
      break;
    }

    // Apply color based on mode
    switch (color_mode) {
    case 0: // Constant
      apply_constant_color(output, attr_class);
      break;
    case 1: // Random
      apply_random_color(output, attr_class);
      break;
    case 2: // Ramp
      apply_ramp_color(output, attr_class);
      break;
    }

    return output;
  }

private:
  void apply_constant_color(std::shared_ptr<core::GeometryContainer> &geo,
                            int attr_class) {
    const float r = get_parameter<float>("color_r", 1.0F);
    const float g = get_parameter<float>("color_g", 1.0F);
    const float b = get_parameter<float>("color_b", 1.0F);
    const core::Vec3f color(r, g, b);

    switch (attr_class) {
    case 0: { // Point
      auto *cd = geo->get_point_attribute_typed<core::Vec3f>("Cd");
      if (cd) {
        std::fill(cd->values_writable().begin(), cd->values_writable().end(),
                  color);
      }
      break;
    }
    case 1: { // Vertex
      auto *cd = geo->get_vertex_attribute_typed<core::Vec3f>("Cd");
      if (cd) {
        std::fill(cd->values_writable().begin(), cd->values_writable().end(),
                  color);
      }
      break;
    }
    case 2: { // Primitive
      auto *cd = geo->get_primitive_attribute_typed<core::Vec3f>("Cd");
      if (cd) {
        std::fill(cd->values_writable().begin(), cd->values_writable().end(),
                  color);
      }
      break;
    }
    }
  }

  void apply_random_color(std::shared_ptr<core::GeometryContainer> &geo,
                          int attr_class) {
    const int seed = get_parameter<int>("seed", 0);
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(0.0F, 1.0F);

    switch (attr_class) {
    case 0: { // Point
      auto *cd = geo->get_point_attribute_typed<core::Vec3f>("Cd");
      if (cd) {
        for (size_t i = 0; i < cd->size(); ++i) {
          (*cd)[i] = core::Vec3f(dist(rng), dist(rng), dist(rng));
        }
      }
      break;
    }
    case 1: { // Vertex
      auto *cd = geo->get_vertex_attribute_typed<core::Vec3f>("Cd");
      if (cd) {
        for (size_t i = 0; i < cd->size(); ++i) {
          (*cd)[i] = core::Vec3f(dist(rng), dist(rng), dist(rng));
        }
      }
      break;
    }
    case 2: { // Primitive
      auto *cd = geo->get_primitive_attribute_typed<core::Vec3f>("Cd");
      if (cd) {
        for (size_t i = 0; i < cd->size(); ++i) {
          (*cd)[i] = core::Vec3f(dist(rng), dist(rng), dist(rng));
        }
      }
      break;
    }
    }
  }

  void apply_ramp_color(std::shared_ptr<core::GeometryContainer> &geo,
                        int attr_class) {
    const float start_r = get_parameter<float>("ramp_start_r", 0.0F);
    const float start_g = get_parameter<float>("ramp_start_g", 0.0F);
    const float start_b = get_parameter<float>("ramp_start_b", 1.0F);
    const float end_r = get_parameter<float>("ramp_end_r", 1.0F);
    const float end_g = get_parameter<float>("ramp_end_g", 0.0F);
    const float end_b = get_parameter<float>("ramp_end_b", 0.0F);
    const int axis = get_parameter<int>("ramp_axis", 1);

    const core::Vec3f start_color(start_r, start_g, start_b);
    const core::Vec3f end_color(end_r, end_g, end_b);

    // Need positions to calculate ramp
    if (!geo->has_point_attribute("P")) {
      return;
    }

    auto *positions = geo->get_point_attribute_typed<core::Vec3f>("P");
    if (!positions) {
      return;
    }

    // Find bounding box for normalization
    float min_val = std::numeric_limits<float>::max();
    float max_val = std::numeric_limits<float>::lowest();

    for (size_t i = 0; i < positions->size(); ++i) {
      float val = (*positions)[i][axis];
      min_val = std::min(min_val, val);
      max_val = std::max(max_val, val);
    }

    const float range = max_val - min_val;
    if (range < 1e-6F) {
      // No range, use start color
      apply_constant_color(geo, attr_class);
      return;
    }

    // Apply ramp based on class
    switch (attr_class) {
    case 0: { // Point
      auto *cd = geo->get_point_attribute_typed<core::Vec3f>("Cd");
      if (cd) {
        for (size_t i = 0; i < cd->size(); ++i) {
          float t = ((*positions)[i][axis] - min_val) / range;
          t = std::clamp(t, 0.0F, 1.0F);
          (*cd)[i] = start_color * (1.0F - t) + end_color * t;
        }
      }
      break;
    }
    case 1: { // Vertex - use point positions
      auto *cd = geo->get_vertex_attribute_typed<core::Vec3f>("Cd");
      if (cd) {
        for (size_t i = 0; i < cd->size(); ++i) {
          int pt_idx = geo->topology().get_vertex_point(i);
          if (pt_idx >= 0 && static_cast<size_t>(pt_idx) < positions->size()) {
            float t = ((*positions)[pt_idx][axis] - min_val) / range;
            t = std::clamp(t, 0.0F, 1.0F);
            (*cd)[i] = start_color * (1.0F - t) + end_color * t;
          }
        }
      }
      break;
    }
    case 2: { // Primitive - use centroid
      auto *cd = geo->get_primitive_attribute_typed<core::Vec3f>("Cd");
      if (cd) {
        for (size_t prim_idx = 0; prim_idx < cd->size(); ++prim_idx) {
          // Calculate primitive centroid
          const auto &prim_verts =
              geo->topology().get_primitive_vertices(prim_idx);
          core::Vec3f centroid(0.0F, 0.0F, 0.0F);
          int vert_count = 0;

          for (int vert_idx : prim_verts) {
            int pt_idx = geo->topology().get_vertex_point(vert_idx);
            if (pt_idx >= 0 &&
                static_cast<size_t>(pt_idx) < positions->size()) {
              centroid += (*positions)[pt_idx];
              vert_count++;
            }
          }

          if (vert_count > 0) {
            centroid /= static_cast<float>(vert_count);
            float t = (centroid[axis] - min_val) / range;
            t = std::clamp(t, 0.0F, 1.0F);
            (*cd)[prim_idx] = start_color * (1.0F - t) + end_color * t;
          }
        }
      }
      break;
    }
    }
  }
};

} // namespace nodo::sop
