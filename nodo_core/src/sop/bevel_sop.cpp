#include "nodo/sop/bevel_sop.hpp"
#include "nodo/core/attribute_types.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <vector>

namespace attrs = nodo::core::standard_attrs;
namespace core = nodo::core; // Alias to allow core:: to refer to nodo::core

namespace nodo::sop {

// Constants
// EPS_NORMAL & EPS_TINY reserved for future advanced bevel intersection math.
// Commented out to silence unused warnings until implemented.
// static constexpr float EPS_NORMAL = 1e-6F;
// static constexpr float EPS_TINY = 1e-9F;
static constexpr float EPS_VERY_TINY = 1e-12F;
static constexpr float DEG_PER_RAD = 180.0F / static_cast<float>(M_PI);
static constexpr double MAX_WIDTH_RANGE = 1000.0;
static constexpr int MAX_SEGMENTS_RANGE = 64;
static constexpr float DEFAULT_ANGLE_LIMIT = 30.0F;
static constexpr float MAX_ANGLE_LIMIT = 180.0F;
static constexpr float PROFILE_MID = 0.5F;
static constexpr float GAMMA_MIN = 0.1F;
static constexpr float GAMMA_MAX = 4.0F;
static constexpr float HALF = 0.5F;
static constexpr float CLAMP_EPS = 1e-6F;

struct EdgeKey {
  int v0, v1;
  EdgeKey(int point_a, int point_b)
      : v0(std::min(point_a, point_b)), v1(std::max(point_a, point_b)) {}
  bool operator<(const EdgeKey &other) const {
    return v0 < other.v0 || (v0 == other.v0 && v1 < other.v1);
  }
  bool operator==(const EdgeKey &other) const {
    return v0 == other.v0 && v1 == other.v1;
  }
};

struct EdgeInfo {
  std::vector<int> faces;
  float angle = 0.0F;
};

//------------------------------------------------------------------------------
// Small helpers to keep bevel_edges complexity low
//------------------------------------------------------------------------------
// Forward declare profile shaping used by strip builder
static inline float shape_profile_t(float t_value, float profile);

// Safely append a batch of points and refresh the positions pointer
static size_t append_points(core::GeometryContainer &geo,
                            core::AttributeStorage<Eigen::Vector3f> *&positions,
                            const std::vector<Eigen::Vector3f> &new_points) {
  size_t base_point = geo.point_count();
  geo.set_point_count(base_point + new_points.size());
  // Reacquire pointer after potential reallocation
  positions = geo.get_point_attribute_typed<Eigen::Vector3f>(attrs::P);
  for (size_t idx = 0; idx < new_points.size(); ++idx) {
    (*positions)[base_point + idx] = new_points[idx];
  }
  return base_point;
}

// Build a multi-segment quad strip for a bevel edge
static void add_chamfer_strip_for_edge(
    core::GeometryContainer &geo,
    core::AttributeStorage<Eigen::Vector3f> *&result_positions,
    const core::AttributeStorage<Eigen::Vector3f> *input_positions,
    const std::vector<Eigen::Vector3f> &prim_normals, const EdgeKey &ekey,
    const EdgeInfo &einfo, int segments, float bevel_width, float profile,
    bool clamp_overlap) {
  int segs = std::max(1, segments);

  // Source endpoints from immutable input
  int point_index0 = ekey.v0;
  int point_index1 = ekey.v1;
  const Eigen::Vector3f &src_pos_point0 = (*input_positions)[point_index0];
  const Eigen::Vector3f &src_pos_point1 = (*input_positions)[point_index1];
  const Eigen::Vector3f &normal0 = prim_normals[einfo.faces[0]];
  const Eigen::Vector3f &normal1 = prim_normals[einfo.faces[1]];

  // Width clamp per-edge
  float edge_len = (src_pos_point1 - src_pos_point0).norm();
  float width_for_edge = bevel_width;
  if (clamp_overlap) {
    width_for_edge = std::min(width_for_edge, (HALF * edge_len) - CLAMP_EPS);
    width_for_edge = std::max(width_for_edge, 0.0F);
  }

  // Prepare points for the whole strip: 2 per section, (segs+1) sections
  std::vector<Eigen::Vector3f> strip_points;
  strip_points.resize(static_cast<size_t>(2 * (segs + 1)));
  for (int seg_index = 0; seg_index <= segs; ++seg_index) {
    float t_raw = static_cast<float>(seg_index) / static_cast<float>(segs);
    float t_blend = shape_profile_t(t_raw, profile);
    Eigen::Vector3f blended = ((1.0F - t_blend) * normal0 + t_blend * normal1);
    float len = blended.norm();
    if (len < EPS_VERY_TINY) {
      blended = normal0;
    } else {
      blended /= len;
    }
    Eigen::Vector3f p0_s = src_pos_point0 + width_for_edge * blended;
    Eigen::Vector3f p1_s = src_pos_point1 + width_for_edge * blended;
    strip_points[static_cast<size_t>((2 * seg_index) + 0)] = p0_s;
    strip_points[static_cast<size_t>((2 * seg_index) + 1)] = p1_s;
  }

  size_t base_point = append_points(geo, result_positions, strip_points);

  // Create vertices and quads per segment
  size_t base_vertex = geo.vertex_count();
  geo.set_vertex_count(base_vertex + static_cast<size_t>(4 * segs));
  auto &topo = geo.topology();
  for (int seg_index = 0; seg_index < segs; ++seg_index) {
    size_t vertex_index0 =
        base_vertex + static_cast<size_t>((4 * seg_index) + 0);
    size_t vertex_index1 =
        base_vertex + static_cast<size_t>((4 * seg_index) + 1);
    size_t vertex_index2 =
        base_vertex + static_cast<size_t>((4 * seg_index) + 2);
    size_t vertex_index3 =
        base_vertex + static_cast<size_t>((4 * seg_index) + 3);

    size_t point_idx_0s = base_point + static_cast<size_t>((2 * seg_index) + 0);
    size_t point_idx_1s = base_point + static_cast<size_t>((2 * seg_index) + 1);
    size_t point_idx_0s1 =
        base_point + static_cast<size_t>((2 * (seg_index + 1)) + 0);
    size_t point_idx_1s1 =
        base_point + static_cast<size_t>((2 * (seg_index + 1)) + 1);

    topo.set_vertex_point(vertex_index0, static_cast<int>(point_idx_0s));
    topo.set_vertex_point(vertex_index1, static_cast<int>(point_idx_1s));
    topo.set_vertex_point(vertex_index2, static_cast<int>(point_idx_1s1));
    topo.set_vertex_point(vertex_index3, static_cast<int>(point_idx_0s1));

    std::vector<int> quad = {
        static_cast<int>(vertex_index0), static_cast<int>(vertex_index1),
        static_cast<int>(vertex_index2), static_cast<int>(vertex_index3)};
    geo.add_primitive(quad);
  }
}
// Create a single chamfer quad for an edge between two faces by offsetting the
// edge endpoints along each face normal by bevel_width. This is a simple
// scaffold (not the final intersection-accurate bevel).
static void
add_chamfer_quad_for_edge(core::GeometryContainer &geo,
                          core::AttributeStorage<Eigen::Vector3f> *positions,
                          const std::vector<Eigen::Vector3f> &prim_normals,
                          const EdgeKey &edge_key, const EdgeInfo &info,
                          float bevel_width) {
  if (positions == nullptr)
    return;
  if (info.faces.size() != 2)
    return;

  int point_index0 = edge_key.v0;
  int point_index1 = edge_key.v1;
  const Eigen::Vector3f &pos_point0 = (*positions)[point_index0];
  const Eigen::Vector3f &pos_point1 = (*positions)[point_index1];

  const Eigen::Vector3f &normal0 = prim_normals[info.faces[0]];
  const Eigen::Vector3f &normal1 = prim_normals[info.faces[1]];

  // Offset points along each adjacent face normal.
  Eigen::Vector3f p0_a = pos_point0 + bevel_width * normal0;
  Eigen::Vector3f p1_a = pos_point1 + bevel_width * normal0;
  Eigen::Vector3f p1_b = pos_point1 + bevel_width * normal1;
  Eigen::Vector3f p0_b = pos_point0 + bevel_width * normal1;

  // Append four new points.
  size_t base_point = geo.point_count();
  geo.set_point_count(base_point + 4);
  // Reacquire positions after potential reallocation
  positions = geo.get_point_attribute_typed<Eigen::Vector3f>(attrs::P);
  (*positions)[base_point + 0] = p0_a;
  (*positions)[base_point + 1] = p1_a;
  (*positions)[base_point + 2] = p1_b;
  (*positions)[base_point + 3] = p0_b;

  // Create four vertices and a quad primitive referencing those vertices.
  size_t base_vertex = geo.vertex_count();
  geo.set_vertex_count(base_vertex + 4);

  // ElementTopology expects add_primitive with vertex indices. We need to map
  // new vertices to new points. Topology::add_primitive will append vertices
  // internally; but here we already resized vertices to keep attributes sized.
  // We'll request a quad with four new vertices logically (indices are created
  // inside topology); afterwards, we must assign vertex->point mapping. That
  // mapping is handled by ElementTopology when we add primitives with explicit
  // vertex indices it returns. Given the current API, the simplest safe path is
  // to create the primitive using point indices via temporary vertices:

  // Build 4 new vertices that reference the 4 new points
  std::vector<int> new_vertices(4);
  for (int i = 0; i < 4; ++i) {
    int vertex_index = static_cast<int>(base_vertex + i);
    new_vertices[i] = vertex_index;
  }

  // Register the primitive with these vertex indices
  geo.add_primitive(new_vertices);

  // Wire vertex -> point mapping in topology
  auto &topo = geo.topology();
  topo.set_vertex_point(static_cast<int>(base_vertex + 0),
                        static_cast<int>(base_point + 0));
  topo.set_vertex_point(static_cast<int>(base_vertex + 1),
                        static_cast<int>(base_point + 1));
  topo.set_vertex_point(static_cast<int>(base_vertex + 2),
                        static_cast<int>(base_point + 2));
  topo.set_vertex_point(static_cast<int>(base_vertex + 3),
                        static_cast<int>(base_point + 3));
}

// Recompute primitive normals (primitive_N) using Newell's method
static void recompute_primitive_normals(core::GeometryContainer &geo) {
  geo.ensure_position_attribute();
  auto *positions = geo.get_point_attribute_typed<Eigen::Vector3f>(attrs::P);
  if (positions == nullptr)
    return;

  // Ensure primitive normal attribute exists
  if (!geo.has_primitive_attribute(attrs::primitive_N)) {
    geo.add_primitive_attribute(attrs::primitive_N, core::AttributeType::VEC3F);
  }
  auto *primN =
      geo.get_primitive_attribute_typed<Eigen::Vector3f>(attrs::primitive_N);
  if (primN == nullptr)
    return;

  primN->resize(geo.primitive_count());

  const auto &topo = geo.topology();
  for (size_t prim = 0; prim < topo.primitive_count(); ++prim) {
    const auto &verts = topo.get_primitive_vertices(prim);
    if (verts.size() < 3) {
      (*primN)[prim] = Eigen::Vector3f(0, 0, 0);
      continue;
    }
    Eigen::Vector3f accum(0, 0, 0);
    for (size_t i = 0; i < verts.size(); ++i) {
      int vertex_index0 = verts[i];
      int vertex_index1 = verts[(i + 1) % verts.size()];
      int point_index0 = topo.get_vertex_point(vertex_index0);
      int point_index1 = topo.get_vertex_point(vertex_index1);
      const auto &posA = (*positions)[point_index0];
      const auto &posB = (*positions)[point_index1];
      accum.x() += (posA.y() - posB.y()) * (posA.z() + posB.z());
      accum.y() += (posA.z() - posB.z()) * (posA.x() + posB.x());
      accum.z() += (posA.x() - posB.x()) * (posA.y() + posB.y());
    }
    float len = accum.norm();
    if (len < EPS_VERY_TINY) {
      (*primN)[prim] = Eigen::Vector3f(0, 0, 0);
    } else {
      (*primN)[prim] = accum / len;
    }
  }
}
static void compute_prim_normals_centroids(
    const core::GeometryContainer &input,
    const core::AttributeStorage<Eigen::Vector3f> *positions_attr,
    std::vector<Eigen::Vector3f> &out_normals,
    std::vector<Eigen::Vector3f> &out_centroids) {
  auto newell_normal = [&](size_t prim_idx) -> Eigen::Vector3f {
    const auto &verts = input.topology().get_primitive_vertices(prim_idx);
    size_t vertex_count = verts.size();
    if (vertex_count < 3)
      return Eigen::Vector3f(0, 0, 0);
    Eigen::Vector3f accum(0, 0, 0);
    for (size_t i = 0; i < vertex_count; ++i) {
      int v_idx0 = verts[i];
      int v_idx1 = verts[(i + 1) % vertex_count];
      int p_idx0 = input.topology().get_vertex_point(v_idx0);
      int p_idx1 = input.topology().get_vertex_point(v_idx1);
      const Eigen::Vector3f &pos_a = (*positions_attr)[p_idx0];
      const Eigen::Vector3f &pos_b = (*positions_attr)[p_idx1];
      accum.x() += (pos_a.y() - pos_b.y()) * (pos_a.z() + pos_b.z());
      accum.y() += (pos_a.z() - pos_b.z()) * (pos_a.x() + pos_b.x());
      accum.z() += (pos_a.x() - pos_b.x()) * (pos_a.y() + pos_b.y());
    }
    float len_norm = accum.norm();
    if (len_norm < EPS_VERY_TINY)
      return Eigen::Vector3f(0, 0, 0);
    return accum / len_norm;
  };

  auto compute_primitive_centroid = [&](size_t prim_idx) -> Eigen::Vector3f {
    const auto &verts = input.topology().get_primitive_vertices(prim_idx);
    Eigen::Vector3f centroid(0, 0, 0);
    if (verts.empty())
      return centroid;
    for (int vert_index : verts) {
      int point_index = input.topology().get_vertex_point(vert_index);
      centroid += (*positions_attr)[point_index];
    }
    centroid /= static_cast<float>(verts.size());
    return centroid;
  };

  size_t prim_count = input.primitive_count();
  out_normals.resize(prim_count);
  out_centroids.resize(prim_count);
  for (size_t prim = 0; prim < prim_count; ++prim) {
    out_normals[prim] = newell_normal(prim);
    out_centroids[prim] = compute_primitive_centroid(prim);
  }
}

static void build_edge_map(const core::GeometryContainer &input,
                           std::map<EdgeKey, EdgeInfo> &edge_map) {
  for (size_t prim = 0; prim < input.primitive_count(); ++prim) {
    const auto &verts = input.topology().get_primitive_vertices(prim);
    size_t vcount = verts.size();
    for (size_t i = 0; i < vcount; ++i) {
      int v_idx0 = verts[i];
      int v_idx1 = verts[(i + 1) % vcount];
      int p_idx0 = input.topology().get_vertex_point(v_idx0);
      int p_idx1 = input.topology().get_vertex_point(v_idx1);
      EdgeKey key(p_idx0, p_idx1);
      auto &info = edge_map[key];
      info.faces.push_back(static_cast<int>(prim));
    }
  }
}

static std::vector<EdgeKey>
find_sharp_edges(const std::map<EdgeKey, EdgeInfo> &edge_map,
                 const std::vector<Eigen::Vector3f> &prim_normals,
                 float angle_threshold) {
  std::vector<EdgeKey> sharp_edges;
  for (const auto &kv_pair : edge_map) {
    const EdgeKey &key = kv_pair.first;
    const EdgeInfo &info = kv_pair.second;
    if (info.faces.size() == 2) {
      const Eigen::Vector3f &n_face0 = prim_normals[info.faces[0]];
      const Eigen::Vector3f &n_face1 = prim_normals[info.faces[1]];
      float dotv = std::clamp(n_face0.dot(n_face1), -1.0F, 1.0F);
      float angle_deg = std::acos(dotv) * DEG_PER_RAD;
      if (angle_deg >= angle_threshold) {
        sharp_edges.push_back(key);
      }
    }
  }
  return sharp_edges;
}

static inline float shape_profile_t(float t_value, float profile) {
  // Map profile [0,1] to an exponent gamma around 1.0 at 0.5
  // profile=0 -> softer (gamma~0.5), profile=1 -> sharper (gamma~1.5)
  float gamma = (profile <= PROFILE_MID) ? (PROFILE_MID + profile)
                                         : (profile + PROFILE_MID);
  gamma = std::clamp(gamma, GAMMA_MIN, GAMMA_MAX);
  float t_clamped = std::clamp(t_value, 0.0F, 1.0F);
  return std::pow(t_clamped, gamma);
}

static std::shared_ptr<core::GeometryContainer>
bevel_edges(const core::GeometryContainer &input, float bevel_width,
            int segments, float angle_threshold, float profile,
            bool clamp_overlap) {
  std::cout << "\n=== Edge Bevel Mode ===\n";

  // Early exits for no-op configurations
  if (bevel_width <= 0.0F || segments < 1) {
    return std::make_shared<core::GeometryContainer>(input.clone());
  }

  if (!input.has_point_attribute(attrs::P)) {
    std::cerr << "Edge Bevel: Missing position attribute P\n";
    return nullptr;
  }
  const auto *positions_attr =
      input.get_point_attribute_typed<Eigen::Vector3f>(attrs::P);
  if (positions_attr == nullptr) {
    std::cerr << "Edge Bevel: Position attribute retrieval failed\n";
    return nullptr;
  }

  auto result = std::make_shared<core::GeometryContainer>(input.clone());
  result->ensure_position_attribute();
  auto *result_positions =
      result->get_point_attribute_typed<Eigen::Vector3f>(attrs::P);
  if (result_positions == nullptr) {
    std::cerr << "Edge Bevel: Failed to access result positions\n";
    return result;
  }

  // 1) Normals & centroids
  std::vector<Eigen::Vector3f> prim_normals;
  std::vector<Eigen::Vector3f> prim_centroids;
  compute_prim_normals_centroids(input, positions_attr, prim_normals,
                                 prim_centroids);

  // 2) Edge map
  std::map<EdgeKey, EdgeInfo> edge_map;
  build_edge_map(input, edge_map);

  // 3) Sharp edges
  auto sharp_edges = find_sharp_edges(edge_map, prim_normals, angle_threshold);
  if (sharp_edges.empty()) {
    std::cout << "Edge Bevel: No sharp edges above threshold ("
              << angle_threshold << "°)\n";
    return std::make_shared<core::GeometryContainer>(input.clone());
  }
  std::cout << "Edge Bevel: Sharp edges detected: " << sharp_edges.size()
            << "\n";

  // 4) Create chamfer quads/strips
  for (const auto &edge_key : sharp_edges) {
    const EdgeInfo &info = edge_map[edge_key];
    if (info.faces.size() != 2)
      continue;

    int segs = std::max(1, segments);
    if (segs == 1) {
      // Clamp width by half-edge length if requested (avoids self-overlap)
      const Eigen::Vector3f &srcA = (*positions_attr)[edge_key.v0];
      const Eigen::Vector3f &srcB = (*positions_attr)[edge_key.v1];
      float edge_len = (srcB - srcA).norm();
      float width_for_edge = bevel_width;
      if (clamp_overlap) {
        width_for_edge =
            std::min(width_for_edge, (HALF * edge_len) - CLAMP_EPS);
        width_for_edge = std::max(width_for_edge, 0.0F);
      }
      add_chamfer_quad_for_edge(*result, result_positions, prim_normals,
                                edge_key, info, width_for_edge);
    } else {
      add_chamfer_strip_for_edge(*result, result_positions, positions_attr,
                                 prim_normals, edge_key, info, segs,
                                 bevel_width, profile, clamp_overlap);
    }
  }

  // Update normals
  recompute_primitive_normals(*result);
  return result;
}

static std::shared_ptr<core::GeometryContainer>
bevel_vertices(const core::GeometryContainer &input, float bevel_width,
               int /*segments*/, float angle_threshold) {
  std::cout << "\n=== Vertex Bevel Mode (prototype) ===\n";

  if (bevel_width <= 0.0F) {
    return std::make_shared<core::GeometryContainer>(input.clone());
  }
  if (!input.has_point_attribute(attrs::P)) {
    std::cerr << "Vertex Bevel: Missing position attribute P\n";
    return nullptr;
  }
  const auto *positions_attr =
      input.get_point_attribute_typed<Eigen::Vector3f>(attrs::P);
  if (positions_attr == nullptr) {
    std::cerr << "Vertex Bevel: Position attribute retrieval failed\n";
    return nullptr;
  }

  // Prepare result geometry
  auto result = std::make_shared<core::GeometryContainer>(input.clone());
  result->ensure_position_attribute();
  auto *result_positions =
      result->get_point_attribute_typed<Eigen::Vector3f>(attrs::P);
  if (result_positions == nullptr) {
    std::cerr << "Vertex Bevel: Failed to access result positions\n";
    return result;
  }

  // Compute primitive normals up-front
  std::vector<Eigen::Vector3f> prim_normals;
  std::vector<Eigen::Vector3f> prim_centroids;
  compute_prim_normals_centroids(input, positions_attr, prim_normals,
                                 prim_centroids);

  // Build edge map and detect sharp edges to identify corner points
  std::map<EdgeKey, EdgeInfo> edge_map;
  build_edge_map(input, edge_map);
  auto sharp_edges = find_sharp_edges(edge_map, prim_normals, angle_threshold);

  // Build adjacency: point -> incident faces, and point -> neighbor points
  std::vector<std::vector<int>> point_faces(input.point_count());
  std::vector<std::vector<int>> point_neighbors(input.point_count());

  for (size_t prim_index = 0; prim_index < input.primitive_count();
       ++prim_index) {
    const auto &primitive_vertices =
        input.topology().get_primitive_vertices(prim_index);
    for (int vertex_index : primitive_vertices) {
      int point_index = input.topology().get_vertex_point(vertex_index);
      point_faces[point_index].push_back(static_cast<int>(prim_index));
    }
  }

  for (const auto &edge_map_entry : edge_map) {
    const EdgeKey &edge = edge_map_entry.first;
    point_neighbors[edge.v0].push_back(edge.v1);
    point_neighbors[edge.v1].push_back(edge.v0);
  }

  // Collect points to bevel: endpoints of sharp edges
  std::set<int> points_to_bevel;
  for (const auto &sharp_edge : sharp_edges) {
    points_to_bevel.insert(sharp_edge.v0);
    points_to_bevel.insert(sharp_edge.v1);
  }
  std::cout << "Vertex Bevel: candidate points = " << points_to_bevel.size()
            << "\n";

  static constexpr float BASIS_HELPER_THRESHOLD = 0.5F;
  auto make_basis = [](const Eigen::Vector3f &normal_in) {
    Eigen::Vector3f normal = normal_in;
    if (normal.norm() < EPS_VERY_TINY)
      normal = Eigen::Vector3f(0, 0, 1);
    else
      normal.normalize();
    // Choose a helper vector least aligned with normal
    Eigen::Vector3f helper = (std::fabs(normal.x()) < BASIS_HELPER_THRESHOLD)
                                 ? Eigen::Vector3f(1, 0, 0)
                                 : Eigen::Vector3f(0, 1, 0);
    Eigen::Vector3f tangent = normal.cross(helper);
    if (tangent.norm() < EPS_VERY_TINY)
      tangent = normal.cross(Eigen::Vector3f(0, 0, 1));
    tangent.normalize();
    Eigen::Vector3f bitangent = normal.cross(tangent);
    bitangent.normalize();
    return std::make_pair(tangent, bitangent);
  };

  // For each point, create a small cap polygon using offsets along incident
  // face normals
  for (int point_index : points_to_bevel) {
    const Eigen::Vector3f original_point = (*positions_attr)[point_index];

    // Clamp width by local neighbor distances (half shortest edge length)
    float local_width = bevel_width;
    float min_half_edge = std::numeric_limits<float>::infinity();
    for (int neighbor_point : point_neighbors[point_index]) {
      float edge_length =
          ((*positions_attr)[neighbor_point] - original_point).norm();
      min_half_edge = std::min(min_half_edge, (HALF * edge_length) - CLAMP_EPS);
    }
    if (std::isfinite(min_half_edge)) {
      local_width = std::max(0.0F, std::min(local_width, min_half_edge));
    }
    if (local_width <= 0.0F)
      continue;

    // Collect unique incident faces for this point
    auto &incident_faces = point_faces[point_index];
    if (incident_faces.size() < 2)
      continue; // nothing to bevel

    // Average normal
    Eigen::Vector3f Nsum(0, 0, 0);
    std::vector<Eigen::Vector3f> used_normals;
    used_normals.reserve(incident_faces.size());
    for (int face_index : incident_faces) {
      Eigen::Vector3f face_normal =
          prim_normals[static_cast<size_t>(face_index)];
      if (face_normal.norm() < EPS_VERY_TINY)
        continue;
      face_normal.normalize();
      used_normals.push_back(face_normal);
      Nsum += face_normal;
    }
    if (used_normals.size() < 2)
      continue;
    Eigen::Vector3f average_normal =
        (Nsum.norm() < EPS_VERY_TINY) ? used_normals[0] : (Nsum.normalized());
    auto [tangent, bitangent] = make_basis(average_normal);

    // Build and order ring points by angle in (T,B) plane
    struct RingPt {
      float ang;
      Eigen::Vector3f pos;
    };
    std::vector<RingPt> ring;
    ring.reserve(used_normals.size());
    for (const auto &unit_face_normal : used_normals) {
      Eigen::Vector3f offset_direction = unit_face_normal; // already unit
      Eigen::Vector3f ring_point =
          original_point + local_width * offset_direction;
      Eigen::Vector3f relative = ring_point - original_point;
      float proj_x = relative.dot(tangent);
      float proj_y = relative.dot(bitangent);
      float angle = std::atan2(proj_y, proj_x);
      ring.push_back({angle, ring_point});
    }
    std::sort(
        ring.begin(), ring.end(),
        [](const RingPt &lhs, const RingPt &rhs) { return lhs.ang < rhs.ang; });

    // Write points
    std::vector<Eigen::Vector3f> new_points;
    new_points.reserve(ring.size());
    for (const auto &ring_point_entry : ring)
      new_points.push_back(ring_point_entry.pos);
    size_t base_point = append_points(*result, result_positions, new_points);

    // Create vertices and one n-gon primitive
    size_t ring_size = new_points.size();
    if (ring_size < 3) {
      continue; // can't form a cap
    }
    size_t base_vertex = result->vertex_count();
    result->set_vertex_count(base_vertex + ring_size);
    auto &topo = result->topology();
    for (size_t i = 0; i < ring_size; ++i) {
      size_t vertex_index = base_vertex + i;
      size_t out_point_index = base_point + i;
      topo.set_vertex_point(vertex_index, static_cast<int>(out_point_index));
    }
    // Triangulate the ring as a fan anchored at the first vertex (0, i, i+1)
    for (size_t i = 1; i + 1 < ring_size; ++i) {
      std::vector<int> tri = {static_cast<int>(base_vertex + 0),
                              static_cast<int>(base_vertex + i),
                              static_cast<int>(base_vertex + i + 1)};
      result->add_primitive(tri);
    }
  }

  // Update normals for all primitives
  recompute_primitive_normals(*result);
  return result;
}

static std::shared_ptr<core::GeometryContainer>
bevel_faces(const core::GeometryContainer &input, float /*bevel_width*/,
            int /*segments*/, float /*angle_threshold*/) {
  std::cout << "Face Bevel: not implemented yet, passing through\n";
  return std::make_shared<core::GeometryContainer>(input.clone());
}

BevelSOP::BevelSOP(const std::string &name) : SOPNode(name, "Bevel") {
  // One geometry input
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);
  // Bevel parameters --------------------------------------------------------
  register_parameter(define_float_parameter("width", DEFAULT_WIDTH)
                         .label("Width")
                         .category("Bevel")
                         .description("Bevel width")
                         .range(0.0, MAX_WIDTH_RANGE)
                         .build());

