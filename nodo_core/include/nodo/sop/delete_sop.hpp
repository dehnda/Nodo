#pragma once

#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief SOP that deletes points or primitives based on groups or patterns
 *
 * Provides selective deletion of geometry elements:
 * - By group membership (selected/non-selected)
 * - By pattern (range, every Nth)
 * - With optional cleanup of unused points
 */
class DeleteSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit DeleteSOP(const std::string& name);

  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override;
};

} // namespace nodo::sop
