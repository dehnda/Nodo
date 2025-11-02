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

  explicit MergeSOP(const std::string &node_name);

  // Multi-input node - requires at least 1 input, accepts unlimited
  int get_min_inputs() const override { return 1; }
  int get_max_inputs() const override { return -1; } // -1 means unlimited

protected:
  std::shared_ptr<core::GeometryContainer> execute() override;
};

} // namespace nodo::sop
