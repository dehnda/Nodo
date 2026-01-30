#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief Merge SOP node - combines multiple input geometries
 *
 * Merges multiple geometry inputs into a single output by:
 * - Combining all points from all inputs
 * - Adjusting primitive vertex indices to account for point offsets
 * - Combining all primitives
 * - Preserving topology and attributes
 */
class MergeSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;
  static constexpr int MAX_INPUTS = -1; // Unlimited inputs

  explicit MergeSOP(const std::string& node_name = "merge");

  // Multi-input node - requires at least 1 input, accepts unlimited
  // Single wide input pin that accepts multiple connections
  InputConfig get_input_config() const override { return {InputType::MULTI_DYNAMIC, 1, MAX_INPUTS, 1}; }

protected:
  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override;
};

} // namespace nodo::sop
