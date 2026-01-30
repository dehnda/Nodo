#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

namespace nodo::sop {
/**
 * @brief Fuse SOP node - Merge points within a specified distance threshold
 *
 * The FuseSOP node merges points that are within a user-defined distance threshold,
 * effectively removing duplicate or closely spaced points from the geometry.
 */
class FuseSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;
  static constexpr float DEFAULT_THRESHOLD = 0.01F;
  explicit FuseSOP(const std::string& node_name = "fuse");

  InputConfig get_input_config() const override { return {InputType::SINGLE, 1, 1, 0}; }

protected:
  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override;
};
} // namespace nodo::sop
