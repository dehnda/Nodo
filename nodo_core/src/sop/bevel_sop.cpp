#include "nodo/sop/bevel_sop.hpp"
#include "nodo/core/attribute_types.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <unordered_map>
#include <utility>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::sop {

struct EdgeKey {
  int p0, p1;

  EdgeKey(int a, int b) : p0(std::min(a, b)), p1(std::max(a, b)) {}
  bool operator==(const EdgeKey &other) const {
    return p0 == other.p0 && p1 == other.p1;
  }
};

struct EdgeKeyHash {
  std::size_t operator()(const EdgeKey &k) const {
    return std::hash<int>()(k.p0) ^ std::hash<int>()(k.p1);
  }
};

struct EdgeInfo {
  std::vector<size_t> adjacent_primitives;
  core::Vec3f avg_normal;
  float angle;
};

BevelSOP::BevelSOP(const std::string &name) : SOPNode(name, "Bevel") {
  // Input geometry port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Parameters (schema)
  register_parameter(define_float_parameter("width", DEFAULT_WIDTH)
                         .label("Width")
                         .range(0.0, 1000.0)
                         .category("Bevel")
                         .description("Bevel width/offset distance")
                         .build());

  register_parameter(
      define_int_parameter("segments", DEFAULT_SEGMENTS)
          .label("Segments")
          .range(1, 16)
          .category("Bevel")
          .description(
              "Number of segments for rounded bevel (topology-preserving "
              "placeholder uses this as a smoothing hint)")
          .build());

  register_parameter(
      define_float_parameter("profile", DEFAULT_PROFILE)
          .label("Profile")
          .range(0.0, 1.0)
          .category("Bevel")
          .description(
              "Bevel profile shape (0.0 = linear, 0.5 = smooth, 1.0 = sharp)")
          .build());

  register_parameter(
      define_int_parameter("bevel_type", static_cast<int>(BevelType::Vertex))
          .label("Mode")
          .options({"Vertex", "Edge", "Face"})
          .category("Bevel")
          .description("Bevel mode: Vertex/Edge/Face (Face = inset)")
          .build());

  register_parameter(define_bool_parameter("clamp_overlap", true)
                         .label("Clamp Overlap")
                         .category("Bevel")
                         .description("Attempt to avoid overlaps (best effort)")
                         .build());

  // Optional: angle limit for future edge selection (not used in placeholder)
  register_parameter(
      define_float_parameter("angle_limit", 30.0f)
          .label("Angle Limit")
          .range(0.0, 180.0)
          .category("Limits")
          .description(
              "Only bevel edges above this angle (not fully implemented)")
          .build());
}

