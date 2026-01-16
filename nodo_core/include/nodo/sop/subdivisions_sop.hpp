#pragma once

#include "nodo/core/geometry_container.hpp"
#include "nodo/processing/subdivision.hpp"
#include "nodo/sop/sop_node.hpp"

#include <memory>
#include <string>

namespace nodo::sop {

/**
 * @brief Subdivision Surface SOP - Applies subdivision to create smooth
 * surfaces
 *
 * Supports multiple subdivision algorithms:
 * - Catmull-Clark: For quads/mixed meshes (creates smooth surfaces)
 * - Loop: For triangle meshes (creates smooth surfaces)
 * - Quad-Tri: For mixed quad/triangle meshes
 */
class SubdivisionSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit SubdivisionSOP(const std::string& name = "subdivision");

  // Configuration methods
  void set_subdivision_levels(int levels) {
    int clamped = std::max(1, std::min(levels, 5)); // Limit to reasonable range
    set_parameter("subdivision_levels", clamped);
  }

  void set_subdivision_type(int type) { set_parameter("subdivision_type", type); }

  // Getters
  int get_subdivision_levels() const { return get_parameter<int>("subdivision_levels", 1); }

  int get_subdivision_type() const { return get_parameter<int>("subdivision_type", 0); }

protected:
  /**
   * @brief Execute the subdivision operation (SOPNode override)
   */
  std::shared_ptr<core::GeometryContainer> execute() override;
};

} // namespace nodo::sop
