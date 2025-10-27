#pragma once

#include "sop_node.hpp"

namespace nodeflux::sop {

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
  explicit DeleteSOP(const std::string &name);

  std::shared_ptr<core::GeometryContainer> execute() override;
};

} // namespace nodeflux::sop
