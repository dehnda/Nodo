#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

#include <algorithm>

namespace nodo::sop {

/**
 * @brief Align SOP - Aligns geometry to world axes or bounding box positions
 *
 * Transforms geometry to align its bounding box to specific positions:
 * - Center to origin
 * - Align min/max to specific axes
 * - Useful for centering geometry before operations
 */
class AlignSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit AlignSOP(const std::string& node_name = "align") : SOPNode(node_name, "Align") {
    // Single geometry input
    input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

    // Align mode
    register_parameter(define_int_parameter("align_mode", 0)
                           .label("Align Mode")
                           .options({"Center to Origin", "Min to Origin", "Max to Origin"})
                           .category("Alignment")
                           .description("How to align the geometry bounding box")
                           .build());

    // Axis selection
    register_parameter(define_int_parameter("align_x", 1)
                           .label("Align X")
                           .options({"Off", "On"})
                           .category("Axes")
                           .description("Apply alignment to X axis")
                           .build());

    register_parameter(define_int_parameter("align_y", 1)
                           .label("Align Y")
                           .options({"Off", "On"})
                           .category("Axes")
                           .description("Apply alignment to Y axis")
                           .build());

    register_parameter(define_int_parameter("align_z", 1)
                           .label("Align Z")
                           .options({"Off", "On"})
                           .category("Axes")
                           .description("Apply alignment to Z axis")
                           .build());
  }

protected:
  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override {
    // Apply group filter if specified (keeps only grouped points)
    auto input_result = apply_group_filter(0, core::ElementClass::POINT, false);
    if (!input_result.is_success()) {
      return {input_result.error().value()};
    }
    const auto& input = input_result.get_value();

    // Clone input
    auto output = std::make_shared<core::GeometryContainer>(input->clone());

    // Get positions
    auto* positions = output->get_point_attribute_typed<core::Vec3f>("P");
    if (!positions || positions->size() == 0) {
      return output;
    }

    // Get parameters
    const int mode = get_parameter<int>("align_mode", 0);
    const bool align_x = get_parameter<int>("align_x", 1) != 0;
    const bool align_y = get_parameter<int>("align_y", 1) != 0;
    const bool align_z = get_parameter<int>("align_z", 1) != 0;

    // Calculate bounding box
    core::Vec3f bbox_min = (*positions)[0];
    core::Vec3f bbox_max = (*positions)[0];

    for (size_t i = 1; i < positions->size(); ++i) {
      const auto& pos = (*positions)[i];
      bbox_min[0] = std::min(bbox_min[0], pos[0]);
      bbox_min[1] = std::min(bbox_min[1], pos[1]);
      bbox_min[2] = std::min(bbox_min[2], pos[2]);
      bbox_max[0] = std::max(bbox_max[0], pos[0]);
      bbox_max[1] = std::max(bbox_max[1], pos[1]);
      bbox_max[2] = std::max(bbox_max[2], pos[2]);
    }

    // Calculate offset based on mode
    core::Vec3f offset(0.0F, 0.0F, 0.0F);

    switch (mode) {
      case 0: { // Center to Origin
        core::Vec3f center = (bbox_min + bbox_max) * 0.5F;
        offset = -center;
        break;
      }
      case 1: // Min to Origin
        offset = -bbox_min;
        break;
      case 2: // Max to Origin
        offset = -bbox_max;
        break;
    }

    // Apply only to selected axes
    if (!align_x)
      offset[0] = 0.0F;
    if (!align_y)
      offset[1] = 0.0F;
    if (!align_z)
      offset[2] = 0.0F;

    // Apply offset to all points
    for (size_t i = 0; i < positions->size(); ++i) {
      (*positions)[i] += offset;
    }

    return output;
  }
};

} // namespace nodo::sop