  register_parameter(define_int_parameter("segments", DEFAULT_SEGMENTS)
                         .label("Segments")
                         .category("Bevel")
                         .description("Number of segments for rounded bevel")
                         .range(1, MAX_SEGMENTS_RANGE)
                         .build());

  register_parameter(define_float_parameter("profile", DEFAULT_PROFILE)
                         .label("Profile")
                         .category("Bevel")
                         .description("Bevel profile shape (0=linear, 1=round)")
                         .range(0.0, 1.0)
                         .build());

  register_parameter(
      define_int_parameter("mode", static_cast<int>(BevelType::Edge))
          .label("Mode")
          .category("Bevel")
          .description("Bevel mode: Vertex, Edge, Face")
          .options({"Vertex", "Edge", "Face"})
          .build());

  register_parameter(define_bool_parameter("clamp_overlap", true)
                         .label("Clamp Overlap")
                         .category("Bevel")
                         .description("Prevent bevel from overlapping itself")
                         .build());

  // Limits ------------------------------------------------------------------
  register_parameter(
      define_float_parameter("angle_limit", DEFAULT_ANGLE_LIMIT)
          .label("Angle Limit")
          .category("Limits")
          .description("Minimum dihedral angle (deg) to bevel edge")
          .range(0.0, MAX_ANGLE_LIMIT)
          .build());
}

