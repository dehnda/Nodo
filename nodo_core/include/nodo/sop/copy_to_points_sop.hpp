#pragma once

#include "../core/geometry_attributes.hpp"
#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief Copy/instance geometry to point locations
 *
 * This SOP takes template geometry and creates instances at each point
 * in the input point cloud, with support for attribute-driven transformations.
 */
class CopyToPointsSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

private:
  static constexpr bool DEFAULT_USE_POINT_NORMALS = true;
  static constexpr bool DEFAULT_USE_POINT_SCALE = true;
  static constexpr float DEFAULT_UNIFORM_SCALE = 1.0F;

public:
  explicit CopyToPointsSOP(const std::string& node_name = "copy_to_points");

  // Dual-input node - requires exactly 2 inputs
  InputConfig get_input_config() const override { return InputConfig(InputType::DUAL, 2, 2, 2); }

  /**
   * @brief Copy template geometry to all point locations
   */
  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override;
};

} // namespace nodo::sop
