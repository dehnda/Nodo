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
  static constexpr float DEFAULT_RADIUS = 1.0F;
  static constexpr float DEFAULT_HEIGHT = 2.0F;
  static constexpr int DEFAULT_RADIAL_SEGMENTS = 32;
  static constexpr int DEFAULT_HEIGHT_SEGMENTS = 1;

public:
  explicit CylinderSOP(const std::string &node_name = "cylinder")
      : SOPNode(node_name, "CylinderSOP") {
    set_parameter("radius", DEFAULT_RADIUS);
    set_parameter("height", DEFAULT_HEIGHT);
    set_parameter("radial_segments", DEFAULT_RADIAL_SEGMENTS);
    set_parameter("height_segments", DEFAULT_HEIGHT_SEGMENTS);
    set_parameter("top_cap", true);
    set_parameter("bottom_cap", true);
  }

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
  std::shared_ptr<core::GeometryContainer> execute() override {
    const float radius = get_parameter<float>("radius", DEFAULT_RADIUS);
    const float height = get_parameter<float>("height", DEFAULT_HEIGHT);
    const int radial_segments =
        get_parameter<int>("radial_segments", DEFAULT_RADIAL_SEGMENTS);
    const int height_segments =
        get_parameter<int>("height_segments", DEFAULT_HEIGHT_SEGMENTS);
    const bool top_cap = get_parameter<bool>("top_cap", true);
    const bool bottom_cap = get_parameter<bool>("bottom_cap", true);

    try {
      auto result = geometry::CylinderGenerator::generate(
          static_cast<double>(radius), static_cast<double>(height),
          radial_segments, height_segments, top_cap, bottom_cap);

      if (!result.has_value()) {
        set_error("Cylinder generation failed");
        return nullptr;
      }

      // hard edges
      sop::utils::compute_hard_edge_normals(result.value(), true);

      return std::make_shared<core::GeometryContainer>(
          std::move(result.value()));

    } catch (const std::exception &exception) {
      set_error("Exception during cylinder generation: " +
                std::string(exception.what()));
      return nullptr;
    }
  }
};

} // namespace nodo::sop
