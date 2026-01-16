#pragma once

#include "../core/geometry_container.hpp"
#include "../geometry/torus_generator.hpp"
#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief Torus (donut) generator SOP node
 */
class TorusSOP : public SOPNode {
private:
  static constexpr int NODE_VERSION = 1;
  static constexpr float DEFAULT_MAJOR_RADIUS = 1.0F;
  static constexpr float DEFAULT_MINOR_RADIUS = 0.3F;
  static constexpr int DEFAULT_MAJOR_SEGMENTS = 48;
  static constexpr int DEFAULT_MINOR_SEGMENTS = 12;

  enum class PrimitiveType {
    Polygon = 0,
    Points = 1
  };

public:
  explicit TorusSOP(const std::string& node_name = "torus") : SOPNode(node_name, "Torus") {
    // Universal: Primitive Type
    register_parameter(define_int_parameter("primitive_type", 0)
                           .label("Primitive Type")
                           .options({"Polygon", "Points"})
                           .category("Universal")
                           .build());

    // Radius parameters
    register_parameter(define_float_parameter("major_radius", DEFAULT_MAJOR_RADIUS)
                           .label("Major Radius")
                           .range(0.01, 100.0)
                           .category("Size")
                           .description("Distance from torus center to tube center")
                           .build());

    register_parameter(define_float_parameter("minor_radius", DEFAULT_MINOR_RADIUS)
                           .label("Minor Radius")
                           .range(0.01, 100.0)
                           .category("Size")
                           .description("Radius of the tube cross-section")
                           .build());

    // Resolution parameters
    register_parameter(define_int_parameter("major_segments", DEFAULT_MAJOR_SEGMENTS)
                           .label("Major Segments")
                           .range(3, 256)
                           .category("Resolution")
                           .description("Number of segments around the major circle")
                           .build());

    register_parameter(define_int_parameter("minor_segments", DEFAULT_MINOR_SEGMENTS)
                           .label("Minor Segments")
                           .range(3, 128)
                           .category("Resolution")
                           .description("Number of segments around the tube cross-section")
                           .build());
  }

  // Generator node - no inputs required
  InputConfig get_input_config() const override { return InputConfig(InputType::NONE, 0, 0, 0); }

  void set_radii(float major_radius, float minor_radius) {
    set_parameter("major_radius", major_radius);
    set_parameter("minor_radius", minor_radius);
  }

  void set_resolution(int major_segments, int minor_segments) {
    set_parameter("major_segments", major_segments);
    set_parameter("minor_segments", minor_segments);
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    const auto major_radius = get_parameter<float>("major_radius", DEFAULT_MAJOR_RADIUS);
    const auto minor_radius = get_parameter<float>("minor_radius", DEFAULT_MINOR_RADIUS);
    const auto major_segments = get_parameter<int>("major_segments", DEFAULT_MAJOR_SEGMENTS);
    const auto minor_segments = get_parameter<int>("minor_segments", DEFAULT_MINOR_SEGMENTS);
    const auto primitive_type = static_cast<PrimitiveType>(get_parameter<int>("primitive_type", 0));

    try {
      auto result = geometry::TorusGenerator::generate(
          static_cast<double>(major_radius), static_cast<double>(minor_radius), major_segments, minor_segments);

      if (!result.has_value()) {
        set_error("Torus generation failed");
        return nullptr;
      }

      auto container = std::make_shared<core::GeometryContainer>(std::move(result.value()));

      if (primitive_type == PrimitiveType::Points) {
        auto& topology = container->topology();
        topology.set_primitive_count(0);
      }

      return container;

    } catch (const std::exception& exception) {
      set_error("Exception during torus generation: " + std::string(exception.what()));
      return nullptr;
    }
  }
};

} // namespace nodo::sop
