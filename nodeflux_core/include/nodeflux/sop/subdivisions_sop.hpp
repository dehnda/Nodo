#pragma once

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/sop/geometry_data.hpp"
#include "nodeflux/sop/sop_node.hpp"
#include <Eigen/Dense>
#include <memory>
#include <string>

namespace nodeflux::sop {

/**
 * @brief Subdivision Surface SOP - Applies Catmull-Clark subdivision to meshes
 *
 * Subdivides mesh faces to create smoother surfaces with higher polygon
 * density.
 */
class SubdivisionSOP : public SOPNode {
public:
  explicit SubdivisionSOP(const std::string &name = "subdivision");

  // Configuration methods
  void set_subdivision_levels(int levels) {
    int clamped = std::max(0, std::min(levels, 4)); // Limit to reasonable range
    if (subdivision_levels_ != clamped) {
      subdivision_levels_ = clamped;
      mark_dirty();
    }
  }

  void set_preserve_boundaries(bool preserve) {
    if (preserve_boundaries_ != preserve) {
      preserve_boundaries_ = preserve;
      mark_dirty();
    }
  }

  // Getters
  int get_subdivision_levels() const { return subdivision_levels_; }
  bool get_preserve_boundaries() const { return preserve_boundaries_; }

protected:
  /**
   * @brief Execute the subdivision operation (SOPNode override)
   */
  std::shared_ptr<GeometryData> execute() override;

private:
  std::optional<core::Mesh>
  apply_catmull_clark_subdivision(const core::Mesh &mesh);

  int subdivision_levels_ = 1;
  bool preserve_boundaries_ = true;
};

} // namespace nodeflux::sop
