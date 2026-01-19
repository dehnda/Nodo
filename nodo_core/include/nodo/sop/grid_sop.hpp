#pragma once

#include "../core/geometry_container.hpp"
#include "../geometry/plane_generator.hpp"
#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief Grid generator SOP node
 *
 * Generates a planar grid with customizable size and resolution.
 * Default orientation is XZ plane (horizontal).
 */
class GridSOP : public SOPNode {
private:
  static constexpr int NODE_VERSION = 1;
  static constexpr float DEFAULT_SIZE = 10.0F;
  static constexpr int DEFAULT_RESOLUTION = 10;

  enum class PrimitiveType {
    Polygon = 0,
    Points = 1
    /* Edges = 2 (maybe later)*/
  };

  static constexpr const char* primitive_type_to_string(PrimitiveType type) {
    switch (type) {
      case PrimitiveType::Polygon:
        return "Polygon";
      case PrimitiveType::Points:
        return "Points";
      default:
        return "Unknown";
    }
  }

public:
  explicit GridSOP(const std::string& node_name = "grid") : SOPNode(node_name, "Grid") {
    // Universal: Primitive Type
    register_parameter(define_int_parameter("primitive_type", 0)
                           .label("Primitive Type")
                           .options({primitive_type_to_string(PrimitiveType::Polygon),
                                     primitive_type_to_string(PrimitiveType::Points)})
                           .category("Universal")
                           .description("Output geometry type (polygon mesh or point cloud)")
                           .build());

    // Size parameters
    register_parameter(define_float_parameter("size_x", DEFAULT_SIZE)
                           .label("Size X")
                           .range(0.01, 1000.0)
                           .category("Size")
                           .description("Width of the grid in X direction")
                           .build());

    register_parameter(define_float_parameter("size_z", DEFAULT_SIZE)
                           .label("Size Z")
                           .range(0.01, 1000.0)
                           .category("Size")
                           .description("Depth of the grid in Z direction")
                           .build());

    // Resolution parameters
    register_parameter(define_int_parameter("columns", DEFAULT_RESOLUTION)
                           .label("Columns")
                           .range(1, 1000)
                           .category("Resolution")
                           .description("Number of divisions along X axis")
                           .build());

    register_parameter(define_int_parameter("rows", DEFAULT_RESOLUTION)
                           .label("Rows")
                           .range(1, 1000)
                           .category("Resolution")
                           .description("Number of divisions along Z axis")
                           .build());
  }

  // Generator node - no inputs required
  InputConfig get_input_config() const override { return InputConfig(InputType::NONE, 0, 0, 0); }

  void set_size(float size_x, float size_z) {
    set_parameter("size_x", size_x);
    set_parameter("size_z", size_z);
  }

  void set_resolution(int columns, int rows) {
    set_parameter("columns", columns);
    set_parameter("rows", rows);
  }

protected:
  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override {
    const auto width = get_parameter<float>("size_x", DEFAULT_SIZE);
    const auto height = get_parameter<float>("size_z", DEFAULT_SIZE);
    const auto columns = get_parameter<int>("columns", DEFAULT_RESOLUTION);
    const auto rows = get_parameter<int>("rows", DEFAULT_RESOLUTION);
    const auto primitive_type = static_cast<PrimitiveType>(get_parameter<int>("primitive_type", 0));

    try {
      auto result =
          geometry::PlaneGenerator::generate(static_cast<double>(width), static_cast<double>(height), columns, rows);

      if (!result.has_value()) {
        set_error("Grid generation failed");
        return {"Grid generation failed"};
      }

      auto container = std::make_shared<core::GeometryContainer>(std::move(result.value()));

      if (primitive_type == PrimitiveType::Points) {
        auto& topology = container->topology();
        topology.set_primitive_count(0);
      }

      return container;

    } catch (const std::exception& exception) {
      set_error("Exception during grid generation: " + std::string(exception.what()));
      return {(std::string) "Exception during grid generation: " + exception.what()};
    }
  }
};

} // namespace nodo::sop
