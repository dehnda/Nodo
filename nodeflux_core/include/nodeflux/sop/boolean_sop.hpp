#pragma once

#include "nodeflux/sop/sop_node.hpp"
#include <memory>
#include <string>

namespace nodeflux::sop {

/**
 * @brief Boolean SOP - Performs boolean operations between geometries
 *
 * Supports union, intersection, and difference operations using Manifold
 * (Apache 2.0) - fully compatible with commercial use.
 */
class BooleanSOP : public SOPNode {
public:
  enum class OperationType {
    UNION = 0,        ///< Combine meshes (A ∪ B)
    INTERSECTION = 1, ///< Keep only overlapping parts (A ∩ B)
    DIFFERENCE = 2    ///< Subtract B from A (A - B)
  };

  /**
   * @brief Construct a new Boolean SOP
   * @param name Node name for debugging
   */
  explicit BooleanSOP(const std::string &node_name = "boolean");

  /**
   * @brief Execute the boolean operation
   * @return Result GeometryData or std::nullopt on failure
   */
  std::shared_ptr<core::GeometryContainer> execute() override;

  /**
   * @brief Convert operation type to string
   * @param operation The operation type
   * @return String representation
   */
  static std::string operation_to_string(OperationType operation);
};

} // namespace nodeflux::sop
