#include "nodo/sop/resample_sop.hpp"
#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"

namespace attrs = nodo::core::standard_attrs;

namespace nodo::sop {

ResampleSOP::ResampleSOP(const std::string &name) : SOPNode(name, "Resample") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(
      define_int_parameter("mode", 0)
          .label("Mode")
          .options({"By Count", "By Length"})
          .category("Resample")
          .description("Resampling mode: fixed point count or segment length")
          .build());

  register_parameter(define_int_parameter("point_count", 10)
                         .label("Point Count")
                         .range(2, 1000)
                         .category("Resample")
                         .visible_when("mode", 0)
                         .description("Target number of points per primitive")
                         .build());

  register_parameter(define_float_parameter("segment_length", 0.1F)
                         .label("Segment Length")
                         .range(0.001, 10.0)
                         .category("Resample")
                         .visible_when("mode", 1)
                         .description("Target length of each segment")
                         .build());
}

std::shared_ptr<core::GeometryContainer> ResampleSOP::execute() {
  auto input = get_input_data(0);
  if (input == nullptr) {
    set_error("No input geometry");
    return nullptr;
  }

  // Get parameters
  const int mode = get_parameter<int>("mode", 0);
  const int point_count = get_parameter<int>("point_count", 10);
  const float segment_length = get_parameter<float>("segment_length", 0.1F);

  // Get input positions
  const auto *input_positions =
      input->get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (input_positions == nullptr) {
    set_error("Input geometry has no position attribute");
    return nullptr;
  }

  const auto &input_topology = input->topology();
  const size_t input_prim_count = input_topology.primitive_count();

  if (input_prim_count == 0) {
    set_error("Input geometry has no primitives to resample");
    return nullptr;
  }

  // Create result container
  auto result = std::make_shared<core::GeometryContainer>();

  // Calculate total points and vertices needed
  size_t total_points = 0;
  size_t total_vertices = 0;

  std::vector<size_t> prim_point_counts;
  prim_point_counts.reserve(input_prim_count);

  for (size_t prim_idx = 0; prim_idx < input_prim_count; ++prim_idx) {
    const auto &vert_indices = input_topology.get_primitive_vertices(prim_idx);
    const size_t num_verts = vert_indices.size();

    if (num_verts < 2) {
      prim_point_counts.push_back(0);
      continue;
    }

    size_t new_point_count = 0;

    if (mode == 0) {
      // By Count mode
      new_point_count = static_cast<size_t>(point_count);
    } else {
      // By Length mode - calculate curve length
      float curve_length = 0.0F;
      for (size_t i = 0; i < num_verts - 1; ++i) {
        const int pt_curr = input_topology.get_vertex_point(vert_indices[i]);
        const int pt_next =
            input_topology.get_vertex_point(vert_indices[i + 1]);
        const core::Vec3f pos_curr = (*input_positions)[pt_curr];
        const core::Vec3f pos_next = (*input_positions)[pt_next];
        curve_length += (pos_next - pos_curr).norm();
      }

      if (segment_length > 0.0001F) {
        new_point_count =
            static_cast<size_t>(curve_length / segment_length) + 1;
        new_point_count = std::max(new_point_count, size_t(2));
      } else {
        new_point_count = 2;
      }
    }

    prim_point_counts.push_back(new_point_count);
    total_points += new_point_count;
    total_vertices += new_point_count;
  }

  // Create result with pre-calculated sizes
  result->set_point_count(total_points);
  result->set_vertex_count(total_vertices);
  result->add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *result_positions =
      result->get_point_attribute_typed<core::Vec3f>(attrs::P);

  if (result_positions == nullptr) {
    set_error("Failed to create position attribute in result");
    return nullptr;
  }

  size_t next_point_idx = 0;
  size_t vertex_offset = 0;

  // Process each primitive
  for (size_t prim_idx = 0; prim_idx < input_prim_count; ++prim_idx) {
    const size_t new_point_count = prim_point_counts[prim_idx];

    if (new_point_count == 0) {
      continue;
    }

    const auto &vert_indices = input_topology.get_primitive_vertices(prim_idx);
    const size_t num_verts = vert_indices.size();

    // Build cumulative lengths along the curve
    std::vector<float> cumulative_lengths;
    cumulative_lengths.reserve(num_verts);
    cumulative_lengths.push_back(0.0F);

    float total_length = 0.0F;
    for (size_t i = 0; i < num_verts - 1; ++i) {
      const int pt_curr = input_topology.get_vertex_point(vert_indices[i]);
      const int pt_next = input_topology.get_vertex_point(vert_indices[i + 1]);
      const core::Vec3f pos_curr = (*input_positions)[pt_curr];
      const core::Vec3f pos_next = (*input_positions)[pt_next];
      const float segment_len = (pos_next - pos_curr).norm();
      total_length += segment_len;
      cumulative_lengths.push_back(total_length);
    }

    // Create resampled points
    std::vector<int> new_prim_verts;
    new_prim_verts.reserve(new_point_count);

    for (size_t i = 0; i < new_point_count; ++i) {
      // Calculate target distance along curve
      float target_distance = 0.0F;
      if (new_point_count > 1) {
        target_distance = total_length * static_cast<float>(i) /
                          static_cast<float>(new_point_count - 1);
      }

      // Find the segment containing this distance
      size_t segment_idx = 0;
      for (size_t j = 0; j < cumulative_lengths.size() - 1; ++j) {
        if (target_distance >= cumulative_lengths[j] &&
            target_distance <= cumulative_lengths[j + 1]) {
          segment_idx = j;
          break;
        }
      }

      // Interpolate position within the segment
      const int pt_curr =
          input_topology.get_vertex_point(vert_indices[segment_idx]);
      const int pt_next = input_topology.get_vertex_point(
          vert_indices[std::min(segment_idx + 1, num_verts - 1)]);
      const core::Vec3f pos_curr = (*input_positions)[pt_curr];
      const core::Vec3f pos_next = (*input_positions)[pt_next];

      float segment_start = cumulative_lengths[segment_idx];
      float segment_end = cumulative_lengths[segment_idx + 1];
      float segment_length_val = segment_end - segment_start;

      core::Vec3f new_pos;
      if (segment_length_val > 0.0001F) {
        float t = (target_distance - segment_start) / segment_length_val;
        t = std::clamp(t, 0.0F, 1.0F);
        new_pos = pos_curr + t * (pos_next - pos_curr);
      } else {
        new_pos = pos_curr;
      }

      // Store the new point
      (*result_positions)[next_point_idx] = new_pos;

      // Add vertex
      result->topology().set_vertex_point(vertex_offset,
                                          static_cast<int>(next_point_idx));
      new_prim_verts.push_back(static_cast<int>(vertex_offset));

      next_point_idx++;
      vertex_offset++;
    }

    // Add the resampled primitive (polyline)
    result->add_primitive(new_prim_verts);
  }

  return result;
}

} // namespace nodo::sop
