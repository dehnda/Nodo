#include "nodo/core/attribute_types.hpp"
#include "nodo/core/math.hpp"
#include "nodo/sop/bevel_sop.hpp"

#include <Eigen/Core>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <unordered_map>

#include <igl/edges.h>
#include <igl/per_face_normals.h>
#include <igl/per_vertex_normals.h>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::sop {

struct EdgeKey {
  int v0, v1;
  EdgeKey(int a, int b) : v0(std::min(a, b)), v1(std::max(a, b)) {}
  bool operator<(const EdgeKey& other) const { return v0 < other.v0 || (v0 == other.v0 && v1 < other.v1); }
  bool operator==(const EdgeKey& other) const { return v0 == other.v0 && v1 == other.v1; }
};

struct EdgeInfo {
  std::vector<int> faces;
  float angle = 0.0F;
};

// Helper: Convert GeometryContainer to Eigen matrices
static bool geometry_to_eigen(const core::GeometryContainer& geom, Eigen::MatrixXd& V, Eigen::MatrixXi& F) {
  const auto* positions = geom.positions();
  if (!positions) {
    return false;
  }

  const auto& topology = geom.topology();
  const size_t num_points = geom.point_count();
  const size_t num_prims = topology.primitive_count();

  // Build vertex matrix (n x 3)
  V.resize(num_points, 3);
  for (size_t i = 0; i < num_points; ++i) {
    const auto& pos = (*positions)[i];
    V(i, 0) = pos.x();
    V(i, 1) = pos.y();
    V(i, 2) = pos.z();
  }

  // Build face matrix - count total vertices across all primitives
  size_t total_faces = 0;
  for (size_t i = 0; i < num_prims; ++i) {
    const auto& prim_verts = topology.get_primitive_vertices(i);
    if (prim_verts.size() == 3) {
      total_faces++; // Triangle
    } else if (prim_verts.size() == 4) {
      total_faces += 2; // Quad -> 2 triangles
    } else if (prim_verts.size() > 4) {
      // N-gon -> fan triangulation
      total_faces += prim_verts.size() - 2;
    }
  }

  F.resize(total_faces, 3);
  size_t face_idx = 0;

  for (size_t prim_idx = 0; prim_idx < num_prims; ++prim_idx) {
    const auto& prim_verts = topology.get_primitive_vertices(prim_idx);
    if (prim_verts.size() < 3) {
      continue;
    }

    // Get point indices from vertices
    std::vector<int> points;
    for (int vert_idx : prim_verts) {
      points.push_back(topology.get_vertex_point(vert_idx));
    }

    if (points.size() == 3) {
      // Triangle
      F(face_idx, 0) = points[0];
      F(face_idx, 1) = points[1];
      F(face_idx, 2) = points[2];
      face_idx++;
    } else if (points.size() == 4) {
      // Quad -> 2 triangles
      F(face_idx, 0) = points[0];
      F(face_idx, 1) = points[1];
      F(face_idx, 2) = points[2];
      face_idx++;

      F(face_idx, 0) = points[0];
      F(face_idx, 1) = points[2];
      F(face_idx, 2) = points[3];
      face_idx++;
    } else {
      // N-gon -> fan triangulation from first vertex
      for (size_t i = 1; i + 1 < points.size(); ++i) {
        F(face_idx, 0) = points[0];
        F(face_idx, 1) = points[i];
        F(face_idx, 2) = points[i + 1];
        face_idx++;
      }
    }
  }

  return true;
}

// Helper: Convert Eigen matrices back to GeometryContainer
static std::shared_ptr<core::GeometryContainer> eigen_to_geometry(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F) {
  auto result = std::make_shared<core::GeometryContainer>();

  const size_t num_points = V.rows();
  const size_t num_faces = F.rows();

  result->set_point_count(num_points);
  result->add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto* result_pos = result->get_point_attribute_typed<core::Vec3f>(attrs::P);

  // Copy positions
  for (size_t i = 0; i < num_points; ++i) {
    (*result_pos)[i] =
        core::Vec3f(static_cast<float>(V(i, 0)), static_cast<float>(V(i, 1)), static_cast<float>(V(i, 2)));
  }

  // Set vertex count and build primitives
  result->set_vertex_count(num_faces * 3);

  size_t vert_idx = 0;
  for (size_t face_idx = 0; face_idx < num_faces; ++face_idx) {
    std::vector<int> prim_verts;
    for (int j = 0; j < 3; ++j) {
      result->topology().set_vertex_point(vert_idx, F(face_idx, j));
      prim_verts.push_back(static_cast<int>(vert_idx));
      vert_idx++;
    }
    result->add_primitive(prim_verts);
  }

  return result;
}

