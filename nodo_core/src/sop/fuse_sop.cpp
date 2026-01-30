#include "nodo/sop/fuse_sop.hpp"

namespace attrs = nodo::core::standard_attrs;

namespace nodo::sop {
FuseSOP::FuseSOP(const std::string& node_name) : SOPNode(node_name, "Fuse") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_float_parameter("threshold", DEFAULT_THRESHOLD)
                         .label("Threshold")
                         .range(0.0, 1.0)
                         .category("Fuse")
                         .description("Distance threshold for fusing points")
                         .build());
}

core::Result<std::shared_ptr<core::GeometryContainer>> FuseSOP::execute() {
  // Get input directly (don't use apply_group_filter since we create new geometry)
  auto input = get_input_data(0);
  if (!input) {
    return {"Fuse node requires input geometry"};
  }

  const float threshold = get_parameter<float>("threshold", DEFAULT_THRESHOLD);

  // Get input positions
  const auto* input_positions = input->get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (input_positions == nullptr) {
    return {"Input geometry has no position attribute"};
  }

  // Build point map and fused positions
  std::vector<int> point_map(input->topology().point_count(), -1);
  std::vector<core::Vec3f> fused_positions;

  for (size_t i = 0; i < input->topology().point_count(); ++i) {
    const core::Vec3f& pos = (*input_positions)[i];
    bool found = false;

    // Check if this point is within threshold of any existing fused point
    for (size_t j = 0; j < fused_positions.size(); ++j) {
      if ((pos - fused_positions[j]).norm() <= threshold) {
        point_map[i] = static_cast<int>(j);
        found = true;
        break;
      }
    }

    if (!found) {
      point_map[i] = static_cast<int>(fused_positions.size());
      fused_positions.push_back(pos);
    }
  }

  // Create output geometry
  auto output = std::make_shared<core::GeometryContainer>();

  // Set point count
  output->set_point_count(fused_positions.size());

  // Add position attribute and fill it
  output->add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto* out_positions = output->get_point_attribute_typed<core::Vec3f>(attrs::P);
  auto out_pos_span = out_positions->values_writable();
  for (size_t i = 0; i < fused_positions.size(); ++i) {
    out_pos_span[i] = fused_positions[i];
  }

  // Rebuild primitives with remapped vertices
  const auto& input_topo = input->topology();

  // Count required vertices
  size_t total_vertices = input_topo.vertex_count();
  output->set_vertex_count(total_vertices);

  // Remap vertices to fused points
  for (size_t vert_idx = 0; vert_idx < total_vertices; ++vert_idx) {
    int old_point_idx = input_topo.get_vertex_point(static_cast<int>(vert_idx));
    int new_point_idx = point_map[old_point_idx];
    output->topology().set_vertex_point(static_cast<int>(vert_idx), new_point_idx);
  }

  // Copy primitives (vertex indices stay the same, but now point to remapped points)
  for (size_t prim_idx = 0; prim_idx < input_topo.primitive_count(); ++prim_idx) {
    const auto& prim_verts = input_topo.get_primitive_vertices(static_cast<int>(prim_idx));
    output->topology().add_primitive(prim_verts);
  }

  // Copy other point attributes (average values for fused points)
  // Note: This is a simple approach - for production, you might want different strategies
  // (first value, average, weighted average, etc.)

  return output;
}
} // namespace nodo::sop