std::shared_ptr<core::GeometryContainer> BevelSOP::execute() {
  auto input = get_input_data(0);
  if (!input) {
    set_error("No input geometry");
    return nullptr;
  }

  auto &topology = input->topology();
  auto *positions = input->positions();

  std::unordered_map<EdgeKey, EdgeInfo, EdgeKeyHash> edge_map;

  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &prim_verts = topology.get_primitive_vertices(prim_idx);

    // For each edge in this primitive (consecutive vertex pairs)
    for (size_t i = 0; i < prim_verts.size(); ++i) {
      size_t next_i = (i + 1) % prim_verts.size(); // Wrap around

      int v0 = prim_verts[i];
      int v1 = prim_verts[next_i];

      // Get the points these vertices reference
      int p0 = topology.get_vertex_point(v0);
      int p1 = topology.get_vertex_point(v1);

      EdgeKey key(p0, p1);
      edge_map[key].adjacent_primitives.push_back(prim_idx);
    }
  }

  // Compute face normal for each primitive
  std::vector<core::Vec3f> face_normals(
      topology.primitive_count(),
      core::Vec3f(0.0F, 1.0F, 0.0F)); // Default to +Y

  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &verts = topology.get_primitive_vertices(prim_idx);
    if (verts.size() < 3) {
      continue; // Keep default normal for degenerate primitives
    }

    // Get first 3 points to compute normal
    const int point0 = topology.get_vertex_point(verts[0]);
    const int point1 = topology.get_vertex_point(verts[1]);
    const int point2 = topology.get_vertex_point(verts[2]);

    const core::Vec3f pos0 = (*positions)[point0];
    const core::Vec3f pos1 = (*positions)[point1];
    const core::Vec3f pos2 = (*positions)[point2];

    const core::Vec3f edge1 = pos1 - pos0;
    const core::Vec3f edge2 = pos2 - pos0;
    const core::Vec3f normal = edge1.cross(edge2);

    // Normalize if not degenerate
    const float length = normal.norm();
    if (length > 1e-6F) {
      face_normals[prim_idx] = normal / length;
    }
  }

  const float angle_threshold = get_parameter<float>("angle_limit");
  constexpr float rad_to_deg = 180.0F / static_cast<float>(M_PI);
  constexpr float deg_to_rad = static_cast<float>(M_PI) / 180.0F;

  std::vector<EdgeKey> edges_to_bevel;

  for (auto &[edge_key, edge_info] : edge_map) {
    // Only bevel edges shared by exactly 2 faces (internal edges)
    if (edge_info.adjacent_primitives.size() != 2) {
      continue;
    }

    // Get normals of the two adjacent faces
    const core::Vec3f &normal0 = face_normals[edge_info.adjacent_primitives[0]];
    const core::Vec3f &normal1 = face_normals[edge_info.adjacent_primitives[1]];

    // Calculate angle between faces
    const float dot_product = normal0.dot(normal1);
    const float angle_deg =
        std::acos(std::clamp(dot_product, -1.0F, 1.0F)) * rad_to_deg;

    edge_info.angle = angle_deg;
    edge_info.avg_normal = (normal0 + normal1).normalized();

    // If angle exceeds threshold, mark for beveling
    if (angle_deg > angle_threshold) {
      edges_to_bevel.push_back(edge_key);
    }
  }

  // Debug output to verify what we found
  std::cout << "\n=== Bevel SOP Analysis ===\n";
  std::cout << "Total edges found: " << edge_map.size() << '\n';
  std::cout << "Edges with 2 adjacent faces: ";
  size_t internal_edges = 0;
  for (const auto &[key, info] : edge_map) {
    if (info.adjacent_primitives.size() == 2) {
      internal_edges++;
    }
  }
  std::cout << internal_edges << '\n';
  std::cout << "Edges to bevel (angle > " << angle_threshold
            << "°): " << edges_to_bevel.size() << '\n';

  // Print details of first few edges to bevel
  if (!edges_to_bevel.empty()) {
    std::cout << "\nFirst few edges to bevel:\n";
    const size_t num_to_print = std::min(size_t{5}, edges_to_bevel.size());
    for (size_t i = 0; i < num_to_print; ++i) {
      const EdgeKey &key = edges_to_bevel[i];
      const EdgeInfo &info = edge_map[key];
      std::cout << "  Edge (" << key.p0 << " -> " << key.p1
                << "): angle = " << info.angle << "°\n";
    }
  }
  std::cout << "========================\n" << std::flush;

  // If no edges to bevel, return input unchanged
  if (edges_to_bevel.empty()) {
    return input;
  }

  // Check bevel mode
  const int bevel_mode = get_parameter<int>("bevel_type");
  const BevelType bevel_type = static_cast<BevelType>(bevel_mode);

  // Currently only Edge mode is implemented
  if (bevel_type != BevelType::Edge) {
    std::cout << "Bevel mode " << bevel_mode << " not yet implemented\n";
    set_error("Only Edge bevel mode is currently implemented");
    return input;
  }

  const float bevel_width = get_parameter<float>("width");

  std::cout << "\n=== Edge Bevel Implementation ===\n";

  // ========================================================================
  // Step 1: Build set of beveled edges for quick lookup
  // ========================================================================

  std::unordered_set<EdgeKey, EdgeKeyHash> beveled_edges_set(
      edges_to_bevel.begin(), edges_to_bevel.end());

  std::vector<core::Vec3f> all_positions;

  // Don't copy original points - we'll only create the offset points we need
  // This avoids having unused corner points in the final geometry

  // We'll create offset points per-face as we process them
  // This allows each face to have its own offset points that move toward that
  // face Key: (face_idx, point_idx) -> offset_point_idx
  std::map<std::pair<size_t, int>, int> face_point_to_offset;

  // ========================================================================
  // Step 2: For each face, create offset points specific to that face
  // ========================================================================

  // Collect all primitives we'll create
  std::vector<std::vector<int>> output_prims;
  std::unordered_set<EdgeKey, EdgeKeyHash> processed_edges;

  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &verts = topology.get_primitive_vertices(prim_idx);
    std::vector<int> face_points;

    // Get original face points
    for (int vert_idx : verts) {
      face_points.push_back(topology.get_vertex_point(vert_idx));
    }

    std::cout << "\nFace " << prim_idx << ": [";
    for (size_t i = 0; i < face_points.size(); ++i) {
      std::cout << face_points[i];
      if (i < face_points.size() - 1)
        std::cout << ", ";
    }
    std::cout << "]\n";

    const core::Vec3f &face_normal = face_normals[prim_idx];

    // Build new inset face by creating offset points for beveled edges
    std::vector<int> new_face;

    for (size_t i = 0; i < face_points.size(); ++i) {
      int curr = face_points[i];
      int next = face_points[(i + 1) % face_points.size()];

      EdgeKey edge(curr, next);

      // Check if this edge should be beveled
      if (beveled_edges_set.find(edge) != beveled_edges_set.end()) {
        // This edge is beveled - create an offset point for this face
        auto key = std::make_pair(prim_idx, curr);

        // Check if we already created an offset point for this corner on this
        // face
        if (face_point_to_offset.find(key) == face_point_to_offset.end()) {
          // Create new offset point
          int offset_idx = static_cast<int>(all_positions.size());

          const core::Vec3f &pos = (*positions)[curr];

          // For proper inset: calculate face center and move toward it
          core::Vec3f face_center = core::Vec3f::Zero();
          for (int fp : face_points) {
            face_center += (*positions)[fp];
          }
          face_center /= static_cast<float>(face_points.size());

          // Direction from this corner toward face center
          core::Vec3f to_center = (face_center - pos).normalized();

          // Move toward center
          core::Vec3f offset_pos = pos + to_center * bevel_width;
          all_positions.push_back(offset_pos);

          face_point_to_offset[key] = offset_idx;
          std::cout << "  Created offset point " << offset_idx << " for corner "
                    << curr << "\n";
        }

        // Use the offset point
        new_face.push_back(face_point_to_offset[key]);
      } else {
        // Edge not beveled - use original point
        new_face.push_back(curr);
      }
    }

    if (!new_face.empty()) {
      output_prims.push_back(new_face);
      std::cout << "  New inset face: [";
      for (size_t i = 0; i < new_face.size(); ++i) {
        std::cout << new_face[i];
        if (i < new_face.size() - 1)
          std::cout << ", ";
      }
      std::cout << "]\n";
    }
  }

  std::cout << "\nTotal points created: " << all_positions.size() << "\n";

  // ========================================================================
  // Step 3: Create bridge quads connecting original edges to offset edges
  // ========================================================================

  std::unordered_set<EdgeKey, EdgeKeyHash> processed_bridge_edges;

  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &verts = topology.get_primitive_vertices(prim_idx);
    std::vector<int> face_points;

    for (int vert_idx : verts) {
      face_points.push_back(topology.get_vertex_point(vert_idx));
    }

    // For each edge on this face
    for (size_t i = 0; i < face_points.size(); ++i) {
      int curr = face_points[i];
      int next = face_points[(i + 1) % face_points.size()];

      EdgeKey edge(curr, next);

      // Only process beveled edges, and only once per edge
      if (beveled_edges_set.find(edge) != beveled_edges_set.end() &&
          processed_bridge_edges.find(edge) == processed_bridge_edges.end()) {

        // Find the two faces that share this edge
        const auto &adjacent_faces = edge_map[edge].adjacent_primitives;

        if (adjacent_faces.size() == 2) {
          size_t face0 = adjacent_faces[0];
          size_t face1 = adjacent_faces[1];

          // Get offset points for this edge from both faces
          auto key0_p0 = std::make_pair(face0, edge.p0);
          auto key0_p1 = std::make_pair(face0, edge.p1);
          auto key1_p0 = std::make_pair(face1, edge.p0);
          auto key1_p1 = std::make_pair(face1, edge.p1);

          // Check if all offset points exist
          if (face_point_to_offset.find(key0_p0) !=
                  face_point_to_offset.end() &&
              face_point_to_offset.find(key0_p1) !=
                  face_point_to_offset.end() &&
              face_point_to_offset.find(key1_p0) !=
                  face_point_to_offset.end() &&
              face_point_to_offset.find(key1_p1) !=
                  face_point_to_offset.end()) {

            int offset0_p0 = face_point_to_offset[key0_p0];
            int offset0_p1 = face_point_to_offset[key0_p1];
            int offset1_p0 = face_point_to_offset[key1_p0];
            int offset1_p1 = face_point_to_offset[key1_p1];

            // Create a single quad connecting the four offset points
            // This forms the bridge strip between the two adjacent faces
            std::vector<int> bridge_quad = {offset0_p0, offset0_p1, offset1_p1,
                                            offset1_p0};
            output_prims.push_back(bridge_quad);

            processed_bridge_edges.insert(edge);

            std::cout << "  Created bridge quad for edge (" << edge.p0 << "->"
                      << edge.p1 << "): [" << offset0_p0 << ", " << offset0_p1
                      << ", " << offset1_p1 << ", " << offset1_p0 << "]\n";
          }
        }
      }
    }
  }

  std::cout << "Created " << processed_bridge_edges.size() << " bridge quads\n";

  // ========================================================================
  // Step 3b: Create corner triangles
  // ========================================================================

  // For each original corner point, find all offset points around it and create
  // a triangle A corner has 3 edges meeting at it (for a cube)
  std::unordered_map<int, std::vector<int>> corner_to_offsets;

  for (const auto &[key, offset_idx] : face_point_to_offset) {
    int original_point = key.second;
    corner_to_offsets[original_point].push_back(offset_idx);
  }

  for (const auto &[corner, offsets] : corner_to_offsets) {
    if (offsets.size() == 3) {
      // Create corner triangle from the 3 offset points
      std::vector<int> corner_tri = {offsets[0], offsets[1], offsets[2]};
      output_prims.push_back(corner_tri);
      std::cout << "  Created corner triangle for point " << corner << ": ["
                << offsets[0] << ", " << offsets[1] << ", " << offsets[2]
                << "]\n";
    }
  }

  // ========================================================================
  // Step 4: Build result geometry
  // ========================================================================

  auto result = std::make_shared<core::GeometryContainer>();
  result->set_point_count(all_positions.size());

  // Add positions
  result->add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *result_pos = result->get_point_attribute_typed<core::Vec3f>(attrs::P);
  for (size_t i = 0; i < all_positions.size(); ++i) {
    (*result_pos)[i] = all_positions[i];
  }

  // ========================================================================
  // Step 5: Add all primitives to result
  // ========================================================================

  std::cout << "\n=== Final Geometry Summary ===\n";
  std::cout << "Total points: " << all_positions.size() << "\n";
  std::cout << "  Original: 0-" << (input->point_count() - 1) << "\n";
  std::cout << "  Offset: " << input->point_count() << "-"
            << (all_positions.size() - 1) << "\n";
  std::cout << "Total primitives: " << output_prims.size() << "\n";

  // Count primitive types
  int inset_faces = 0;
  int bridge_quads = 0;
  int original_faces = 0;

  for (const auto &prim : output_prims) {
    // Heuristic: if all points are >= original point count, it's an inset face
    bool all_offset = true;
    bool any_offset = false;
    for (int pt : prim) {
      if (pt < static_cast<int>(input->point_count())) {
        all_offset = false;
      } else {
        any_offset = true;
      }
    }

    if (all_offset) {
      inset_faces++;
    } else if (any_offset && prim.size() == 4) {
      bridge_quads++;
    } else {
      original_faces++;
    }
  }

  std::cout << "  Inset faces: " << inset_faces << "\n";
  std::cout << "  Bridge quads: " << bridge_quads << "\n";
  std::cout << "  Original faces: " << original_faces << "\n";
  std::cout << "==============================\n\n";

  size_t total_verts = 0;
  for (const auto &prim : output_prims) {
    total_verts += prim.size();
  }

  result->set_vertex_count(total_verts);

  size_t vert_idx = 0;
  for (const auto &prim : output_prims) {
    std::vector<int> prim_verts;
    for (int pt : prim) {
      result->topology().set_vertex_point(vert_idx, pt);
      prim_verts.push_back(static_cast<int>(vert_idx));
      vert_idx++;
    }
    result->add_primitive(prim_verts);
  }

  std::cout << "\nCreated " << output_prims.size() << " primitives total\n";
  std::cout << "Bevel complete!\n" << std::flush;

  return result;
}

} // namespace nodo::sop