BevelSOP::BevelSOP(const std::string& name) : SOPNode(name, "Bevel") {
  // Input geometry port
  input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

  // Parameters (schema)
  register_parameter(define_float_parameter("width", DEFAULT_WIDTH)
                         .label("Width")
                         .range(0.0, 1000.0)
                         .category("Bevel")
                         .description("Bevel width/offset distance")
                         .build());

  register_parameter(define_int_parameter("segments", DEFAULT_SEGMENTS)
                         .label("Segments")
                         .range(1, 8)
                         .category("Bevel")
                         .description("Number of segments in the bevel (1=simple chamfer, 2+=rounded)")
                         .build());

  register_parameter(define_float_parameter("profile", DEFAULT_PROFILE)
                         .label("Profile")
                         .range(0.0, 1.0)
                         .category("Bevel")
                         .description("Reserved for future profile control (currently unused)")
                         .build());

  register_parameter(define_int_parameter("bevel_type", static_cast<int>(BevelType::Edge))
                         .label("Mode")
                         .options({"Vertex", "Edge", "Face"})
                         .category("Bevel")
                         .description("Edge=proper edge bevel, Face=face inset, Vertex=experimental")
                         .build());

  register_parameter(define_bool_parameter("clamp_overlap", true)
                         .label("Clamp Overlap")
                         .category("Bevel")
                         .description("Reserved for future use")
                         .build());

  register_parameter(define_float_parameter("angle_limit", 30.0F)
                         .label("Angle Limit")
                         .range(0.0, 180.0)
                         .category("Limits")
                         .description("Reserved for future edge selection")
                         .build());
}

