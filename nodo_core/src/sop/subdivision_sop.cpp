#include "nodo/sop/sop_utils.hpp"
using namespace nodo;
#include "nodo/core/geometry_container.hpp"
#include "nodo/core/math.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/core/types.hpp"
#include "nodo/sop/sop_utils.hpp"
#include "nodo/sop/subdivisions_sop.hpp"
#include <iostream>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::sop {

SubdivisionSOP::SubdivisionSOP(const std::string &name)
    : SOPNode(name, "Subdivision") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_int_parameter("subdivision_levels", 1)
                         .label("Subdivision Levels")
                         .range(0, 5)
                         .category("Subdivision")
                         .build());

  register_parameter(define_int_parameter("method", 0)
                         .label("Method")
                         .options({"Catmull-Clark", "Simple"})
                         .category("Subdivision")
                         .build());

  register_parameter(define_int_parameter("preserve_boundaries", 1)
                         .label("Preserve Boundaries")
                         .range(0, 1)
                         .category("Subdivision")
                         .build());
}

std::shared_ptr<core::GeometryContainer> SubdivisionSOP::execute() {
  auto input = get_input_data(0);
  if (!input) {
    return nullptr;
  }

  // Get parameters
  const int levels = get_parameter<int>("subdivision_levels", 1);
  const int method = get_parameter<int>("method", 0);
  const bool preserve_boundaries =
      (get_parameter<int>("preserve_boundaries", 1) != 0);

  // If no subdivision requested, just return clone
  if (levels <= 0) {
    return std::make_shared<core::GeometryContainer>(input->clone());
  }

  // Clone input for modification
  auto output = std::make_shared<core::GeometryContainer>(input->clone());

  // Apply subdivision based on method
  for (int level = 0; level < levels; ++level) {
    if (method == 0) {
      // Catmull-Clark
      apply_catmull_clark(*output, preserve_boundaries);
    } else {
      // Simple (midpoint)
      apply_simple_subdivision(*output);
    }
  }

  // Recompute normals after subdivision
  utils::compute_hard_edge_normals(*output);
  return output;
}

namespace {

// Edge key for hashing (always stores smaller index first)
struct Edge {
  int v0, v1;

  Edge(int a, int b) : v0(std::min(a, b)), v1(std::max(a, b)) {}

  bool operator==(const Edge &other) const {
    return v0 == other.v0 && v1 == other.v1;
  }

  bool operator<(const Edge &other) const {
    return v0 < other.v0 || (v0 == other.v0 && v1 < other.v1);
  }
};

} // namespace

