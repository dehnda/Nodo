#include "../../include/nodeflux/nodes/box_node.hpp"
#include "../../include/nodeflux/geometry/box_generator.hpp"

namespace nodeflux::nodes {

BoxNode::BoxNode(
    double width,
    double height,
    double depth,
    int width_segments,
    int height_segments,
    int depth_segments)
    : width_(width)
    , height_(height)
    , depth_(depth)
    , width_segments_(width_segments)
    , height_segments_(height_segments)
    , depth_segments_(depth_segments)
    , use_bounds_(false)
    , min_corner_(Eigen::Vector3d::Zero())
    , max_corner_(Eigen::Vector3d::Zero()) {
}

BoxNode BoxNode::create_from_bounds(
    const Eigen::Vector3d& min_corner,
    const Eigen::Vector3d& max_corner,
    int width_segments,
    int height_segments,
    int depth_segments) {
    
    BoxNode node(0.0, 0.0, 0.0, width_segments, height_segments, depth_segments);
    node.use_bounds_ = true;
    node.min_corner_ = min_corner;
    node.max_corner_ = max_corner;
    
    // Calculate dimensions from bounds
    const Eigen::Vector3d size = max_corner - min_corner;
    node.width_ = size.x();
    node.height_ = size.y();
    node.depth_ = size.z();
    
    return node;
}

std::optional<core::Mesh> BoxNode::generate() const {
    if (use_bounds_) {
        return geometry::BoxGenerator::generate_from_bounds(
            min_corner_,
            max_corner_,
            width_segments_,
            height_segments_,
            depth_segments_
        );
    } else {
        return geometry::BoxGenerator::generate(
            width_,
            height_,
            depth_,
            width_segments_,
            height_segments_,
            depth_segments_
        );
    }
}

void BoxNode::set_bounds(const Eigen::Vector3d& min_corner, const Eigen::Vector3d& max_corner) {
    use_bounds_ = true;
    min_corner_ = min_corner;
    max_corner_ = max_corner;
    
    // Update dimensions
    const Eigen::Vector3d size = max_corner - min_corner;
    width_ = size.x();
    height_ = size.y();
    depth_ = size.z();
}

const core::Error& BoxNode::last_error() {
    return geometry::BoxGenerator::last_error();
}

} // namespace nodeflux::nodes