// ============================================================================
// Vertex Bevel: Creates beveled corners by offsetting vertices toward face
// centers
// ============================================================================
static std::shared_ptr<core::GeometryContainer> bevel_vertices(const core::GeometryContainer& input, float bevel_width,
                                                               int segments, float angle_threshold) {
  std::cout << "\n=== Vertex Bevel Mode ===\n";

  // Convert to Eigen for analysis
  Eigen::MatrixXd V;
  Eigen::MatrixXi F;
  if (!geometry_to_eigen(input, V, F)) {
    return nullptr;
  }

  // Compute face normals
  Eigen::MatrixXd face_normals;
  igl::per_face_normals(V, F, face_normals);

  // Build edge-to-face adjacency map
  std::map<EdgeKey, EdgeInfo> edge_map;
  for (int face_idx = 0; face_idx < F.rows(); ++face_idx) {
    for (int i = 0; i < 3; ++i) {
      int vert_a = F(face_idx, i);
      int vert_b = F(face_idx, (i + 1) % 3);
      EdgeKey edge(vert_a, vert_b);
      edge_map[edge].faces.push_back(face_idx);
    }
  }

  // Find sharp edges based on angle
  std::set<EdgeKey> sharp_edges;
  const float angle_rad = angle_threshold * static_cast<float>(core::math::PI) / 180.0F;

  for (auto& [edge, info] : edge_map) {
    if (info.faces.size() == 2) {
      const Eigen::Vector3d& normal_a = face_normals.row(info.faces[0]);
      const Eigen::Vector3d& normal_b = face_normals.row(info.faces[1]);
      double dot_product = normal_a.dot(normal_b);
      double angle = std::acos(std::clamp(dot_product, -1.0, 1.0));
      info.angle = static_cast<float>(angle);

      if (angle > angle_rad) {
        sharp_edges.insert(edge);
      }
    }
  }

  std::cout << "Found " << sharp_edges.size() << " sharp edges\n";

  // Find vertices that are part of sharp edges
  std::set<int> sharp_vertices;
  for (const auto& edge : sharp_edges) {
    sharp_vertices.insert(edge.v0);
    sharp_vertices.insert(edge.v1);
  }

  std::cout << "Found " << sharp_vertices.size() << " vertices adjacent to sharp edges\n";

  if (sharp_vertices.empty()) {
    std::cout << "No sharp vertices found, returning input unchanged\n";
    // Need to return a copy since we can't just return the input reference
    auto result = std::make_shared<core::GeometryContainer>();
    // Copy geometry data here if needed, or just return nullptr to signal no
    // change
    return nullptr; // Caller should handle nullptr as "no change needed"
  }

  // Build vertex-to-faces adjacency
  std::map<int, std::vector<int>> vertex_to_faces;
  for (int face_idx = 0; face_idx < F.rows(); ++face_idx) {
    for (int i = 0; i < 3; ++i) {
      int vert = F(face_idx, i);
      vertex_to_faces[vert].push_back(face_idx);
    }
  }

  // For each sharp vertex, create offset vertices (one per adjacent face)
  // Map: (vertex_id, face_id) -> new_vertex_id
  std::map<std::pair<int, int>, int> vertex_face_to_new_vertex;
  std::vector<Eigen::Vector3d> out_vertices;

  // Copy all original vertices first
  for (int i = 0; i < V.rows(); ++i) {
    out_vertices.push_back(V.row(i));
  }

  // Create offset vertices for sharp vertices
  for (int vert_id : sharp_vertices) {
    const auto& adjacent_faces = vertex_to_faces[vert_id];
    Eigen::Vector3d vertex_pos = V.row(vert_id);

    for (int face_idx : adjacent_faces) {
      // Calculate face center
      Eigen::Vector3d face_center = (V.row(F(face_idx, 0)) + V.row(F(face_idx, 1)) + V.row(F(face_idx, 2))) / 3.0;

      // Calculate offset direction (toward face center)
      Eigen::Vector3d offset_dir = (face_center - vertex_pos).normalized();

      // Create offset vertex
      Eigen::Vector3d offset_pos = vertex_pos + offset_dir * bevel_width;

      int new_vert_id = static_cast<int>(out_vertices.size());
      out_vertices.push_back(offset_pos);

      vertex_face_to_new_vertex[{vert_id, face_idx}] = new_vert_id;
    }
  }

  // Build output faces, replacing sharp vertices with their offset versions
  std::vector<std::vector<int>> out_faces;

  for (int face_idx = 0; face_idx < F.rows(); ++face_idx) {
    std::vector<int> face_verts;

    for (int i = 0; i < 3; ++i) {
      int vert_id = F(face_idx, i);

      // Check if this vertex is sharp
      if (sharp_vertices.find(vert_id) != sharp_vertices.end()) {
        // Use the offset vertex for this face
        int new_vert = vertex_face_to_new_vertex[{vert_id, face_idx}];
        face_verts.push_back(new_vert);
      } else {
        // Use original vertex
        face_verts.push_back(vert_id);
      }
    }

    out_faces.push_back(face_verts);
  }

  // DON'T create cap faces for now - they're causing the bad geometry
  // The beveled corners will be open, which is better than broken geometry
  // TODO: Implement proper cap face creation with correct vertex ordering

  std::cout << "Output: " << out_vertices.size() << " vertices, " << out_faces.size() << " faces\n";

  // Convert back to GeometryContainer
  auto result = std::make_shared<core::GeometryContainer>();
  result->set_point_count(out_vertices.size());
  result->add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto* result_pos = result->get_point_attribute_typed<core::Vec3f>(attrs::P);

  for (size_t i = 0; i < out_vertices.size(); ++i) {
    (*result_pos)[i] = core::Vec3f(static_cast<float>(out_vertices[i].x()), static_cast<float>(out_vertices[i].y()),
                                   static_cast<float>(out_vertices[i].z()));
  }

  // Build topology
  size_t total_verts = 0;
  for (const auto& face : out_faces) {
    total_verts += face.size();
  }
  result->set_vertex_count(total_verts);

  size_t vert_idx = 0;
  for (const auto& face : out_faces) {
    std::vector<int> prim_verts;
    for (int point_idx : face) {
      result->topology().set_vertex_point(vert_idx, point_idx);
      prim_verts.push_back(static_cast<int>(vert_idx));
      vert_idx++;
    }
    result->add_primitive(prim_verts);
  }

  return result;
}