void SubdivisionSOP::apply_simple_subdivision(
    core::GeometryContainer &container) {
  const auto &old_topology = container.topology();
  const size_t old_point_count = old_topology.point_count();
  const size_t old_prim_count = old_topology.primitive_count();

  // Get P attribute
  auto *old_P = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (old_P == nullptr) {
    std::cerr << "SubdivisionSOP: No P attribute found\n";
    return;
  }

  // Build edge midpoint map
  std::map<Edge, int> edge_midpoints;
  int next_point_idx = static_cast<int>(old_point_count);

  // Collect old positions
  std::vector<core::Vec3f> old_positions(old_point_count);
  for (size_t i = 0; i < old_point_count; ++i) {
    old_positions[i] = (*old_P)[i];
  }

  // Find all unique edges and create midpoints
  std::vector<core::Vec3f> new_points;
  for (size_t prim_idx = 0; prim_idx < old_prim_count; ++prim_idx) {
    const auto &prim_verts = old_topology.get_primitive_vertices(prim_idx);
    const size_t num_verts = prim_verts.size();

    for (size_t i = 0; i < num_verts; ++i) {
      const size_t next_i = (i + 1) % num_verts;
      const int v0 = old_topology.get_vertex_point(prim_verts[i]);
      const int v1 = old_topology.get_vertex_point(prim_verts[next_i]);

      Edge edge(v0, v1);
      if (edge_midpoints.find(edge) == edge_midpoints.end()) {
        // Create midpoint
        const core::Vec3f midpoint =
            (old_positions[v0] + old_positions[v1]) * 0.5F;
        edge_midpoints[edge] = next_point_idx++;
        new_points.push_back(midpoint);
      }
    }
  }

  // Create new topology
  core::GeometryContainer new_container;
  const size_t total_points = old_point_count + new_points.size();
  new_container.set_point_count(total_points);

  // Add P attribute and copy data
  new_container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *new_P = new_container.get_point_attribute_typed<core::Vec3f>(attrs::P);

  // Copy old points
  for (size_t i = 0; i < old_point_count; ++i) {
    new_P->set(i, old_positions[i]);
  }
  // Copy new midpoints
  for (size_t i = 0; i < new_points.size(); ++i) {
    new_P->set(old_point_count + i, new_points[i]);
  }

  // Create subdivided primitives (split each face into 4)
  for (size_t prim_idx = 0; prim_idx < old_prim_count; ++prim_idx) {
    const auto &prim_verts = old_topology.get_primitive_vertices(prim_idx);
    const size_t num_verts = prim_verts.size();

    if (num_verts == 3) {
      // Triangle: split into 4 triangles
      const int p0 = old_topology.get_vertex_point(prim_verts[0]);
      const int p1 = old_topology.get_vertex_point(prim_verts[1]);
      const int p2 = old_topology.get_vertex_point(prim_verts[2]);

      const int m01 = edge_midpoints[Edge(p0, p1)];
      const int m12 = edge_midpoints[Edge(p1, p2)];
      const int m20 = edge_midpoints[Edge(p2, p0)];

      // 4 corner triangles
      add_triangle(new_container, p0, m01, m20);
      add_triangle(new_container, p1, m12, m01);
      add_triangle(new_container, p2, m20, m12);
      add_triangle(new_container, m01, m12, m20); // center
    } else if (num_verts == 4) {
      // Quad: split into 4 quads
      const int p0 = old_topology.get_vertex_point(prim_verts[0]);
      const int p1 = old_topology.get_vertex_point(prim_verts[1]);
      const int p2 = old_topology.get_vertex_point(prim_verts[2]);
      const int p3 = old_topology.get_vertex_point(prim_verts[3]);

      const int m01 = edge_midpoints[Edge(p0, p1)];
      const int m12 = edge_midpoints[Edge(p1, p2)];
      const int m23 = edge_midpoints[Edge(p2, p3)];
      const int m30 = edge_midpoints[Edge(p3, p0)];

      // Face center point
      const core::Vec3f center = (old_positions[p0] + old_positions[p1] +
                                  old_positions[p2] + old_positions[p3]) *
                                 0.25F;
      const int center_idx = static_cast<int>(new_container.point_count());
      new_container.set_point_count(new_container.point_count() + 1);
      new_P->set(center_idx, center);

      // 4 corner quads
      add_quad(new_container, p0, m01, center_idx, m30);
      add_quad(new_container, p1, m12, center_idx, m01);
      add_quad(new_container, p2, m23, center_idx, m12);
      add_quad(new_container, p3, m30, center_idx, m23);
    }
  }

  // Replace container contents
  container = std::move(new_container);
}

