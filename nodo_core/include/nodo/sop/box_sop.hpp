#pragma once

#include "../core/geometry_container.hpp"
#include "../geometry/box_generator.hpp"
#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief Box (Cube) generator SOP node
 */
class BoxSOP : public SOPNode {
private:
  static constexpr int NODE_VERSION = 1;
  static constexpr float DEFAULT_SIZE = 2.0F;
  static constexpr int DEFAULT_SEGMENTS = 1;

  enum class PrimitiveType { Polygon = 0, Points = 1 };

public:
  explicit BoxSOP(const std::string &node_name = "box")
      : SOPNode(node_name, "Box") {

    // Universal: Primitive Type
    register_parameter(define_int_parameter("primitive_type", 0)
                           .label("Primitive Type")
                           .options({"Polygon", "Points"})
                           .category("Universal")
                           .build());

    // Size parameters
    register_parameter(define_float_parameter("width", DEFAULT_SIZE)
                           .label("Width")
                           .range(0.01, 100.0)
                           .category("Size")
                           .description("Width of the box along X axis")
                           .build());

    register_parameter(define_float_parameter("height", DEFAULT_SIZE)
                           .label("Height")
                           .range(0.01, 100.0)
                           .category("Size")
                           .description("Height of the box along Y axis")
                           .build());

    register_parameter(define_float_parameter("depth", DEFAULT_SIZE)
                           .label("Depth")
                           .range(0.01, 100.0)
                           .category("Size")
                           .description("Depth of the box along Z axis")
                           .build());

    // Subdivision parameters
    register_parameter(
        define_int_parameter("width_segments", DEFAULT_SEGMENTS)
            .label("Width Segments")
            .range(1, 100)
            .category("Subdivisions")
            .description("Number of subdivisions along width (X)")
            .build());

    register_parameter(
        define_int_parameter("height_segments", DEFAULT_SEGMENTS)
            .label("Height Segments")
            .range(1, 100)
            .category("Subdivisions")
            .description("Number of subdivisions along height (Y)")
            .build());

    register_parameter(
        define_int_parameter("depth_segments", DEFAULT_SEGMENTS)
            .label("Depth Segments")
            .range(1, 100)
            .category("Subdivisions")
            .description("Number of subdivisions along depth (Z)")
            .build());
  }

  // Generator node - no inputs required
  InputConfig get_input_config() const override {
    return InputConfig(InputType::NONE, 0, 0, 0);
  }

  void set_dimensions(float width, float height, float depth) {
    set_parameter("width", width);
    set_parameter("height", height);
    set_parameter("depth", depth);
  }

  void set_segments(int width_seg, int height_seg, int depth_seg) {
    set_parameter("width_segments", width_seg);
    set_parameter("height_segments", height_seg);
    set_parameter("depth_segments", depth_seg);
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    const auto width = get_parameter<float>("width", DEFAULT_SIZE);
    const auto height = get_parameter<float>("height", DEFAULT_SIZE);
    const auto depth = get_parameter<float>("depth", DEFAULT_SIZE);
    const auto width_segments =
        get_parameter<int>("width_segments", DEFAULT_SEGMENTS);
    const auto height_segments =
        get_parameter<int>("height_segments", DEFAULT_SEGMENTS);
    const auto depth_segments =
        get_parameter<int>("depth_segments", DEFAULT_SEGMENTS);
    const auto primitive_type =
        static_cast<PrimitiveType>(get_parameter<int>("primitive_type", 0));

    try {
      auto result = geometry::BoxGenerator::generate(
          static_cast<double>(width), static_cast<double>(height),
          static_cast<double>(depth), width_segments, height_segments,
          depth_segments);

      if (!result.has_value()) {
        set_error("Box generation failed");
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
      set_error("Exception during box generation: " +
                std::string(exception.what()));
      return nullptr;
    }
  }
};

} // namespace nodo::sop