// ============================================================================
// Edge Bevel: Creates beveled edges (Cylinders connecting scaled faces)
// Based on: https://stackoverflow.com/questions/3484497/
// ============================================================================
static std::shared_ptr<core::GeometryContainer> bevel_edges(const core::GeometryContainer& input, float bevel_width,
                                                            int segments, float angle_threshold) {
  std::cout << "\n=== Edge Bevel Mode (Cylinder Strips) ===\n";

  // Convert to Eigen
  Eigen::MatrixXd V;
  Eigen::MatrixXi F;
  if (!geometry_to_eigen(input, V, F)) {
    return nullptr;
  }

  // Compute face normals
  Eigen::MatrixXd face_normals;
  igl::per_face_normals(V, F, face_normals);

  // Build edge-to-face map
  std::map<EdgeKey, EdgeInfo> edge_map;
  for (int face_idx = 0; face_idx < F.rows(); ++face_idx) {
    for (int i = 0; i < 3; ++i) {
      int vert_a = F(face_idx, i);
      int vert_b = F(face_idx, (i + 1) % 3);
      EdgeKey edge(vert_a, vert_b);
      edge_map[edge].faces.push_back(face_idx);
    }
  }

  // Find sharp edges
  std::set<EdgeKey> sharp_edges;
  const float angle_rad = angle_threshold * static_cast<float>(core::math::PI) / 180.0F;

  std::cout << "Checking edges for angle threshold: " << angle_threshold << " degrees (" << angle_rad << " radians)\n";
  std::cout << "Total unique edges: " << edge_map.size() << "\n";

  for (auto& [edge, info] : edge_map) {
    if (info.faces.size() == 2) {
      const Eigen::Vector3d& normal_a = face_normals.row(info.faces[0]);
      const Eigen::Vector3d& normal_b = face_normals.row(info.faces[1]);
      double dot_product = normal_a.dot(normal_b);
      double angle = std::acos(std::clamp(dot_product, -1.0, 1.0));
      double angle_degrees = angle * 180.0 / core::math::PI;

      // Debug first few edges
      if (sharp_edges.size() < 3) {
        std::cout << "Edge (" << edge.v0 << "," << edge.v1 << "): angle = " << angle_degrees
                  << " degrees (dot=" << dot_product << ")\n";
      }

      if (angle > angle_rad) {
        sharp_edges.insert(edge);
      }
    }
  }

  std::cout << "Found " << sharp_edges.size() << " sharp edges\n";

  if (sharp_edges.empty()) {
    std::cout << "WARNING: No sharp edges found with angle > " << angle_threshold
              << " degrees. Try lowering the angle limit parameter.\n";
    return nullptr;
  }

  // Step 1: Copy original vertices
  std::vector<Eigen::Vector3d> out_vertices;
  for (int i = 0; i < V.rows(); ++i) {
    out_vertices.push_back(V.row(i));
  }
  int orig_vert_count = V.rows();

  // Step 2: For each face adjacent to sharp edges, create inset vertices
  // For vertices on sharp edges, offset along the edge bisector
  // For vertices not on sharp edges, keep original position

  std::map<std::pair<int, int>, int> face_vert_to_inset;

  // Find all faces adjacent to sharp edges
  std::set<int> faces_with_sharp_edges;
  for (const EdgeKey& edge : sharp_edges) {
    const EdgeInfo& info = edge_map[edge];
    for (int face_idx : info.faces) {
      faces_with_sharp_edges.insert(face_idx);
    }
  }

  std::cout << "Found " << faces_with_sharp_edges.size() << " faces adjacent to sharp edges\n";

  // For each face with sharp edges, create inset vertices
  for (int face_idx : faces_with_sharp_edges) {
    Eigen::Vector3d face_normal = face_normals.row(face_idx);

    for (int i = 0; i < 3; ++i) {
      int vert_idx = F(face_idx, i);
      Eigen::Vector3d vert_pos = V.row(vert_idx);

      // Offset vertex inward along face normal
      Eigen::Vector3d inset_pos = vert_pos - face_normal * bevel_width;

      int new_vert_idx = static_cast<int>(out_vertices.size());
      out_vertices.push_back(inset_pos);
      face_vert_to_inset[{face_idx, i}] = new_vert_idx;
    }
  }

  std::cout << "Created " << face_vert_to_inset.size() << " inset vertices\n";

  std::vector<std::vector<int>> out_faces;

  // Step 3: Add faces - inset versions for faces with sharp edges, original for
  // others
  for (int face_idx = 0; face_idx < F.rows(); ++face_idx) {
    bool has_sharp_edge = (faces_with_sharp_edges.find(face_idx) != faces_with_sharp_edges.end());

    std::vector<int> face_verts;
    for (int i = 0; i < 3; ++i) {
      if (has_sharp_edge) {
        // Use inset vertex
        face_verts.push_back(face_vert_to_inset[{face_idx, i}]);
      } else {
        // Use original vertex
        face_verts.push_back(F(face_idx, i));
      }
    }
    out_faces.push_back(face_verts);
  }

  std::cout << "Added " << F.rows() << " faces (" << faces_with_sharp_edges.size() << " with insets)\n";

  // Step 4: Create bevel strips for sharp edges
  std::cout << "Creating bevel strips for " << sharp_edges.size() << " sharp edges...\n";
  int bridge_count = 0;

  for (const EdgeKey& edge : sharp_edges) {
    const EdgeInfo& info = edge_map[edge];
    if (info.faces.size() != 2) {
      continue;
    }

    int face0 = info.faces[0];
    int face1 = info.faces[1];

    // Find which local indices in each face correspond to this edge
    int f0_idx0 = -1, f0_idx1 = -1;
    for (int i = 0; i < 3; ++i) {
      int curr = F(face0, i);
      int next = F(face0, (i + 1) % 3);
      EdgeKey face_edge(curr, next);
      if (face_edge == edge) {
        f0_idx0 = i;
        f0_idx1 = (i + 1) % 3;
        break;
      }
    }

    int f1_idx0 = -1, f1_idx1 = -1;
    for (int i = 0; i < 3; ++i) {
      int curr = F(face1, i);
      int next = F(face1, (i + 1) % 3);
      EdgeKey face_edge(curr, next);
      if (face_edge == edge) {
        f1_idx0 = i;
        f1_idx1 = (i + 1) % 3;
        break;
      }
    }

    if (f0_idx0 == -1 || f1_idx0 == -1) {
      continue;
    }

    // Get inset vertex indices from both faces
    int inset_f0_v0 = face_vert_to_inset[{face0, f0_idx0}];
    int inset_f0_v1 = face_vert_to_inset[{face0, f0_idx1}];
    int inset_f1_v0 = face_vert_to_inset[{face1, f1_idx0}];
    int inset_f1_v1 = face_vert_to_inset[{face1, f1_idx1}];

    // Check edge direction
    int orig_v0 = F(face0, f0_idx0);
    int orig_v1 = F(face0, f0_idx1);
    int f1_orig_v0 = F(face1, f1_idx0);

    bool same_direction = (orig_v0 == f1_orig_v0);

    if (same_direction) {
      // Create bridge quad connecting the two inset edges
      out_faces.push_back({inset_f0_v0, inset_f0_v1, inset_f1_v0});
      out_faces.push_back({inset_f0_v1, inset_f1_v1, inset_f1_v0});
    } else {
      // Opposite direction
      out_faces.push_back({inset_f0_v0, inset_f0_v1, inset_f1_v1});
      out_faces.push_back({inset_f0_v0, inset_f1_v1, inset_f1_v0});
    }
    bridge_count++;
  }

  std::cout << "Created " << bridge_count << " bevel strips (2 triangles each)\n";
  std::cout << "Total output: " << out_vertices.size() << " vertices, " << out_faces.size() << " faces\n";

  std::cout << "Output: " << out_vertices.size() << " vertices, " << out_faces.size() << " faces\n";

  // Convert back to GeometryContainer
  auto result = std::make_shared<core::GeometryContainer>();
  result->set_point_count(out_vertices.size());
  result->add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto* result_pos = result->get_point_attribute_typed<core::Vec3f>(attrs::P);

  for (size_t i = 0; i < out_vertices.size(); ++i) {
    (*result_pos)[i] = core::Vec3f(static_cast<float>(out_vertices[i].x()), static_cast<float>(out_vertices[i].y()),
                                   static_cast<float>(out_vertices[i].z()));
  }

  // Build topology
  size_t total_verts = 0;
  for (const auto& face : out_faces) {
    total_verts += face.size();
  }
  result->set_vertex_count(total_verts);

  size_t vert_idx = 0;
  for (const auto& face : out_faces) {
    std::vector<int> prim_verts;
    for (int point_idx : face) {
      result->topology().set_vertex_point(vert_idx, point_idx);
      prim_verts.push_back(static_cast<int>(vert_idx));
      vert_idx++;
    }
    result->add_primitive(prim_verts);
  }

  return result;
}