void SubdivisionSOP::apply_catmull_clark(core::GeometryContainer &container,
                                         bool preserve_boundaries) {
  const auto &old_topology = container.topology();
  const size_t old_point_count = old_topology.point_count();
  const size_t old_prim_count = old_topology.primitive_count();

  // Get P attribute
  auto *old_P = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (old_P == nullptr) {
    std::cerr << "SubdivisionSOP: No P attribute found\n";
    return;
  }

  // Check if mesh has triangles - if so, we'll convert them to quads during
  // subdivision
  bool has_triangles = false;
  for (size_t i = 0; i < old_prim_count; ++i) {
    const auto &verts = old_topology.get_primitive_vertices(i);
    if (verts.size() == 3) {
      has_triangles = true;
      break;
    }
  }

  // Collect old positions
  std::vector<core::Vec3f> old_positions(old_point_count);
  for (size_t i = 0; i < old_point_count; ++i) {
    old_positions[i] = (*old_P)[i];
  }

  // Step 1: Calculate face points (average of all face vertices)
  std::vector<core::Vec3f> face_points(old_prim_count);
  for (size_t prim_idx = 0; prim_idx < old_prim_count; ++prim_idx) {
    const auto &prim_verts = old_topology.get_primitive_vertices(prim_idx);
    core::Vec3f sum(0.0F, 0.0F, 0.0F);
    for (int vert_idx : prim_verts) {
      const int point_idx = old_topology.get_vertex_point(vert_idx);
      sum += old_positions[point_idx];
    }
    face_points[prim_idx] = sum / static_cast<float>(prim_verts.size());
  }

  // Step 2: Calculate edge points
  // Build edge connectivity (which faces share each edge)
  std::map<Edge, std::vector<int>> edge_to_faces;
  std::map<Edge, std::pair<int, int>> edge_endpoints;

  for (size_t prim_idx = 0; prim_idx < old_prim_count; ++prim_idx) {
    const auto &prim_verts = old_topology.get_primitive_vertices(prim_idx);
    const size_t num_verts = prim_verts.size();

    for (size_t i = 0; i < num_verts; ++i) {
      const size_t next_i = (i + 1) % num_verts;
      const int p0 = old_topology.get_vertex_point(prim_verts[i]);
      const int p1 = old_topology.get_vertex_point(prim_verts[next_i]);

      Edge edge(p0, p1);
      edge_to_faces[edge].push_back(static_cast<int>(prim_idx));
      edge_endpoints[edge] = {p0, p1};
    }
  }

  // Calculate edge point positions
  std::map<Edge, core::Vec3f> edge_points;
  for (const auto &[edge, faces] : edge_to_faces) {
    const auto &[p0, p1] = edge_endpoints[edge];

    if (faces.size() == 2) {
      // Interior edge: average of 2 face points + 2 edge endpoints
      const core::Vec3f avg = (face_points[faces[0]] + face_points[faces[1]] +
                               old_positions[p0] + old_positions[p1]) *
                              0.25F;
      edge_points[edge] = avg;
    } else {
      // Boundary edge: midpoint of endpoints
      edge_points[edge] = (old_positions[p0] + old_positions[p1]) * 0.5F;
    }
  }

  // Step 3: Calculate new vertex positions
  // Build point-to-edges and point-to-faces connectivity
  std::unordered_map<int, std::vector<Edge>> point_to_edges;
  std::unordered_map<int, std::vector<int>> point_to_faces;

  for (const auto &[edge, faces] : edge_to_faces) {
    point_to_edges[edge.v0].push_back(edge);
    point_to_edges[edge.v1].push_back(edge);

    for (int face_idx : faces) {
      point_to_faces[edge.v0].push_back(face_idx);
      point_to_faces[edge.v1].push_back(face_idx);
    }
  }

  std::vector<core::Vec3f> new_vertex_positions(old_point_count);
  for (size_t i = 0; i < old_point_count; ++i) {
    const auto edges_it = point_to_edges.find(static_cast<int>(i));
    const auto faces_it = point_to_faces.find(static_cast<int>(i));

    if (edges_it == point_to_edges.end() || faces_it == point_to_faces.end()) {
      // No connectivity - keep original position
      new_vertex_positions[i] = old_positions[i];
      continue;
    }

    const auto &edges = edges_it->second;
    const auto &faces = faces_it->second;

    // Remove duplicates from faces
    std::unordered_set<int> unique_faces(faces.begin(), faces.end());

    const int num_edges = static_cast<int>(edges.size());
    const int num_faces = static_cast<int>(unique_faces.size());

    // For triangulated meshes, use simpler averaging
    // Average of: face centers + edge midpoints + original position

    core::Vec3f face_avg(0.0F, 0.0F, 0.0F);
    if (num_faces > 0) {
      for (int face_idx : unique_faces) {
        face_avg += face_points[face_idx];
      }
      face_avg /= static_cast<float>(num_faces);
    }

    // R = average of edge points (the new points we created on edges)
    core::Vec3f edge_points_avg(0.0F, 0.0F, 0.0F);
    if (num_edges > 0) {
      for (const auto &edge : edges) {
        // Use the edge point position we calculated earlier
        edge_points_avg += edge_points.at(edge);
      }
      edge_points_avg /= static_cast<float>(num_edges);
    }

    // Boundary vertex: if number of faces < number of edges, it's on the
    // boundary
    if (num_faces < num_edges) {
      // Boundary vertex: average of edge midpoints and original position
      new_vertex_positions[i] = (edge_points_avg + old_positions[i]) * 0.5F;
    } else if (num_faces == 3) {
      // Triangle mesh: average face, edge, and original position
      new_vertex_positions[i] =
          (face_avg + edge_points_avg + old_positions[i]) / 3.0F;
    } else {
      // Catmull-Clark for quads and n-gons (interior vertex)
      const float n_val = static_cast<float>(num_faces);
      new_vertex_positions[i] = (face_avg + edge_points_avg * 2.0F +
                                 old_positions[i] * (n_val - 3.0F)) /
                                n_val;
    }
  }

  // Step 4: Build new topology
  core::GeometryContainer new_container;

  // Count total points needed
  const size_t num_face_points = old_prim_count;
  const size_t num_edge_points = edge_points.size();
  const size_t total_points =
      old_point_count + num_face_points + num_edge_points;

  new_container.set_point_count(total_points);
  new_container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *new_P = new_container.get_point_attribute_typed<core::Vec3f>(attrs::P);

  // Copy new vertex positions
  for (size_t i = 0; i < old_point_count; ++i) {
    new_P->set(i, new_vertex_positions[i]);
  }

  // Add face points
  for (size_t i = 0; i < num_face_points; ++i) {
    new_P->set(old_point_count + i, face_points[i]);
  }

  // Add edge points and build index map
  std::map<Edge, int> edge_point_indices;
  int edge_point_idx = 0;
  for (const auto &[edge, pos] : edge_points) {
    const int idx =
        static_cast<int>(old_point_count + num_face_points + edge_point_idx);
    new_P->set(idx, pos);
    edge_point_indices[edge] = idx;
    ++edge_point_idx;
  }

  // Step 5: Create new faces
  // For triangles: create 3 quads (one per corner)
  // For quads: create 4 quads (one per corner)

  for (size_t prim_idx = 0; prim_idx < old_prim_count; ++prim_idx) {
    const auto &prim_verts = old_topology.get_primitive_vertices(prim_idx);
    const size_t num_verts = prim_verts.size();
    const int face_point_idx = static_cast<int>(old_point_count + prim_idx);

    if (num_verts == 3) {
      // Triangle: create 3 quads from center to each edge
      for (size_t i = 0; i < 3; ++i) {
        const size_t next_i = (i + 1) % 3;

        const int curr_vert = old_topology.get_vertex_point(prim_verts[i]);
        const int next_vert = old_topology.get_vertex_point(prim_verts[next_i]);

        // Edge from current to next
        const Edge curr_next_edge(curr_vert, next_vert);

        if (edge_point_indices.find(curr_next_edge) ==
            edge_point_indices.end()) {
          continue;
        }

        const int edge_pt = edge_point_indices[curr_next_edge];

        // For triangles, create a quad from: curr_vert -> edge_to_next ->
        // face_point -> edge_from_prev
        const size_t prev_i = (i + 2) % 3;
        const int prev_vert = old_topology.get_vertex_point(prim_verts[prev_i]);
        const Edge prev_curr_edge(prev_vert, curr_vert);
        const int prev_edge_pt = edge_point_indices[prev_curr_edge];

        add_quad(new_container, curr_vert, edge_pt, face_point_idx,
                 prev_edge_pt);
      }
    } else {
      // Quad or n-gon: create N quads
      for (size_t i = 0; i < num_verts; ++i) {
        const size_t next_i = (i + 1) % num_verts;
        const size_t prev_i = (i + num_verts - 1) % num_verts;

        const int curr_vert = old_topology.get_vertex_point(prim_verts[i]);
        const int next_vert = old_topology.get_vertex_point(prim_verts[next_i]);
        const int prev_vert = old_topology.get_vertex_point(prim_verts[prev_i]);

        // Edge from current to next
        const Edge curr_next_edge(curr_vert, next_vert);
        const int curr_next_edge_pt = edge_point_indices[curr_next_edge];

        // Edge from previous to current
        const Edge prev_curr_edge(prev_vert, curr_vert);
        const int prev_curr_edge_pt = edge_point_indices[prev_curr_edge];

        // Create quad: current_vertex -> edge_to_next -> face_point ->
        // edge_from_prev
        add_quad(new_container, curr_vert, curr_next_edge_pt, face_point_idx,
                 prev_curr_edge_pt);
      }
    }
  }

  // Replace container contents
  container = std::move(new_container);
}

