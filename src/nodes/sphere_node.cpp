#include "../../include/nodeflux/nodes/sphere_node.hpp"
#include "../../include/nodeflux/geometry/sphere_generator.hpp"

namespace nodeflux::nodes {

SphereNode::SphereNode(double radius, int u_segments, int v_segments)
    : radius_(radius), u_segments_(u_segments), v_segments_(v_segments),
      subdivisions_(2), use_icosphere_(false) {}

SphereNode SphereNode::create_icosphere(double radius, int subdivisions) {
  SphereNode node(radius, 0, 0); // u/v segments not used for icosphere
  node.subdivisions_ = subdivisions;
  node.use_icosphere_ = true;
  return node;
}

std::optional<core::Mesh> SphereNode::generate() {
  if (use_icosphere_) {
    return geometry::SphereGenerator::generate_icosphere(radius_,
                                                         subdivisions_);
  } else {
    return geometry::SphereGenerator::generate_uv_sphere(radius_, u_segments_,
                                                         v_segments_);
  }
}

const core::Error &SphereNode::last_error() const {
  return geometry::SphereGenerator::last_error();
}

} // namespace nodeflux::nodes
