#pragma once

#include "../core/geometry_container.hpp"
#include "../geometry/cylinder_generator.hpp"
#include "sop_node.hpp"
#include "sop_utils.hpp"

namespace nodo::sop {

/**
 * @brief Cylinder generator SOP node
 */
class CylinderSOP : public SOPNode {
private:
  static constexpr int NODE_VERSION = 1;
  static constexpr float DEFAULT_RADIUS = 1.0F;
  static constexpr float DEFAULT_HEIGHT = 2.0F;
  static constexpr int DEFAULT_RADIAL_SEGMENTS = 32;
  static constexpr int DEFAULT_HEIGHT_SEGMENTS = 1;

  enum class PrimitiveType {
    Polygon = 0,
    Points = 1
  };

public:
  explicit CylinderSOP(const std::string& node_name = "cylinder") : SOPNode(node_name, "Cylinder") {
    // Universal: Primitive Type
    register_parameter(define_int_parameter("primitive_type", 0)
                           .label("Primitive Type")
                           .options({"Polygon", "Points"})
                           .category("Universal")
                           .build());

    // Size parameters
    register_parameter(define_float_parameter("radius", DEFAULT_RADIUS)
                           .label("Radius")
                           .range(0.01, 100.0)
                           .category("Size")
                           .description("Radius of the cylinder")
                           .build());

    register_parameter(define_float_parameter("height", DEFAULT_HEIGHT)
                           .label("Height")
                           .range(0.01, 100.0)
                           .category("Size")
                           .description("Height of the cylinder along Y axis")
                           .build());

    // Resolution parameters
    register_parameter(define_int_parameter("radial_segments", DEFAULT_RADIAL_SEGMENTS)
                           .label("Radial Segments")
                           .range(3, 256)
                           .category("Resolution")
                           .description("Number of segments around the circumference")
                           .build());

    register_parameter(define_int_parameter("height_segments", DEFAULT_HEIGHT_SEGMENTS)
                           .label("Height Segments")
                           .range(1, 100)
                           .category("Resolution")
                           .description("Number of segments along the height")
                           .build());

    // Cap options
    register_parameter(define_bool_parameter("top_cap", true)
                           .label("Top Cap")
                           .category("Caps")
                           .description("Enable top cap (circular face at +Y)")
                           .build());

    register_parameter(define_bool_parameter("bottom_cap", true)
                           .label("Bottom Cap")
                           .category("Caps")
                           .description("Enable bottom cap (circular face at -Y)")
                           .build());
  }

  // Generator node - no inputs required
  InputConfig get_input_config() const override { return InputConfig(InputType::NONE, 0, 0, 0); }

  void set_dimensions(float radius, float height) {}

  void set_radius(float radius) { set_parameter("radius", radius); }
  void set_height(float height) { set_parameter("height", height); }

  void set_resolution(int radial_segments, int height_segments) {
    set_parameter("radial_segments", radial_segments);
    set_parameter("height_segments", height_segments);
  }

  void set_caps(bool top, bool bottom) {
    set_parameter("top_cap", top);
    set_parameter("bottom_cap", bottom);
  }

protected:
  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override {
    const auto radius = get_parameter<float>("radius", DEFAULT_RADIUS);
    const auto height = get_parameter<float>("height", DEFAULT_HEIGHT);
    const auto radial_segments = get_parameter<int>("radial_segments", DEFAULT_RADIAL_SEGMENTS);
    const auto height_segments = get_parameter<int>("height_segments", DEFAULT_HEIGHT_SEGMENTS);
    const auto top_cap = get_parameter<bool>("top_cap", true);
    const auto bottom_cap = get_parameter<bool>("bottom_cap", true);
    const auto primitive_type = static_cast<PrimitiveType>(get_parameter<int>("primitive_type", 0));

    try {
      auto result = geometry::CylinderGenerator::generate(static_cast<double>(radius), static_cast<double>(height),
                                                          radial_segments, height_segments, top_cap, bottom_cap);

      if (!result.has_value()) {
        return {"Cylinder generation failed"};
      }

      // Hard edges
      sop::utils::compute_hard_edge_normals(result.value(), true);

      auto container = std::make_shared<core::GeometryContainer>(std::move(result.value()));

      if (primitive_type == PrimitiveType::Points) {
        auto& topology = container->topology();
        topology.set_primitive_count(0);
      }

      return container;

    } catch (const std::exception& exception) {
      set_error("Exception during cylinder generation: " + std::string(exception.what()));
      return {(std::string) "Exception during cylinder generation: " + exception.what()};
    }
  }
};

} // namespace nodo::sop
