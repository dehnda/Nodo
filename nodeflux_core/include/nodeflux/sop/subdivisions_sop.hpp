#pragma once

#include "nodeflux/core/mesh.hpp"
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
    int clamped = std::max(0, std::min(levels, 5)); // Limit to reasonable range
    set_parameter("subdivision_levels", clamped);
  }

  void set_preserve_boundaries(bool preserve) {
    set_parameter("preserve_boundaries", preserve ? 1 : 0);
  }

  // Getters
  int get_subdivision_levels() const {
    return get_parameter<int>("subdivision_levels", 1);
  }
  bool get_preserve_boundaries() const {
    return (get_parameter<int>("preserve_boundaries", 1) != 0);
  }

protected:
  /**
   * @brief Execute the subdivision operation (SOPNode override)
   */
  std::shared_ptr<core::GeometryContainer> execute() override;

private:
  void apply_simple_subdivision(core::GeometryContainer &container);
  void apply_catmull_clark(core::GeometryContainer &container,
                           bool preserve_boundaries);

  void add_triangle(core::GeometryContainer &container, int p0, int p1, int p2);
  void add_quad(core::GeometryContainer &container, int p0, int p1, int p2,
                int p3);

  int subdivision_levels_ = 1;
  bool preserve_boundaries_ = true;
};

} // namespace nodeflux::sop