std::shared_ptr<core::GeometryContainer> BevelSOP::execute() {
  auto input = get_input_data(0);
  if (!input) {
    std::cerr << "BevelSOP: No input geometry\n";
    return nullptr;
  }

  std::cout << "\n=== Bevel SOP ===\n";
  std::cout << "Input: " << input->point_count() << " points, "
            << input->primitive_count() << " primitives\n";

  // Get parameters
  float bevel_width = get_parameter<float>("width", DEFAULT_WIDTH);
  int segments = get_parameter<int>("segments", DEFAULT_SEGMENTS);
  float angle_threshold =
      get_parameter<float>("angle_limit", DEFAULT_ANGLE_LIMIT);
  auto mode = static_cast<BevelType>(
      get_parameter<int>("mode", static_cast<int>(BevelType::Edge)));

  std::cout << "Bevel width: " << bevel_width << ", segments: " << segments
            << ", angle threshold: " << angle_threshold << "°\n";
  std::cout << "Mode: " << static_cast<int>(mode)
            << " (0=Vertex, 1=Edge, 2=Face)\n";

  std::shared_ptr<core::GeometryContainer> result;

  bool clamp_overlap = get_parameter<bool>("clamp_overlap", true);
  float profile = get_parameter<float>("profile", DEFAULT_PROFILE);

  switch (mode) {
  case BevelType::Vertex:
    result = bevel_vertices(*input, bevel_width, segments, angle_threshold);
    break;
  case BevelType::Edge:
    result = bevel_edges(*input, bevel_width, segments, angle_threshold,
                         profile, clamp_overlap);
    break;
  case BevelType::Face:
    result = bevel_faces(*input, bevel_width, segments, angle_threshold);
    break;
  }

  if (result) {
    std::cout << "Result: " << result->point_count() << " points, "
              << result->primitive_count() << " primitives\n";
  } else {
    std::cout << "Bevel operation returned no result\n";
  }

  std::cout << "=================\n\n";

  return result;
}

} // namespace nodo::sop
