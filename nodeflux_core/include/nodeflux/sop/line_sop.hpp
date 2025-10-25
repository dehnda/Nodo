#pragma once

#include "nodeflux/core/geometry_container.hpp"
#include "nodeflux/sop/sop_node.hpp"
#include <memory>
#include <string>

namespace nodeflux::sop {

/**
 * @brief Line SOP - Generates a line between two points
 *
 * Creates a simple line geometry with a specified number of segments.
 * Useful for creating curves, paths, and guide lines for other operations.
 */
class LineSOP : public SOPNode {
public:
  explicit LineSOP(const std::string &name = "line");

protected:
  /**
   * @brief Execute the line generation (SOPNode override)
   */
  std::shared_ptr<core::GeometryContainer> execute() override;
};

} // namespace nodeflux::sop
