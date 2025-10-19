#include "nodeflux/nodes/cylinder_node.hpp"
#include "nodeflux/geometry/cylinder_generator.hpp"

namespace nodeflux::nodes {

CylinderNode::CylinderNode(double radius, double height, int radial_segments,
                           int height_segments, bool top_cap, bool bottom_cap)
    : radius_(radius), height_(height), radial_segments_(radial_segments),
      height_segments_(height_segments), top_cap_(top_cap),
      bottom_cap_(bottom_cap) {}

std::optional<core::Mesh> CylinderNode::generate() const {
  return geometry::CylinderGenerator::generate(
      radius_, height_, radial_segments_, height_segments_, top_cap_,
      bottom_cap_);
}

const core::Error &CylinderNode::last_error() {
  return geometry::CylinderGenerator::last_error();
}

} // namespace nodeflux::nodes