void SubdivisionSOP::add_triangle(core::GeometryContainer &container, int p0,
                                  int p1, int p2) {
  const size_t base_vert = container.vertex_count();
  container.set_vertex_count(base_vert + 3);

  auto &topology = container.topology();
  topology.set_vertex_point(base_vert + 0, p0);
  topology.set_vertex_point(base_vert + 1, p1);
  topology.set_vertex_point(base_vert + 2, p2);

  container.add_primitive({static_cast<int>(base_vert + 0),
                           static_cast<int>(base_vert + 1),
                           static_cast<int>(base_vert + 2)});
}

void SubdivisionSOP::add_quad(core::GeometryContainer &container, int p0,
                              int p1, int p2, int p3) {
  const size_t base_vert = container.vertex_count();
  container.set_vertex_count(base_vert + 4);

  auto &topology = container.topology();
  topology.set_vertex_point(base_vert + 0, p0);
  topology.set_vertex_point(base_vert + 1, p1);
  topology.set_vertex_point(base_vert + 2, p2);
  topology.set_vertex_point(base_vert + 3, p3);

  container.add_primitive(
      {static_cast<int>(base_vert + 0), static_cast<int>(base_vert + 1),
       static_cast<int>(base_vert + 2), static_cast<int>(base_vert + 3)});
}

} // namespace nodo::sop
