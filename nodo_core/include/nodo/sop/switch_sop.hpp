#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief Switch SOP node - selects one of multiple inputs based on index
 */
class SwitchSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;
  static constexpr int MAX_INPUTS = 10;

  explicit SwitchSOP(const std::string& node_name = "switch");

  [[nodiscard]] InputConfig get_input_config() const override { return {InputType::MULTI_DYNAMIC, 1, MAX_INPUTS, 1}; }

protected:
  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override;
};

} // namespace nodo::sop
