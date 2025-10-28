#pragma once

#include "../core/geometry_container.hpp"
#include "../geometry/plane_generator.hpp"
#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief Plane/grid generator SOP node
 */
class PlaneSOP : public SOPNode {
private:
  static constexpr float DEFAULT_WIDTH = 2.0F;
  static constexpr float DEFAULT_HEIGHT = 2.0F;
  static constexpr int DEFAULT_SEGMENTS = 1;

public:
  explicit PlaneSOP(const std::string &node_name = "plane")
      : SOPNode(node_name, "PlaneSOP") {
    set_parameter("width", DEFAULT_WIDTH);
    set_parameter("height", DEFAULT_HEIGHT);
    set_parameter("width_segments", DEFAULT_SEGMENTS);
    set_parameter("height_segments", DEFAULT_SEGMENTS);
  }

  void set_size(float width, float height) {
    set_parameter("width", width);
    set_parameter("height", height);
  }

  void set_resolution(int width_segments, int height_segments) {
    set_parameter("width_segments", width_segments);
    set_parameter("height_segments", height_segments);
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    const float width = get_parameter<float>("width", DEFAULT_WIDTH);
    const float height = get_parameter<float>("height", DEFAULT_HEIGHT);
    const int width_segments =
        get_parameter<int>("width_segments", DEFAULT_SEGMENTS);
    const int height_segments =
        get_parameter<int>("height_segments", DEFAULT_SEGMENTS);

    try {
      auto result = geometry::PlaneGenerator::generate(
          static_cast<double>(width), static_cast<double>(height),
          width_segments, height_segments);

      if (!result.has_value()) {
        set_error("Plane generation failed");
        return nullptr;
      }

      return std::make_shared<core::GeometryContainer>(
          std::move(result.value()));

    } catch (const std::exception &exception) {
      set_error("Exception during plane generation: " +
                std::string(exception.what()));
      return nullptr;
    }
  }
};

} // namespace nodo::sop
