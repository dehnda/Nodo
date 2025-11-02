#pragma once

#include "../core/geometry_container.hpp"
#include "../geometry/sphere_generator.hpp"
#include "sop_node.hpp"

namespace nodo {
namespace sop {

/**
 * @brief Sphere generator SOP node
 *
 * Generates UV spheres with customizable radius and resolution.
 */
class SphereSOP : public SOPNode {
private:
  static constexpr int NODE_VERSION = 1;
  static constexpr float DEFAULT_RADIUS = 1.0F;
  static constexpr int DEFAULT_SEGMENTS = 32;
  static constexpr int DEFAULT_RINGS = 16;

  enum class PrimitiveType { Polygon = 0, Points = 1 };

public:
  explicit SphereSOP(const std::string &node_name = "sphere")
      : SOPNode(node_name, "Sphere") {

    // Universal: Primitive Type
    register_parameter(define_int_parameter("primitive_type", 0)
                           .label("Primitive Type")
                           .options({"Polygon", "Points"})
                           .category("Universal")
                           .build());

    // Radius parameter
    register_parameter(define_float_parameter("radius", DEFAULT_RADIUS)
                           .label("Radius")
                           .range(0.01, 100.0)
                           .category("Size")
                           .description("Radius of the sphere")
                           .build());

    // Resolution parameters
    register_parameter(
        define_int_parameter("segments", DEFAULT_SEGMENTS)
            .label("Segments")
            .range(3, 256)
            .category("Resolution")
            .description("Number of vertical segments (longitude)")
            .build());

    register_parameter(define_int_parameter("rings", DEFAULT_RINGS)
                           .label("Rings")
                           .range(3, 128)
                           .category("Resolution")
                           .description("Number of horizontal rings (latitude)")
                           .build());
  }

  // Generator node - no inputs required
  int get_min_inputs() const override { return 0; }
  int get_max_inputs() const override { return 0; }

  /**
   * @brief Set sphere radius
   */
  void set_radius(float radius) { set_parameter("radius", radius); }

  /**
   * @brief Set sphere resolution
   */
  void set_resolution(int segments, int rings) {
    set_parameter("segments", segments);
    set_parameter("rings", rings);
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    const auto radius = get_parameter<float>("radius", DEFAULT_RADIUS);
    const auto segments = get_parameter<int>("segments", DEFAULT_SEGMENTS);
    const auto rings = get_parameter<int>("rings", DEFAULT_RINGS);
    const auto primitive_type =
        static_cast<PrimitiveType>(get_parameter<int>("primitive_type", 0));

    try {
      auto result = geometry::SphereGenerator::generate_uv_sphere(
          static_cast<double>(radius), segments, rings);

      if (!result.has_value()) {
        set_error("Sphere generation failed");
        return nullptr;
      }

      auto container =
          std::make_shared<core::GeometryContainer>(std::move(result.value()));

      if (primitive_type == PrimitiveType::Points) {
        auto &topology = container->topology();
        topology.set_primitive_count(0);
      }

      return container;

    } catch (const std::exception &exception) {
      set_error("Exception during sphere generation: " +
                std::string(exception.what()));
      return nullptr;
    }
  }
};

} // namespace sop
} // namespace nodo
