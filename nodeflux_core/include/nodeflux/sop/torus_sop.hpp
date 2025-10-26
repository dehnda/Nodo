#pragma once

#include "../core/geometry_container.hpp"
#include "../geometry/torus_generator.hpp"
#include "sop_node.hpp"

namespace nodeflux::sop {

/**
 * @brief Torus (donut) generator SOP node
 */
class TorusSOP : public SOPNode {
private:
  static constexpr float DEFAULT_MAJOR_RADIUS = 1.0F;
  static constexpr float DEFAULT_MINOR_RADIUS = 0.3F;
  static constexpr int DEFAULT_MAJOR_SEGMENTS = 48;
  static constexpr int DEFAULT_MINOR_SEGMENTS = 12;

public:
  explicit TorusSOP(const std::string &node_name = "torus")
      : SOPNode(node_name, "TorusSOP") {
    set_parameter("major_radius", DEFAULT_MAJOR_RADIUS);
    set_parameter("minor_radius", DEFAULT_MINOR_RADIUS);
    set_parameter("major_segments", DEFAULT_MAJOR_SEGMENTS);
    set_parameter("minor_segments", DEFAULT_MINOR_SEGMENTS);
  }

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
    const float major_radius =
        get_parameter<float>("major_radius", DEFAULT_MAJOR_RADIUS);
    const float minor_radius =
        get_parameter<float>("minor_radius", DEFAULT_MINOR_RADIUS);
    const int major_segments =
        get_parameter<int>("major_segments", DEFAULT_MAJOR_SEGMENTS);
    const int minor_segments =
        get_parameter<int>("minor_segments", DEFAULT_MINOR_SEGMENTS);

    try {
      auto result = geometry::TorusGenerator::generate(
          static_cast<double>(major_radius), static_cast<double>(minor_radius),
          major_segments, minor_segments);

      if (!result.has_value()) {
        set_error("Torus generation failed");
        return nullptr;
      }

      return std::make_shared<core::GeometryContainer>(
          std::move(result.value()));

    } catch (const std::exception &exception) {
      set_error("Exception during torus generation: " +
                std::string(exception.what()));
      return nullptr;
    }
  }
};

} // namespace nodeflux::sop
