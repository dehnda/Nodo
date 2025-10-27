#include "nodeflux/sop/extrude_sop.hpp"

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::sop {

ExtrudeSOP::ExtrudeSOP(const std::string &name) : SOPNode(name, "Extrude") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_float_parameter("distance", 1.0F)
                         .label("Distance")
                         .range(0.0, 10.0)
                         .category("Extrusion")
                         .build());

  register_parameter(
      define_int_parameter("mode", 0)
          .label("Mode")
          .options({"Face Normals", "Uniform Direction", "Region Normals"})
          .category("Extrusion")
          .build());

  register_parameter(define_float_parameter("inset", 0.0F)
                         .label("Inset")
                         .range(0.0, 5.0)
                         .category("Extrusion")
                         .build());

  // Direction vector (for uniform mode)
  register_parameter(define_float_parameter("direction_x", 0.0F)
                         .label("Direction X")
                         .range(-1.0, 1.0)
                         .category("Direction")
                         .visible_when("mode", 1)
                         .build());

  register_parameter(define_float_parameter("direction_y", 0.0F)
                         .label("Direction Y")
                         .range(-1.0, 1.0)
                         .category("Direction")
                         .visible_when("mode", 1)
                         .build());

  register_parameter(define_float_parameter("direction_z", 1.0F)
                         .label("Direction Z")
                         .range(-1.0, 1.0)
                         .category("Direction")
                         .visible_when("mode", 1)
                         .build());
}

