#pragma once

#include "sop_node.hpp"
#include <memory>

namespace nodo::sop {

/**
 * @brief Create and manipulate named groups of geometry elements
 *
 * Groups are named collections of points, vertices, or primitives used for
 * selective operations. This SOP provides a UI for creating groups using
 * various selection methods:
 * - Range selection (0-10)
 * - Pattern selection (every Nth element)
 * - Random selection
 * - Attribute-based selection (future)
 *
 * Groups are stored as integer attributes (0 or 1) on the geometry.
 */
class GroupSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit GroupSOP(const std::string &name = "group");
  ~GroupSOP() override = default;

protected:
  std::shared_ptr<core::GeometryContainer> execute() override;
};

} // namespace nodo::sop