// ============================================================================
// Face Bevel/Inset: Insets faces toward their centers
// This creates beveled borders around faces
// ============================================================================
static std::shared_ptr<core::GeometryContainer> bevel_faces(const core::GeometryContainer& input, float bevel_width,
                                                            int segments, float angle_threshold) {
  std::cout << "\n=== Face Inset/Bevel Mode ===\n";

  // Convert to Eigen for processing
  Eigen::MatrixXd V;
  Eigen::MatrixXi F;
  if (!geometry_to_eigen(input, V, F)) {
    return nullptr;
  }

  // Compute face normals to determine which faces to bevel
  Eigen::MatrixXd face_normals;
  igl::per_face_normals(V, F, face_normals);

  // Build edge-to-face map
  std::map<EdgeKey, EdgeInfo> edge_map;
  for (int face_idx = 0; face_idx < F.rows(); ++face_idx) {
    for (int i = 0; i < 3; ++i) {
      int vert_a = F(face_idx, i);
      int vert_b = F(face_idx, (i + 1) % 3);
      EdgeKey edge(vert_a, vert_b);
      edge_map[edge].faces.push_back(face_idx);
    }
  }

  // Find faces adjacent to sharp edges
  std::set<int> faces_to_bevel;
  const float angle_rad = angle_threshold * static_cast<float>(core::math::PI) / 180.0F;

  for (auto& [edge, info] : edge_map) {
    if (info.faces.size() == 2) {
      const Eigen::Vector3d& normal_a = face_normals.row(info.faces[0]);
      const Eigen::Vector3d& normal_b = face_normals.row(info.faces[1]);
      double dot_product = normal_a.dot(normal_b);
      double angle = std::acos(std::clamp(dot_product, -1.0, 1.0));

      if (angle > angle_rad) {
        // Both faces adjacent to this sharp edge should be beveled
        faces_to_bevel.insert(info.faces[0]);
        faces_to_bevel.insert(info.faces[1]);
      }
    }
  }

  std::cout << "Found " << faces_to_bevel.size() << " faces adjacent to sharp edges\n";

  std::vector<Eigen::Vector3d> out_vertices;
  std::vector<std::vector<int>> out_faces;

  // Copy original vertices
  for (int i = 0; i < V.rows(); ++i) {
    out_vertices.push_back(V.row(i));
  }

  // Process each face
  for (int face_idx = 0; face_idx < F.rows(); ++face_idx) {
    // Get face vertices
    std::vector<int> face_verts;
    for (int i = 0; i < 3; ++i) {
      face_verts.push_back(F(face_idx, i));
    }

    bool should_bevel = faces_to_bevel.find(face_idx) != faces_to_bevel.end();

    if (!should_bevel) {
      // Keep original face unchanged
      out_faces.push_back(face_verts);
      continue;
    }

    // Calculate face center
    Eigen::Vector3d center(0, 0, 0);
    for (int vert_id : face_verts) {
      center += V.row(vert_id);
    }
    center /= static_cast<double>(face_verts.size());

    // Create inset vertices
    std::vector<int> inset_verts;
    for (int orig_vert : face_verts) {
      Eigen::Vector3d orig_pos = V.row(orig_vert);
      Eigen::Vector3d to_center = center - orig_pos;
      Eigen::Vector3d inset_pos = orig_pos + to_center * bevel_width;

      int new_vert_id = static_cast<int>(out_vertices.size());
      out_vertices.push_back(inset_pos);
      inset_verts.push_back(new_vert_id);
    }

    // Create inset face
    out_faces.push_back(inset_verts);

    // Create bridge quads for each edge
    for (size_t i = 0; i < face_verts.size(); ++i) {
      size_t next_i = (i + 1) % face_verts.size();

      int orig_v0 = face_verts[i];
      int orig_v1 = face_verts[next_i];
      int inset_v0 = inset_verts[i];
      int inset_v1 = inset_verts[next_i];

      // Create quad as two triangles
      // Triangle 1: orig_v0, orig_v1, inset_v1
      out_faces.push_back({orig_v0, orig_v1, inset_v1});

      // Triangle 2: orig_v0, inset_v1, inset_v0
      out_faces.push_back({orig_v0, inset_v1, inset_v0});
    }
  }

  std::cout << "Output: " << out_vertices.size() << " vertices, " << out_faces.size() << " faces\n";

  // Convert back to GeometryContainer
  auto result = std::make_shared<core::GeometryContainer>();
  result->set_point_count(out_vertices.size());
  result->add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto* result_pos = result->get_point_attribute_typed<core::Vec3f>(attrs::P);

  for (size_t i = 0; i < out_vertices.size(); ++i) {
    (*result_pos)[i] = core::Vec3f(static_cast<float>(out_vertices[i].x()), static_cast<float>(out_vertices[i].y()),
                                   static_cast<float>(out_vertices[i].z()));
  }

  // Build topology
  size_t total_verts = 0;
  for (const auto& face : out_faces) {
    total_verts += face.size();
  }
  result->set_vertex_count(total_verts);

  size_t vert_idx = 0;
  for (const auto& face : out_faces) {
    std::vector<int> prim_verts;
    for (int point_idx : face) {
      result->topology().set_vertex_point(vert_idx, point_idx);
      prim_verts.push_back(static_cast<int>(vert_idx));
      vert_idx++;
    }
    result->add_primitive(prim_verts);
  }

  return result;
}