std::shared_ptr<core::GeometryContainer> ExtrudeSOP::execute() {
  auto input = get_input_data(0);
  if (input == nullptr) {
    set_error("No input geometry");
    return nullptr;
  }

  // Get parameters
  const float distance = get_parameter<float>("distance", 1.0F);
  const int mode = get_parameter<int>("mode", 0);
  const float inset = get_parameter<float>("inset", 0.0F);
  const float dir_x = get_parameter<float>("direction_x", 0.0F);
  const float dir_y = get_parameter<float>("direction_y", 0.0F);
  const float dir_z = get_parameter<float>("direction_z", 1.0F);

  // Get input positions
  const auto *input_positions =
      input->get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (input_positions == nullptr) {
    set_error("Input geometry has no position attribute");
    return nullptr;
  }

  const auto &input_topology = input->topology();
  const size_t input_point_count = input_topology.point_count();
  const size_t input_prim_count = input_topology.primitive_count();

  if (input_prim_count == 0) {
    set_error("Input geometry has no primitives to extrude");
    return nullptr;
  }

  // Create result container
  auto result = std::make_shared<core::GeometryContainer>();

  // Calculate extrusion direction for uniform mode
  core::Vec3f uniform_dir(dir_x, dir_y, dir_z);
  if (mode == 1 && uniform_dir.squaredNorm() > 0.0001F) { // Uniform Direction
    uniform_dir.normalize();
  }

  // For each primitive, calculate face normal and centroid
  std::vector<core::Vec3f> face_normals(input_prim_count);
  std::vector<core::Vec3f> face_centroids(input_prim_count);

  for (size_t prim_idx = 0; prim_idx < input_prim_count; ++prim_idx) {
    const auto &vert_indices = input_topology.get_primitive_vertices(prim_idx);
    if (vert_indices.size() < 3) {
      continue; // Skip degenerate primitives
    }

    // Get first three points to calculate normal
    const int p0_idx = input_topology.get_vertex_point(vert_indices[0]);
    const int p1_idx = input_topology.get_vertex_point(vert_indices[1]);
    const int p2_idx = input_topology.get_vertex_point(vert_indices[2]);

    const core::Vec3f p0 = (*input_positions)[p0_idx];
    const core::Vec3f p1 = (*input_positions)[p1_idx];
    const core::Vec3f p2 = (*input_positions)[p2_idx];

    // Calculate face normal using cross product
    const core::Vec3f edge1 = p1 - p0;
    const core::Vec3f edge2 = p2 - p0;
    core::Vec3f normal = edge1.cross(edge2);
    if (normal.squaredNorm() > 0.0001F) {
      normal.normalize();
    }
    face_normals[prim_idx] = normal;

    // Calculate centroid
    core::Vec3f centroid = core::Vec3f::Zero();
    for (size_t vert_idx : vert_indices) {
      const int point_idx = input_topology.get_vertex_point(vert_idx);
      centroid += (*input_positions)[point_idx];
    }
    centroid /= static_cast<float>(vert_indices.size());
    face_centroids[prim_idx] = centroid;
  }

  // Calculate total number of points and vertices needed
  // For each primitive:
  //   - If inset: num_verts new bottom points + num_verts new top points
  //   - If no inset: num_verts new top points (reuse original for bottom)
  //   - Vertices: num_verts for bottom + num_verts for top + 4*num_verts for
  //   sides
  size_t total_new_points = input_point_count;
  size_t total_vertices = 0;

  for (size_t prim_idx = 0; prim_idx < input_prim_count; ++prim_idx) {
    const auto &vert_indices = input_topology.get_primitive_vertices(prim_idx);
    const size_t num_verts = vert_indices.size();

    if (num_verts < 3) {
      continue;
    }

    if (inset > 0.0001F) {
      total_new_points += num_verts * 2; // bottom inset + top extruded
    } else {
      total_new_points += num_verts; // only top extruded
    }

    // Bottom face + top face + side quads
    total_vertices += num_verts + num_verts + (num_verts * 4);
  }

  // Create result container with pre-calculated sizes
  result->set_point_count(total_new_points);
  result->set_vertex_count(total_vertices);
  result->add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *result_positions =
      result->get_point_attribute_typed<core::Vec3f>(attrs::P);

  if (result_positions == nullptr) {
    set_error("Failed to create position attribute in result");
    return nullptr;
  }

  // Copy original points first
  for (size_t pt_idx = 0; pt_idx < input_point_count; ++pt_idx) {
    (*result_positions)[pt_idx] = (*input_positions)[pt_idx];
  }

  size_t next_point_idx = input_point_count;

  // For each primitive, create extruded geometry
  size_t vertex_offset = 0;

  for (size_t prim_idx = 0; prim_idx < input_prim_count; ++prim_idx) {
    const auto &vert_indices = input_topology.get_primitive_vertices(prim_idx);
    const size_t num_verts = vert_indices.size();

    if (num_verts < 3) {
      continue; // Skip degenerate primitives
    }

    // Determine extrusion direction for this face
    core::Vec3f extrude_dir;
    if (mode == 0) { // Face Normals
      extrude_dir = face_normals[prim_idx];
    } else if (mode == 1) { // Uniform Direction
      extrude_dir = uniform_dir;
    } else { // Region Normals (simplified - use face normal for now)
      extrude_dir = face_normals[prim_idx];
    }

    const core::Vec3f face_center = face_centroids[prim_idx];

    // Create new points for bottom and top faces
    std::vector<int> bottom_point_indices;
    std::vector<int> top_point_indices;
    bottom_point_indices.reserve(num_verts);
    top_point_indices.reserve(num_verts);

    for (size_t vert_idx : vert_indices) {
      const int old_point_idx = input_topology.get_vertex_point(vert_idx);
      const core::Vec3f old_pos = (*input_positions)[old_point_idx];

      // Apply inset (move toward face center)
      core::Vec3f inset_pos = old_pos;
      if (inset > 0.0001F) {
        const core::Vec3f to_center = face_center - old_pos;
        inset_pos = old_pos + to_center * inset;

        // Store new bottom point (inset)
        (*result_positions)[next_point_idx] = inset_pos;
        bottom_point_indices.push_back(static_cast<int>(next_point_idx));
        next_point_idx++;
      } else {
        // Use original point for bottom
        bottom_point_indices.push_back(old_point_idx);
      }

      // Apply extrusion
      const core::Vec3f new_pos = inset_pos + extrude_dir * distance;

      // Store top point (extruded)
      (*result_positions)[next_point_idx] = new_pos;
      top_point_indices.push_back(static_cast<int>(next_point_idx));
      next_point_idx++;
    }

    // Create bottom face (original, possibly inset)
    std::vector<int> bottom_prim_verts;
    bottom_prim_verts.reserve(num_verts);
    for (int bottom_pt_idx : bottom_point_indices) {
      const int new_vert_idx = static_cast<int>(vertex_offset);
      result->topology().set_vertex_point(vertex_offset, bottom_pt_idx);
      bottom_prim_verts.push_back(new_vert_idx);
      vertex_offset++;
    }
    result->add_primitive(bottom_prim_verts);

    // Create top face (extruded)
    std::vector<int> top_prim_verts;
    top_prim_verts.reserve(num_verts);
    for (int top_pt_idx : top_point_indices) {
      const int new_vert_idx = static_cast<int>(vertex_offset);
      result->topology().set_vertex_point(vertex_offset, top_pt_idx);
      top_prim_verts.push_back(new_vert_idx);
      vertex_offset++;
    }
    result->add_primitive(top_prim_verts);

    // Create side walls (quads connecting bottom to top)
    for (size_t i = 0; i < num_verts; ++i) {
      const size_t next_i = (i + 1) % num_verts;

      // Get the point indices for this edge
      const int bottom_pt_curr = bottom_point_indices[i];
      const int bottom_pt_next = bottom_point_indices[next_i];
      const int top_pt_curr = top_point_indices[i];
      const int top_pt_next = top_point_indices[next_i];

      // Create quad: bottom_curr -> bottom_next -> top_next -> top_curr
      std::vector<int> quad_verts;
      quad_verts.reserve(4);

      int v0 = static_cast<int>(vertex_offset);
      result->topology().set_vertex_point(vertex_offset++, bottom_pt_curr);
      quad_verts.push_back(v0);

      int v1 = static_cast<int>(vertex_offset);
      result->topology().set_vertex_point(vertex_offset++, bottom_pt_next);
      quad_verts.push_back(v1);

      int v2 = static_cast<int>(vertex_offset);
      result->topology().set_vertex_point(vertex_offset++, top_pt_next);
      quad_verts.push_back(v2);

      int v3 = static_cast<int>(vertex_offset);
      result->topology().set_vertex_point(vertex_offset++, top_pt_curr);
      quad_verts.push_back(v3);

      result->add_primitive(quad_verts);
    }
  }

  return result;
}

} // namespace nodeflux::sop
