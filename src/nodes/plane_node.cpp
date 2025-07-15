#include "../../include/nodeflux/nodes/plane_node.hpp"
#include "../../include/nodeflux/geometry/plane_generator.hpp"

namespace nodeflux::nodes {

PlaneNode::PlaneNode(
    double width,
    double height,
    int width_segments,
    int height_segments)
    : width_(width)
    , height_(height)
    , width_segments_(width_segments)
    , height_segments_(height_segments) {
}

std::optional<core::Mesh> PlaneNode::generate() const {
    return geometry::PlaneGenerator::generate(
        width_, 
        height_, 
        width_segments_, 
        height_segments_
    );
}

const core::Error& PlaneNode::last_error() {
    return geometry::PlaneGenerator::last_error();
}

} // namespace nodeflux::nodes