// ============================================================================
// Main Execute Function
// ============================================================================
core::Result<std::shared_ptr<core::GeometryContainer>> BevelSOP::execute() {
  auto input = get_input_data(0);
  if (!input) {
    return {"No input geometry"};
  }

  const float bevel_width = get_parameter<float>("width");
  const int segments = get_parameter<int>("segments");
  const float angle_threshold = get_parameter<float>("angle_limit");
  const int bevel_type_int = get_parameter<int>("bevel_type");
  const BevelType bevel_type = static_cast<BevelType>(bevel_type_int);

  std::cout << "\n=== Bevel SOP ===\n";
  std::cout << "Input: " << input->point_count() << " points, " << input->topology().primitive_count()
            << " primitives\n";
  std::cout << "Bevel width: " << bevel_width << ", segments: " << segments << ", angle threshold: " << angle_threshold
            << "°\n";
  std::cout << "Mode: " << bevel_type_int << " (0=Vertex, 1=Edge, 2=Face)\n";

  // Dispatch to appropriate bevel function
  std::shared_ptr<core::GeometryContainer> result;

  switch (bevel_type) {
    case BevelType::Vertex:
      result = bevel_vertices(*input, bevel_width, segments, angle_threshold);
      break;

    case BevelType::Edge:
      result = bevel_edges(*input, bevel_width, segments, angle_threshold);
      break;

    case BevelType::Face:
      result = bevel_faces(*input, bevel_width, segments, angle_threshold);
      break;

    default:
      return {"Invalid bevel mode"};
  }

  if (!result) {
    // nullptr means no change needed, return input
    return input;
  }

  std::cout << "Result: " << result->point_count() << " points, " << result->topology().primitive_count()
            << " primitives\n";
  std::cout << "=================\n\n";

  return result;
}

} // namespace nodo::sop
