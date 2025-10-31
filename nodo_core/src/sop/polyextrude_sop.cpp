#include "nodo/sop/polyextrude_sop.hpp"

namespace attrs = nodo::core::standard_attrs;

namespace nodo::sop {

PolyExtrudeSOP::PolyExtrudeSOP(const std::string &name)
    : SOPNode(name, "PolyExtrude") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_float_parameter("distance", 1.0F)
                         .label("Distance")
                         .range(0.0, 10.0)
                         .category("Extrusion")
                         .description("Distance to extrude each face")
                         .build());

  register_parameter(
      define_float_parameter("inset", 0.0F)
          .label("Inset")
          .range(0.0, 1.0)
          .category("Extrusion")
          .description("Amount to inset face borders before extrusion")
          .build());

  register_parameter(
      define_int_parameter("individual_faces", 1)
          .label("Individual Faces")
          .range(0, 1)
          .category("Extrusion")
          .description("Extrude each face separately (1) or as a group (0)")
          .build());
}

std::shared_ptr<core::GeometryContainer> PolyExtrudeSOP::execute() {
  auto input = get_input_data(0);
  if (input == nullptr) {
    set_error("No input geometry");
    return nullptr;
  }

  // Get parameters
  const float distance = get_parameter<float>("distance", 1.0F);
  const float inset = get_parameter<float>("inset", 0.0F);
  const bool individual_faces = get_parameter<int>("individual_faces", 1) != 0;

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
    set_error("Input geometry has no primitives to extrude");
    return nullptr;
  }

  // Create result container
  auto result = std::make_shared<core::GeometryContainer>();

  // For poly extrude, each face is extruded independently
  // This means we create separate points for each face's extrusion

  // Calculate total points and vertices
  size_t total_points = 0;
  size_t total_vertices = 0;

  for (size_t prim_idx = 0; prim_idx < input_prim_count; ++prim_idx) {
    const auto &vert_indices = input_topology.get_primitive_vertices(prim_idx);
    const size_t num_verts = vert_indices.size();

    if (num_verts < 3) {
      continue;
    }

    if (individual_faces) {
      // Each face gets its own set of points (no sharing)
      // Original points for this face + inset points (if needed) + extruded
      // points
      if (inset > 0.0001F) {
        total_points += num_verts * 3; // original + inset bottom + extruded top
      } else {
        total_points += num_verts * 2; // original bottom + extruded top
      }
    } else {
      // Shared vertices (similar to regular extrude)
      if (inset > 0.0001F) {
        total_points += num_verts * 2; // inset bottom + extruded top
      } else {
        total_points += num_verts; // only extruded top
      }
    }

    // Bottom face + top face + side quads
    total_vertices += num_verts + num_verts + (num_verts * 4);
  }

  // If not individual faces, add original input points
  if (!individual_faces) {
    total_points += input_topology.point_count();
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

  // If not individual faces, copy original points first
  if (!individual_faces) {
    for (size_t pt_idx = 0; pt_idx < input_topology.point_count(); ++pt_idx) {
      (*result_positions)[next_point_idx++] = (*input_positions)[pt_idx];
    }
  }

  // Process each primitive
  for (size_t prim_idx = 0; prim_idx < input_prim_count; ++prim_idx) {
    const auto &vert_indices = input_topology.get_primitive_vertices(prim_idx);
    const size_t num_verts = vert_indices.size();

    if (num_verts < 3) {
      continue;
    }

    // Calculate face normal and centroid
    const int p0_idx = input_topology.get_vertex_point(vert_indices[0]);
    const int p1_idx = input_topology.get_vertex_point(vert_indices[1]);
    const int p2_idx = input_topology.get_vertex_point(vert_indices[2]);

    const core::Vec3f point0 = (*input_positions)[p0_idx];
    const core::Vec3f point1 = (*input_positions)[p1_idx];
    const core::Vec3f point2 = (*input_positions)[p2_idx];

    // Calculate face normal
    const core::Vec3f edge1 = point1 - point0;
    const core::Vec3f edge2 = point2 - point0;
    core::Vec3f normal = edge1.cross(edge2);
    if (normal.squaredNorm() > 0.0001F) {
      normal.normalize();
    }

    // Calculate face centroid
    core::Vec3f centroid = core::Vec3f::Zero();
    for (size_t vert_idx : vert_indices) {
      const int point_idx = input_topology.get_vertex_point(vert_idx);
      centroid += (*input_positions)[point_idx];
    }
    centroid /= static_cast<float>(num_verts);

    // Create points for this face's extrusion
    std::vector<int> bottom_point_indices;
    std::vector<int> top_point_indices;
    bottom_point_indices.reserve(num_verts);
    top_point_indices.reserve(num_verts);

    for (size_t vert_idx : vert_indices) {
      const int old_point_idx = input_topology.get_vertex_point(vert_idx);
      const core::Vec3f old_pos = (*input_positions)[old_point_idx];

      if (individual_faces) {
        // Create new bottom point (copy of original)
        (*result_positions)[next_point_idx] = old_pos;
        const int bottom_base_pt = static_cast<int>(next_point_idx);
        next_point_idx++;

        // Apply inset if needed
        core::Vec3f inset_pos = old_pos;
        if (inset > 0.0001F) {
          const core::Vec3f to_center = centroid - old_pos;
          inset_pos = old_pos + to_center * inset;

          // Create inset bottom point
          (*result_positions)[next_point_idx] = inset_pos;
          bottom_point_indices.push_back(static_cast<int>(next_point_idx));
          next_point_idx++;
        } else {
          bottom_point_indices.push_back(bottom_base_pt);
        }

        // Create extruded top point
        const core::Vec3f top_pos = inset_pos + normal * distance;
        (*result_positions)[next_point_idx] = top_pos;
        top_point_indices.push_back(static_cast<int>(next_point_idx));
        next_point_idx++;
      } else {
        // Shared points mode (like regular extrude)
        core::Vec3f inset_pos = old_pos;
        if (inset > 0.0001F) {
          const core::Vec3f to_center = centroid - old_pos;
          inset_pos = old_pos + to_center * inset;

          (*result_positions)[next_point_idx] = inset_pos;
          bottom_point_indices.push_back(static_cast<int>(next_point_idx));
          next_point_idx++;
        } else {
          bottom_point_indices.push_back(old_point_idx);
        }

        const core::Vec3f top_pos = inset_pos + normal * distance;
        (*result_positions)[next_point_idx] = top_pos;
        top_point_indices.push_back(static_cast<int>(next_point_idx));
        next_point_idx++;
      }
    }

    // Create bottom face
    std::vector<int> bottom_prim_verts;
    bottom_prim_verts.reserve(num_verts);
    for (int bottom_pt_idx : bottom_point_indices) {
      result->topology().set_vertex_point(vertex_offset, bottom_pt_idx);
      bottom_prim_verts.push_back(static_cast<int>(vertex_offset));
      vertex_offset++;
    }
    result->add_primitive(bottom_prim_verts);

    // Create top face
    std::vector<int> top_prim_verts;
    top_prim_verts.reserve(num_verts);
    for (int top_pt_idx : top_point_indices) {
      result->topology().set_vertex_point(vertex_offset, top_pt_idx);
      top_prim_verts.push_back(static_cast<int>(vertex_offset));
      vertex_offset++;
    }
    result->add_primitive(top_prim_verts);

    // Create side walls
    for (size_t i = 0; i < num_verts; ++i) {
      const size_t next_i = (i + 1) % num_verts;

      const int bottom_pt_curr = bottom_point_indices[i];
      const int bottom_pt_next = bottom_point_indices[next_i];
      const int top_pt_curr = top_point_indices[i];
      const int top_pt_next = top_point_indices[next_i];

      // Create quad
      std::vector<int> quad_verts;
      quad_verts.reserve(4);

      result->topology().set_vertex_point(vertex_offset, bottom_pt_curr);
      quad_verts.push_back(static_cast<int>(vertex_offset++));

      result->topology().set_vertex_point(vertex_offset, bottom_pt_next);
      quad_verts.push_back(static_cast<int>(vertex_offset++));

      result->topology().set_vertex_point(vertex_offset, top_pt_next);
      quad_verts.push_back(static_cast<int>(vertex_offset++));

      result->topology().set_vertex_point(vertex_offset, top_pt_curr);
      quad_verts.push_back(static_cast<int>(vertex_offset++));

      result->add_primitive(quad_verts);
    }
  }

  return result;
}

} // namespace nodo::sop
