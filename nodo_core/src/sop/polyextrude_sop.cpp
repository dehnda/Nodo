#include "nodo/sop/polyextrude_sop.hpp"

namespace attrs = nodo::core::standard_attrs;

namespace nodo::sop {

PolyExtrudeSOP::PolyExtrudeSOP(const std::string& name) : SOPNode(name, "PolyExtrude") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_int_parameter("extrusion_type", 0)
                         .label("Extrusion Type")
                         .options({"Faces", "Edges", "Points"})
                         .category("Extrusion")
                         .description("Type of geometry elements to extrude")
                         .build());

  register_parameter(define_float_parameter("distance", 1.0F)
                         .label("Distance")
                         .range(0.0, 10.0)
                         .category("Extrusion")
                         .description("Distance to extrude")
                         .build());

  register_parameter(define_float_parameter("inset", 0.0F)
                         .label("Inset")
                         .range(0.0, 1.0)
                         .category("Extrusion")
                         .description("Amount to inset borders before extrusion")
                         .build());

  register_parameter(define_bool_parameter("individual_faces", true)
                         .label("Individual Elements")
                         .category("Extrusion")
                         .description("Extrude each element separately (true) or as a group (false)")
                         .build());

  // Edge extrusion direction parameters
  register_parameter(define_int_parameter("edge_direction_mode", 0)
                         .label("Edge Direction")
                         .options({"Auto (Perpendicular)", "Custom Direction"})
                         .category("Edge Extrusion")
                         .visible_when("extrusion_type", 1)
                         .description("How to calculate edge extrusion direction")
                         .build());

  register_parameter(define_float_parameter("edge_direction_x", 0.0F)
                         .label("Direction X")
                         .range(-1.0, 1.0)
                         .category("Edge Extrusion")
                         .visible_when("edge_direction_mode", 1)
                         .description("X component of custom edge extrusion direction")
                         .build());

  register_parameter(define_float_parameter("edge_direction_y", 1.0F)
                         .label("Direction Y")
                         .range(-1.0, 1.0)
                         .category("Edge Extrusion")
                         .visible_when("edge_direction_mode", 1)
                         .description("Y component of custom edge extrusion direction")
                         .build());

  register_parameter(define_float_parameter("edge_direction_z", 0.0F)
                         .label("Direction Z")
                         .range(-1.0, 1.0)
                         .category("Edge Extrusion")
                         .visible_when("edge_direction_mode", 1)
                         .description("Z component of custom edge extrusion direction")
                         .build());
}

core::Result<std::shared_ptr<core::GeometryContainer>> PolyExtrudeSOP::execute() {
  auto input = get_input_data(0);
  if (input == nullptr) {
    set_error("No input geometry");
    return {(std::string) "No input geometry"};
  }

  // Get extrusion type
  const int extrusion_type = get_parameter<int>("extrusion_type", 0);

  if (extrusion_type == 0) {
    return extrude_faces();
  }
  if (extrusion_type == 1) {
    return extrude_edges();
  }
  if (extrusion_type == 2) {
    return extrude_points();
  }

  set_error("Invalid extrusion type");
  return {(std::string) "Invalid extrusion type"};
}

