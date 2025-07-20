#include "../../include/nodeflux/nodes/torus_node.hpp"
#include "../../include/nodeflux/geometry/torus_generator.hpp"

namespace nodeflux::nodes {

TorusNode::TorusNode(double major_radius, double minor_radius,
                     int major_segments, int minor_segments)
    : major_radius_(major_radius), minor_radius_(minor_radius),
      major_segments_(major_segments), minor_segments_(minor_segments) {}

std::optional<core::Mesh> TorusNode::generate() const {
  return geometry::TorusGenerator::generate(major_radius_, minor_radius_,
                                            major_segments_, minor_segments_);
}

const core::Error &TorusNode::last_error() const {
  return geometry::TorusGenerator::last_error();
}

} // namespace nodeflux::nodes
