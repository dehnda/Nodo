#pragma once

#include "../core/geometry_container.hpp"
#include "../geometry/box_generator.hpp"
#include "sop_node.hpp"

namespace nodeflux::sop {

/**
 * @brief Box generator SOP node
 */
class BoxSOP : public SOPNode {
private:
  static constexpr float DEFAULT_WIDTH = 2.0F;
  static constexpr float DEFAULT_HEIGHT = 2.0F;
  static constexpr float DEFAULT_DEPTH = 2.0F;
  static constexpr int DEFAULT_SEGMENTS = 1;

public:
  explicit BoxSOP(const std::string &node_name = "box")
      : SOPNode(node_name, "BoxSOP") {
    set_parameter("width", DEFAULT_WIDTH);
    set_parameter("height", DEFAULT_HEIGHT);
    set_parameter("depth", DEFAULT_DEPTH);
    set_parameter("width_segments", DEFAULT_SEGMENTS);
    set_parameter("height_segments", DEFAULT_SEGMENTS);
    set_parameter("depth_segments", DEFAULT_SEGMENTS);
  }

  void set_size(float width, float height, float depth) {
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
    const float width = get_parameter<float>("width", DEFAULT_WIDTH);
    const float height = get_parameter<float>("height", DEFAULT_HEIGHT);
    const float depth = get_parameter<float>("depth", DEFAULT_DEPTH);
    const int width_segments =
        get_parameter<int>("width_segments", DEFAULT_SEGMENTS);
    const int height_segments =
        get_parameter<int>("height_segments", DEFAULT_SEGMENTS);
    const int depth_segments =
        get_parameter<int>("depth_segments", DEFAULT_SEGMENTS);

    try {
      auto result = geometry::BoxGenerator::generate(
          static_cast<double>(width), static_cast<double>(height),
          static_cast<double>(depth), width_segments, height_segments,
          depth_segments);

      if (!result.has_value()) {
        set_error("Box generation failed");
        return nullptr;
      }

      return std::make_shared<core::GeometryContainer>(
          std::move(result.value()));

    } catch (const std::exception &exception) {
      set_error("Exception during box generation: " +
                std::string(exception.what()));
      return nullptr;
    }
  }
};

} // namespace nodeflux::sop