std::shared_ptr<core::GeometryContainer> PolyExtrudeSOP::extrude_faces() {
  auto input = get_input_data(0);
  if (input == nullptr) {
    set_error("No input geometry");
    return nullptr;
  }

  // Get parameters
  const float distance = get_parameter<float>("distance", 1.0F);
  const float inset = get_parameter<float>("inset", 0.0F);
  const bool individual_faces = get_parameter<bool>("individual_faces", true);

  // Get input positions
  const auto* input_positions = input->get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (input_positions == nullptr) {
    set_error("Input geometry has no position attribute");
    return nullptr;
  }

  const auto& input_topology = input->topology();
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
    const auto& vert_indices = input_topology.get_primitive_vertices(prim_idx);
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
  auto* result_positions = result->get_point_attribute_typed<core::Vec3f>(attrs::P);

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
    const auto& vert_indices = input_topology.get_primitive_vertices(prim_idx);
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

std::shared_ptr<core::GeometryContainer> PolyExtrudeSOP::extrude_edges() {
  auto input = get_input_data(0);
  if (input == nullptr) {
    set_error("No input geometry");
    return nullptr;
  }

  // Get parameters
  const float distance = get_parameter<float>("distance", 1.0F);
  const bool individual = get_parameter<int>("individual_faces", 1) != 0;
  const int direction_mode = get_parameter<int>("edge_direction_mode", 0);

  // Custom direction vector (if mode = 1)
  const float dir_x = get_parameter<float>("edge_direction_x", 0.0F);
  const float dir_y = get_parameter<float>("edge_direction_y", 1.0F);
  const float dir_z = get_parameter<float>("edge_direction_z", 0.0F);
  core::Vec3f custom_direction(dir_x, dir_y, dir_z);
  if (custom_direction.squaredNorm() > 0.0001F) {
    custom_direction.normalize();
  } else {
    custom_direction = core::Vec3f(0.0F, 1.0F, 0.0F); // Fallback
  }

  // Get input positions
  const auto* input_positions = input->get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (input_positions == nullptr) {
    set_error("Input geometry has no position attribute");
    return nullptr;
  }

  const auto& input_topology = input->topology();
  const size_t input_prim_count = input_topology.primitive_count();

  if (input_prim_count == 0) {
    set_error("Input geometry has no primitives to extrude");
    return nullptr;
  }

  // Create result container
  auto result = std::make_shared<core::GeometryContainer>();

  // Count edge primitives (2 vertices)
  size_t edge_count = 0;
  for (size_t prim_idx = 0; prim_idx < input_prim_count; ++prim_idx) {
    const auto& vert_indices = input_topology.get_primitive_vertices(prim_idx);
    if (vert_indices.size() == 2) {
      edge_count++;
    }
  }

  if (edge_count == 0) {
    set_error("No edge primitives (2 vertices) found in input geometry");
    return nullptr;
  }

  // Calculate total points and vertices needed
  size_t total_points = 0;
  size_t total_vertices = edge_count * 4; // Each edge creates a quad

  if (individual) {
    // Each edge gets its own 4 points (2 original + 2 extruded)
    total_points = edge_count * 4;
  } else {
    // Share points: original points + extruded copies
    total_points = input_topology.point_count() * 2;
  }

  // Create result with pre-calculated sizes
  result->set_point_count(total_points);
  result->set_vertex_count(total_vertices);

  // Add position attribute
  result->add_point_attribute(attrs::P, core::AttributeType::VEC3F);

  auto* result_positions = result->get_point_attribute_typed<core::Vec3f>(attrs::P);

  if (result_positions == nullptr) {
    set_error("Failed to create position attribute in result");
    return nullptr;
  }

  size_t next_point_idx = 0;
  size_t vertex_offset = 0;

  // Helper: Calculate edge extrusion direction
  auto calculate_edge_normal = [&](const core::Vec3f& p0, const core::Vec3f& p1) -> core::Vec3f {
    // Mode 1: Custom Direction
    if (direction_mode == 1) {
      // Use custom direction directly (ignore edge orientation)
      return custom_direction;
    }

    // Mode 0: Auto (Perpendicular)
    // Calculate direction perpendicular to edge
    core::Vec3f edge_dir = (p1 - p0).normalized();

    // Default up vector for auto mode
    core::Vec3f up(0.0F, 1.0F, 0.0F);

    // If edge is nearly parallel to up vector, use a different reference
    if (std::abs(edge_dir.dot(up)) > 0.99F) {
      // Edge is vertical, use X-axis instead
      up = core::Vec3f(1.0F, 0.0F, 0.0F);
    }

    // Extrusion direction perpendicular to both edge and up
    core::Vec3f extrude_dir = edge_dir.cross(up);
    if (extrude_dir.squaredNorm() > 0.0001F) {
      extrude_dir.normalize();
    } else {
      // Fallback if cross product fails
      extrude_dir = core::Vec3f(0.0F, 0.0F, 1.0F);
    }

    return extrude_dir;
  };

  if (individual) {
    // Individual mode: Each edge gets its own 4 points
    for (size_t prim_idx = 0; prim_idx < input_prim_count; ++prim_idx) {
      const auto& vert_indices = input_topology.get_primitive_vertices(prim_idx);

      // Skip non-edge primitives
      if (vert_indices.size() != 2) {
        continue;
      }

      // Get the two points of the edge
      const int p0_idx = input_topology.get_vertex_point(vert_indices[0]);
      const int p1_idx = input_topology.get_vertex_point(vert_indices[1]);

      const core::Vec3f p0 = (*input_positions)[p0_idx];
      const core::Vec3f p1 = (*input_positions)[p1_idx];

      // Calculate extrusion direction
      const core::Vec3f extrude_dir = calculate_edge_normal(p0, p1);

      // Create extruded points
      const core::Vec3f p2 = p1 + extrude_dir * distance;
      const core::Vec3f p3 = p0 + extrude_dir * distance;

      // Create 4 new points for this edge
      const int new_p0 = static_cast<int>(next_point_idx++);
      const int new_p1 = static_cast<int>(next_point_idx++);
      const int new_p2 = static_cast<int>(next_point_idx++);
      const int new_p3 = static_cast<int>(next_point_idx++);

      (*result_positions)[new_p0] = p0;
      (*result_positions)[new_p1] = p1;
      (*result_positions)[new_p2] = p2;
      (*result_positions)[new_p3] = p3;

      // Create quad face (p0 -> p1 -> p2 -> p3)
      std::vector<int> quad_verts;
      quad_verts.reserve(4);

      result->topology().set_vertex_point(vertex_offset, new_p0);
      quad_verts.push_back(static_cast<int>(vertex_offset++));

      result->topology().set_vertex_point(vertex_offset, new_p1);
      quad_verts.push_back(static_cast<int>(vertex_offset++));

      result->topology().set_vertex_point(vertex_offset, new_p2);
      quad_verts.push_back(static_cast<int>(vertex_offset++));

      result->topology().set_vertex_point(vertex_offset, new_p3);
      quad_verts.push_back(static_cast<int>(vertex_offset++));

      result->add_primitive(quad_verts);
    }
  } else {
    // Shared mode: Copy original points, then create extruded versions
    const size_t input_point_count = input_topology.point_count();

    // First, copy all original points
    for (size_t i = 0; i < input_point_count; ++i) {
      (*result_positions)[i] = (*input_positions)[i];
    }

    // Then create extruded versions of all points
    // We need to calculate the average extrusion direction for each point
    // by considering all edges that use that point
    std::vector<core::Vec3f> point_extrude_dirs(input_point_count, core::Vec3f(0.0F, 0.0F, 0.0F));
    std::vector<int> point_edge_count(input_point_count, 0);

    // First pass: Accumulate extrusion directions for each point
    for (size_t prim_idx = 0; prim_idx < input_prim_count; ++prim_idx) {
      const auto& vert_indices = input_topology.get_primitive_vertices(prim_idx);

      if (vert_indices.size() != 2) {
        continue;
      }

      const int p0_idx = input_topology.get_vertex_point(vert_indices[0]);
      const int p1_idx = input_topology.get_vertex_point(vert_indices[1]);

      const core::Vec3f p0 = (*input_positions)[p0_idx];
      const core::Vec3f p1 = (*input_positions)[p1_idx];

      const core::Vec3f extrude_dir = calculate_edge_normal(p0, p1);

      // Accumulate for both endpoints
      point_extrude_dirs[p0_idx] += extrude_dir;
      point_edge_count[p0_idx]++;

      point_extrude_dirs[p1_idx] += extrude_dir;
      point_edge_count[p1_idx]++;
    }

    // Average and create extruded points
    for (size_t i = 0; i < input_point_count; ++i) {
      if (point_edge_count[i] > 0) {
        core::Vec3f avg_dir = point_extrude_dirs[i] / static_cast<float>(point_edge_count[i]);
        if (avg_dir.squaredNorm() > 0.0001F) {
          avg_dir.normalize();
        }
        (*result_positions)[input_point_count + i] = (*input_positions)[i] + avg_dir * distance;
      } else {
        // Point not used by any edge - just copy it
        (*result_positions)[input_point_count + i] = (*input_positions)[i];
      }
    }

    // Second pass: Create quad primitives using shared points
    for (size_t prim_idx = 0; prim_idx < input_prim_count; ++prim_idx) {
      const auto& vert_indices = input_topology.get_primitive_vertices(prim_idx);

      if (vert_indices.size() != 2) {
        continue;
      }

      const int p0_idx = input_topology.get_vertex_point(vert_indices[0]);
      const int p1_idx = input_topology.get_vertex_point(vert_indices[1]);

      // Point indices: original at [i], extruded at [input_point_count + i]
      const int original_p0 = p0_idx;
      const int original_p1 = p1_idx;
      const int extruded_p0 = static_cast<int>(input_point_count) + p0_idx;
      const int extruded_p1 = static_cast<int>(input_point_count) + p1_idx;

      // Create quad face (original_p0 -> original_p1 -> extruded_p1 ->
      // extruded_p0)
      std::vector<int> quad_verts;
      quad_verts.reserve(4);

      result->topology().set_vertex_point(vertex_offset, original_p0);
      quad_verts.push_back(static_cast<int>(vertex_offset++));

      result->topology().set_vertex_point(vertex_offset, original_p1);
      quad_verts.push_back(static_cast<int>(vertex_offset++));

      result->topology().set_vertex_point(vertex_offset, extruded_p1);
      quad_verts.push_back(static_cast<int>(vertex_offset++));

      result->topology().set_vertex_point(vertex_offset, extruded_p0);
      quad_verts.push_back(static_cast<int>(vertex_offset++));

      result->add_primitive(quad_verts);
    }
  }

  return result;
}

std::shared_ptr<core::GeometryContainer> PolyExtrudeSOP::extrude_points() {
  auto input = get_input_data(0);
  if (input == nullptr) {
    set_error("No input geometry");
    return nullptr;
  }

  // Get parameters
  const float distance = get_parameter<float>("distance", 1.0F);
  const int direction_mode = get_parameter<int>("edge_direction_mode", 0);

  // Custom direction vector (reuse edge direction parameters)
  const float dir_x = get_parameter<float>("edge_direction_x", 0.0F);
  const float dir_y = get_parameter<float>("edge_direction_y", 1.0F);
  const float dir_z = get_parameter<float>("edge_direction_z", 0.0F);
  core::Vec3f custom_direction(dir_x, dir_y, dir_z);
  if (custom_direction.squaredNorm() > 0.0001F) {
    custom_direction.normalize();
  } else {
    custom_direction = core::Vec3f(0.0F, 1.0F, 0.0F); // Fallback
  }

  // Get input positions
  const auto* input_positions = input->get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (input_positions == nullptr) {
    set_error("Input geometry has no position attribute");
    return nullptr;
  }

  const auto& input_topology = input->topology();
  const size_t input_point_count = input_topology.point_count();

  if (input_point_count == 0) {
    set_error("Input geometry has no points to extrude");
    return nullptr;
  }

  // Create result container
  auto result = std::make_shared<core::GeometryContainer>();

  // For points: Create line segments from each point
  // Each point creates 2 points (original + extruded) and 1 edge primitive
  const size_t total_points = input_point_count * 2;
  const size_t total_vertices = input_point_count * 2; // Each edge has 2 vertices

  result->set_point_count(total_points);
  result->set_vertex_count(total_vertices);

  // Add position attribute
  result->add_point_attribute(attrs::P, core::AttributeType::VEC3F);

  auto* result_positions = result->get_point_attribute_typed<core::Vec3f>(attrs::P);

  if (result_positions == nullptr) {
    set_error("Failed to create position attribute in result");
    return nullptr;
  }

  size_t vertex_offset = 0;

  // Determine extrusion direction for each point
  core::Vec3f extrude_dir = custom_direction;

  // If auto mode (mode 0), use the custom direction as default
  // (For points, there's no "perpendicular" - we just use a direction)
  if (direction_mode == 0) {
    // Auto mode: use Y-up as default for points
    extrude_dir = core::Vec3f(0.0F, 1.0F, 0.0F);
  }

  // Process each point
  for (size_t pt_idx = 0; pt_idx < input_point_count; ++pt_idx) {
    const core::Vec3f original_pos = (*input_positions)[pt_idx];
    const core::Vec3f extruded_pos = original_pos + extrude_dir * distance;

    // Create two points: original and extruded
    const int p0 = static_cast<int>(pt_idx * 2);
    const int p1 = static_cast<int>(pt_idx * 2 + 1);

    (*result_positions)[p0] = original_pos;
    (*result_positions)[p1] = extruded_pos;

    // Create edge primitive connecting them
    std::vector<int> edge_verts;
    edge_verts.reserve(2);

    result->topology().set_vertex_point(vertex_offset, p0);
    edge_verts.push_back(static_cast<int>(vertex_offset++));

    result->topology().set_vertex_point(vertex_offset, p1);
    edge_verts.push_back(static_cast<int>(vertex_offset++));

    result->add_primitive(edge_verts);
  }

  return result;
}

} // namespace nodo::sop
