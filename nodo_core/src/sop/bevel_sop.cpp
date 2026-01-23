// Clean consolidated implementation for Bevel SOP – removes previous duplicated
// blocks.
#include "nodo/sop/bevel_sop.hpp"

#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

namespace nodo::sop {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
constexpr float EPS_VERY_TINY = 1e-8F;
constexpr float HALF = 0.5F;
constexpr float CLAMP_EPS = 1e-5F;
constexpr float PROFILE_MID = 0.5F;
constexpr float GAMMA_MIN = 0.5F;
constexpr float GAMMA_MAX = 1.5F;
constexpr float DEG_PER_RAD = 180.0F / 3.14159265358979323846F;
// Default width/segments/profile values are defined in BevelSOP (header)
constexpr float DEFAULT_ANGLE_LIMIT = 30.0F;
constexpr int MAX_SEGMENTS_RANGE = 16;
constexpr float MAX_WIDTH_RANGE = 10.0F;
constexpr float MAX_ANGLE_LIMIT = 180.0F;
constexpr float BASIS_HELPER_THRESHOLD = 0.5F;

// ---------------------------------------------------------------------------
// Edge structures
// ---------------------------------------------------------------------------
struct EdgeKey {
  int point_index_a{};
  int point_index_b{};
  EdgeKey() = default;
  EdgeKey(int a_index, int b_index)
      : point_index_a(std::min(a_index, b_index)), point_index_b(std::max(a_index, b_index)) {}
  bool operator<(const EdgeKey& other) const {
    if (point_index_a != other.point_index_a)
      return point_index_a < other.point_index_a;
    return point_index_b < other.point_index_b;
  }
};
struct EdgeInfo {
  std::vector<int> faces;
};

// Corner reuse mapping to stitch vertex ring to edge strip endpoints
struct CornerReuseMap {
  // point_index (original corner) -> (face_index -> created point_index)
  std::unordered_map<int, std::unordered_map<int, int>> map;
};

// ---------------------------------------------------------------------------
// Utility helpers
// ---------------------------------------------------------------------------
static inline float shape_profile_t(float t_value, float profile) {
  float gamma = (profile <= PROFILE_MID) ? (PROFILE_MID + profile) : (profile + PROFILE_MID);
  gamma = std::clamp(gamma, GAMMA_MIN, GAMMA_MAX);
  float t_clamped = std::clamp(t_value, 0.0F, 1.0F);
  return std::pow(t_clamped, gamma);
}

static size_t append_points(core::GeometryContainer& geo, core::AttributeStorage<Eigen::Vector3f>*& positions,
                            const std::vector<Eigen::Vector3f>& new_points) {
  if (new_points.empty())
    return geo.point_count();
  size_t base_point_count = geo.point_count();
  geo.set_point_count(base_point_count + new_points.size());
  positions = geo.get_point_attribute_typed<Eigen::Vector3f>(core::standard_attrs::P);
  if (positions == nullptr)
    return base_point_count;
  for (size_t i = 0; i < new_points.size(); ++i)
    (*positions)[base_point_count + i] = new_points[i];
  return base_point_count;
}

// Compute primitive normals & centroids (Newell)
static void compute_prim_normals_centroids(const core::GeometryContainer& input,
                                           const core::AttributeStorage<Eigen::Vector3f>* positions_attr,
                                           std::vector<Eigen::Vector3f>& out_normals,
                                           std::vector<Eigen::Vector3f>& out_centroids) {
  auto newell_normal = [&](size_t primitive_index) -> Eigen::Vector3f {
    const auto& primitive_vertices = input.topology().get_primitive_vertices(primitive_index);
    if (primitive_vertices.size() < 3)
      return Eigen::Vector3f(0, 0, 0);
    Eigen::Vector3f accum(0, 0, 0);
    for (size_t i = 0; i < primitive_vertices.size(); ++i) {
      int vertex_index_current = primitive_vertices[i];
      int vertex_index_next = primitive_vertices[(i + 1) % primitive_vertices.size()];
      int point_index_current = input.topology().get_vertex_point(vertex_index_current);
      int point_index_next = input.topology().get_vertex_point(vertex_index_next);
      const auto& pos_current = (*positions_attr)[point_index_current];
      const auto& pos_next = (*positions_attr)[point_index_next];
      accum.x() += (pos_current.y() - pos_next.y()) * (pos_current.z() + pos_next.z());
      accum.y() += (pos_current.z() - pos_next.z()) * (pos_current.x() + pos_next.x());
      accum.z() += (pos_current.x() - pos_next.x()) * (pos_current.y() + pos_next.y());
    }
    float len = accum.norm();
    return (len < EPS_VERY_TINY) ? Eigen::Vector3f(0, 0, 0) : accum / len;
  };
  auto compute_centroid = [&](size_t primitive_index) -> Eigen::Vector3f {
    const auto& primitive_vertices = input.topology().get_primitive_vertices(primitive_index);
    if (primitive_vertices.empty())
      return Eigen::Vector3f(0, 0, 0);
    Eigen::Vector3f centroid(0, 0, 0);
    for (int vertex_index : primitive_vertices) {
      int point_index = input.topology().get_vertex_point(vertex_index);
      centroid += (*positions_attr)[point_index];
    }
    return centroid / static_cast<float>(primitive_vertices.size());
  };
  size_t primitive_count = input.primitive_count();
  out_normals.resize(primitive_count);
  out_centroids.resize(primitive_count);
  for (size_t prim_i = 0; prim_i < primitive_count; ++prim_i) {
    out_normals[prim_i] = newell_normal(prim_i);
    out_centroids[prim_i] = compute_centroid(prim_i);
  }
}

static void build_edge_map(const core::GeometryContainer& input, std::map<EdgeKey, EdgeInfo>& edge_map) {
  for (size_t primitive_index = 0; primitive_index < input.primitive_count(); ++primitive_index) {
    const auto& primitive_vertices = input.topology().get_primitive_vertices(primitive_index);
    size_t vertex_count = primitive_vertices.size();
    for (size_t i = 0; i < vertex_count; ++i) {
      int vertex_index_current = primitive_vertices[i];
      int vertex_index_next = primitive_vertices[(i + 1) % vertex_count];
      int point_index_current = input.topology().get_vertex_point(vertex_index_current);
      int point_index_next = input.topology().get_vertex_point(vertex_index_next);
      EdgeKey key(point_index_current, point_index_next);
      edge_map[key].faces.push_back(static_cast<int>(primitive_index));
    }
  }
}

static std::vector<EdgeKey> find_sharp_edges(const std::map<EdgeKey, EdgeInfo>& edge_map,
                                             const std::vector<Eigen::Vector3f>& prim_normals, float angle_threshold) {
  std::vector<EdgeKey> result;
  for (const auto& edge_entry : edge_map) {
    const EdgeKey& key = edge_entry.first;
    const EdgeInfo& info = edge_entry.second;
    if (info.faces.size() == 2) {
      const Eigen::Vector3f& normal_a = prim_normals[info.faces[0]];
      const Eigen::Vector3f& normal_b = prim_normals[info.faces[1]];
      float dotv = std::clamp(normal_a.dot(normal_b), -1.0F, 1.0F);
      float angle_deg = std::acos(dotv) * DEG_PER_RAD;
      if (angle_deg >= angle_threshold)
        result.push_back(key);
    }
  }
  return result;
}

// Primitive normals recompute
static void recompute_primitive_normals(core::GeometryContainer& geo) {
  geo.ensure_position_attribute();
  auto* positions_attr = geo.get_point_attribute_typed<Eigen::Vector3f>(core::standard_attrs::P);
  if (positions_attr == nullptr)
    return;
  if (!geo.has_primitive_attribute(core::standard_attrs::primitive_N)) {
    geo.add_primitive_attribute(core::standard_attrs::primitive_N, core::AttributeType::VEC3F);
  }
  auto* prim_normals_attr = geo.get_primitive_attribute_typed<Eigen::Vector3f>(core::standard_attrs::primitive_N);
  if (prim_normals_attr == nullptr)
    return;
  prim_normals_attr->resize(geo.primitive_count());
  for (size_t prim_i = 0; prim_i < geo.primitive_count(); ++prim_i) {
    const auto& primitive_vertices = geo.topology().get_primitive_vertices(prim_i);
    if (primitive_vertices.size() < 3) {
      (*prim_normals_attr)[prim_i] = Eigen::Vector3f(0, 0, 0);
      continue;
    }
    Eigen::Vector3f accum(0, 0, 0);
    for (size_t i = 0; i < primitive_vertices.size(); ++i) {
      int vertex_index_current = primitive_vertices[i];
      int vertex_index_next = primitive_vertices[(i + 1) % primitive_vertices.size()];
      int point_index_current = geo.topology().get_vertex_point(vertex_index_current);
      int point_index_next = geo.topology().get_vertex_point(vertex_index_next);
      const auto& pos_current = (*positions_attr)[point_index_current];
      const auto& pos_next = (*positions_attr)[point_index_next];
      accum.x() += (pos_current.y() - pos_next.y()) * (pos_current.z() + pos_next.z());
      accum.y() += (pos_current.z() - pos_next.z()) * (pos_current.x() + pos_next.x());
      accum.z() += (pos_current.x() - pos_next.x()) * (pos_current.y() + pos_next.y());
    }
    float len = accum.norm();
    (*prim_normals_attr)[prim_i] = (len < EPS_VERY_TINY) ? Eigen::Vector3f(0, 0, 0) : accum / len;
  }
}

// Edge bevel (quad and multi-seg stub)
static void add_chamfer_quad_for_edge(core::GeometryContainer& geo,
                                      core::AttributeStorage<Eigen::Vector3f>* positions_attr,
                                      const std::vector<Eigen::Vector3f>& prim_normals, const EdgeKey& edge_key,
                                      const EdgeInfo& info, float bevel_width) {
  if (positions_attr == nullptr || info.faces.size() != 2)
    return;
  int point_index_a = edge_key.point_index_a;
  int point_index_b = edge_key.point_index_b;
  const Eigen::Vector3f& pos_a = (*positions_attr)[point_index_a];
  const Eigen::Vector3f& pos_b = (*positions_attr)[point_index_b];
  const Eigen::Vector3f& normal_face_a = prim_normals[info.faces[0]];
  const Eigen::Vector3f& normal_face_b = prim_normals[info.faces[1]];
  Eigen::Vector3f a_offset_face_a = pos_a + bevel_width * normal_face_a;
  Eigen::Vector3f b_offset_face_a = pos_b + bevel_width * normal_face_a;
  Eigen::Vector3f b_offset_face_b = pos_b + bevel_width * normal_face_b;
  Eigen::Vector3f a_offset_face_b = pos_a + bevel_width * normal_face_b;
  size_t base_point = geo.point_count();
  geo.set_point_count(base_point + 4);
  positions_attr = geo.get_point_attribute_typed<Eigen::Vector3f>(core::standard_attrs::P);
  (*positions_attr)[base_point + 0] = a_offset_face_a;
  (*positions_attr)[base_point + 1] = b_offset_face_a;
  (*positions_attr)[base_point + 2] = b_offset_face_b;
  (*positions_attr)[base_point + 3] = a_offset_face_b;
  size_t base_vertex = geo.vertex_count();
  geo.set_vertex_count(base_vertex + 4);
  auto& topology = geo.topology();
  topology.set_vertex_point(static_cast<int>(base_vertex + 0), static_cast<int>(base_point + 0));
  topology.set_vertex_point(static_cast<int>(base_vertex + 1), static_cast<int>(base_point + 1));
  topology.set_vertex_point(static_cast<int>(base_vertex + 2), static_cast<int>(base_point + 2));
  topology.set_vertex_point(static_cast<int>(base_vertex + 3), static_cast<int>(base_point + 3));
  geo.add_primitive({static_cast<int>(base_vertex + 0), static_cast<int>(base_vertex + 1),
                     static_cast<int>(base_vertex + 2), static_cast<int>(base_vertex + 3)});
}

static void add_chamfer_strip_for_edge(core::GeometryContainer& geo,
                                       core::AttributeStorage<Eigen::Vector3f>*& positions_attr,
                                       const core::AttributeStorage<Eigen::Vector3f>* /*input_positions*/,
                                       const std::vector<Eigen::Vector3f>& prim_normals, const EdgeKey& edge_key,
                                       const EdgeInfo& info, int segments, float bevel_width, float profile,
                                       bool /*clamp*/) {
  // Build a rounded strip by interpolating normals between the two faces.
  if (positions_attr == nullptr || info.faces.size() != 2 || segments < 1 || bevel_width <= 0.0F) {
    return;
  }

  const int point_index_a = edge_key.point_index_a;
  const int point_index_b = edge_key.point_index_b;
  const Eigen::Vector3f pos_a = (*positions_attr)[point_index_a];
  const Eigen::Vector3f pos_b = (*positions_attr)[point_index_b];

  Eigen::Vector3f normal_face_0 = prim_normals[info.faces[0]];
  Eigen::Vector3f normal_face_1 = prim_normals[info.faces[1]];
  if (normal_face_0.norm() < EPS_VERY_TINY || normal_face_1.norm() < EPS_VERY_TINY) {
    add_chamfer_quad_for_edge(geo, positions_attr, prim_normals, edge_key, info, bevel_width);
    return;
  }
  normal_face_0.normalize();
  normal_face_1.normalize();

  // Precompute strip points: for each slice s in [0..segments], two points
  // (a_s, b_s)
  std::vector<Eigen::Vector3f> strip_points;
  strip_points.reserve(static_cast<size_t>((segments + 1) * 2));
  for (int segment_index = 0; segment_index <= segments; ++segment_index) {
    float param_t = static_cast<float>(segment_index) / static_cast<float>(segments);
    float t_shaped = shape_profile_t(param_t, profile);
    // Interpolate normals (simple lerp then normalize)
    Eigen::Vector3f blended_normal = ((1.0F - t_shaped) * normal_face_0) + (t_shaped * normal_face_1);
    if (blended_normal.norm() < EPS_VERY_TINY) {
      blended_normal = normal_face_0; // fallback
    } else {
      blended_normal.normalize();
    }
    Eigen::Vector3f offset_a = pos_a + bevel_width * blended_normal;
    Eigen::Vector3f offset_b = pos_b + bevel_width * blended_normal;
    strip_points.push_back(offset_a);
    strip_points.push_back(offset_b);
  }

  // Append new points once
  size_t base_point = append_points(geo, positions_attr, strip_points);

  // For each adjacent pair of slices, create one quad
  auto& topology = geo.topology();
  for (int segment_index = 0; segment_index < segments; ++segment_index) {
    int a0_point_index = static_cast<int>(base_point + (2 * segment_index));
    int b0_point_index = static_cast<int>(base_point + (2 * segment_index) + 1);
    int a1_point_index = static_cast<int>(base_point + (2 * (segment_index + 1)));
    int b1_point_index = static_cast<int>(base_point + (2 * (segment_index + 1)) + 1);

    size_t base_vertex = geo.vertex_count();
    geo.set_vertex_count(base_vertex + 4);
    int vertex_index_0 = static_cast<int>(base_vertex + 0);
    int vertex_index_1 = static_cast<int>(base_vertex + 1);
    int vertex_index_2 = static_cast<int>(base_vertex + 2);
    int vertex_index_3 = static_cast<int>(base_vertex + 3);
    geo.add_primitive({vertex_index_0, vertex_index_1, vertex_index_2, vertex_index_3});
    topology.set_vertex_point(vertex_index_0, a0_point_index);
    topology.set_vertex_point(vertex_index_1, b0_point_index);
    topology.set_vertex_point(vertex_index_2, b1_point_index);
    topology.set_vertex_point(vertex_index_3, a1_point_index);
  }
}

// Vertex bevel patch (multi-ring)
enum class VertexPatchMode : std::uint8_t {
  ApexFan = 0,
  RingStart = 1
};

static void add_vertex_bevel_patch_apexfan(core::GeometryContainer& geo,
                                           core::AttributeStorage<Eigen::Vector3f>*& result_positions,
                                           const core::AttributeStorage<Eigen::Vector3f>* input_positions,
                                           const std::vector<Eigen::Vector3f>& prim_normals, int point_index,
                                           const std::vector<int>& incident_faces,
                                           const std::vector<int>& neighbor_points, float bevel_width, int segments,
                                           float profile) {
  if (segments < 1 || bevel_width <= 0.0F)
    return;
  const Eigen::Vector3f& origin = (*input_positions)[point_index];
  float min_half_edge = std::numeric_limits<float>::infinity(); // Initialize min_half_edge
  for (int neighbor_index : neighbor_points) {
    float distance = ((*input_positions)[neighbor_index] - origin).norm();
    min_half_edge = std::min(min_half_edge, (HALF * distance) - CLAMP_EPS);
  }
  float local_width = bevel_width;
  if (std::isfinite(min_half_edge))
    local_width = std::min(local_width, min_half_edge);
  if (local_width <= 0.0F)
    return;
  std::vector<Eigen::Vector3f> normals;
  normals.reserve(incident_faces.size());
  Eigen::Vector3f normal_accum(0, 0, 0);
  for (int face_index : incident_faces) {
    Eigen::Vector3f face_normal = prim_normals[static_cast<size_t>(face_index)];
    if (face_normal.norm() < EPS_VERY_TINY) {
      continue;
    }
    face_normal.normalize();
    normals.push_back(face_normal);
    normal_accum += face_normal;
  }
  if (normals.size() < 2)
    return;
  Eigen::Vector3f average_normal = (normal_accum.norm() < EPS_VERY_TINY) ? normals[0] : normal_accum.normalized();
  Eigen::Vector3f helper_vec =
      (std::fabs(average_normal.x()) < BASIS_HELPER_THRESHOLD) ? Eigen::Vector3f(1, 0, 0) : Eigen::Vector3f(0, 1, 0);
  Eigen::Vector3f tangent_vec = average_normal.cross(helper_vec);
  if (tangent_vec.norm() < EPS_VERY_TINY)
    tangent_vec = average_normal.cross(Eigen::Vector3f(0, 0, 1));
  tangent_vec.normalize();
  Eigen::Vector3f bitangent_vec = average_normal.cross(tangent_vec);
  bitangent_vec.normalize();
  struct OrderedDir {
    float angle;
    Eigen::Vector3f dir;
  };
  std::vector<OrderedDir> ordered_dirs;
  ordered_dirs.reserve(normals.size());
  for (auto& face_normal : normals) {
    float x_comp = face_normal.dot(tangent_vec);
    float y_comp = face_normal.dot(bitangent_vec);
    float angle = std::atan2(y_comp, x_comp);
    ordered_dirs.push_back({angle, face_normal});
  }
  std::sort(ordered_dirs.begin(), ordered_dirs.end(),
            [](const OrderedDir& lhs, const OrderedDir& rhs) { return lhs.angle < rhs.angle; });
  int direction_count = static_cast<int>(ordered_dirs.size());
  std::vector<Eigen::Vector3f> ring_points;
  ring_points.reserve(static_cast<size_t>(direction_count * segments));
  for (int ring_i = 1; ring_i <= segments; ++ring_i) {
    float t_raw = static_cast<float>(ring_i) / static_cast<float>(segments);
    float t_shaped = shape_profile_t(t_raw, profile);
    float radius = local_width * t_shaped;
    for (const auto& entry : ordered_dirs)
      ring_points.push_back(origin + radius * entry.dir);
  }
  size_t base_point = append_points(geo, result_positions, ring_points);
  auto& topology = geo.topology();
  size_t base_vertex = geo.vertex_count();
  geo.set_vertex_count(base_vertex + 1 + ring_points.size());
  topology.set_vertex_point(base_vertex, point_index); // apex
  size_t first_ring_vertex_start = base_vertex + 1;
  for (size_t i = 0; i < ring_points.size(); ++i)
    topology.set_vertex_point(first_ring_vertex_start + i, static_cast<int>(base_point + i));
  if (segments >= 1) {
    for (int dir_i = 0; dir_i < direction_count; ++dir_i) {
      int v_apex = static_cast<int>(base_vertex);
      int v_curr = static_cast<int>(first_ring_vertex_start + dir_i);
      int v_next = static_cast<int>(first_ring_vertex_start + ((dir_i + 1) % direction_count));
      geo.add_primitive({v_apex, v_curr, v_next});
    }
  }
  for (int ring_i = 1; ring_i < segments; ++ring_i) {
    size_t prev_start = first_ring_vertex_start + static_cast<size_t>((ring_i - 1) * direction_count);
    size_t curr_start = first_ring_vertex_start + static_cast<size_t>(ring_i * direction_count);
    for (int dir_i = 0; dir_i < direction_count; ++dir_i) {
      int dir_next = (dir_i + 1) % direction_count;
      int v_prev0 = static_cast<int>(prev_start + dir_i);
      int v_prev1 = static_cast<int>(prev_start + dir_next);
      int v_curr1 = static_cast<int>(curr_start + dir_next);
      int v_curr0 = static_cast<int>(curr_start + dir_i);
      geo.add_primitive({v_prev0, v_prev1, v_curr1, v_curr0});
    }
  }
}

// Grid variant: per-ring grid sampling between adjacent face normals (angular
// subdivisions)
static void add_vertex_bevel_patch_grid(core::GeometryContainer& geo,
                                        core::AttributeStorage<Eigen::Vector3f>*& result_positions,
                                        const core::AttributeStorage<Eigen::Vector3f>* input_positions,
                                        const std::vector<Eigen::Vector3f>& prim_normals, int point_index,
                                        const std::vector<int>& incident_faces, const std::vector<int>& neighbor_points,
                                        float bevel_width, int segments, float profile,
                                        const CornerReuseMap* reuse = nullptr) {
  if (segments < 1 || bevel_width <= 0.0F)
    return;
  const Eigen::Vector3f& origin = (*input_positions)[point_index];

  float min_half_edge = std::numeric_limits<float>::infinity();
  for (int neighbor_index : neighbor_points) {
    float distance = ((*input_positions)[neighbor_index] - origin).norm();
    min_half_edge = std::min(min_half_edge, (HALF * distance) - CLAMP_EPS);
  }
  float local_width = bevel_width;
  if (std::isfinite(min_half_edge))
    local_width = std::min(local_width, min_half_edge);
  if (local_width <= 0.0F)
    return;

  struct FaceDir {
    int face;
    Eigen::Vector3f n;
  };
  std::vector<FaceDir> face_dirs;
  face_dirs.reserve(incident_faces.size());
  Eigen::Vector3f normal_accum(0, 0, 0);
  for (int face_index : incident_faces) {
    Eigen::Vector3f nrm = prim_normals[static_cast<size_t>(face_index)];
    if (nrm.norm() < EPS_VERY_TINY)
      continue;
    nrm.normalize();
    face_dirs.push_back({face_index, nrm});
    normal_accum += nrm;
  }
  if (face_dirs.size() < 2)
    return;
  Eigen::Vector3f average_normal = (normal_accum.norm() < EPS_VERY_TINY) ? face_dirs[0].n : normal_accum.normalized();

  Eigen::Vector3f helper_vec =
      (std::fabs(average_normal.x()) < BASIS_HELPER_THRESHOLD) ? Eigen::Vector3f(1, 0, 0) : Eigen::Vector3f(0, 1, 0);
  Eigen::Vector3f tangent_vec = average_normal.cross(helper_vec);
  if (tangent_vec.norm() < EPS_VERY_TINY)
    tangent_vec = average_normal.cross(Eigen::Vector3f(0, 0, 1));
  tangent_vec.normalize();
  Eigen::Vector3f bitangent_vec = average_normal.cross(tangent_vec);
  bitangent_vec.normalize();

  struct OrderedFD {
    float angle;
    int face;
    Eigen::Vector3f dir;
  };
  std::vector<OrderedFD> ordered;
  ordered.reserve(face_dirs.size());
  for (const auto& fd_entry : face_dirs) {
    float x_comp = fd_entry.n.dot(tangent_vec);
    float y_comp = fd_entry.n.dot(bitangent_vec);
    float ang = std::atan2(y_comp, x_comp);
    ordered.push_back({ang, fd_entry.face, fd_entry.n});
  }
  std::sort(ordered.begin(), ordered.end(),
            [](const OrderedFD& left, const OrderedFD& right) { return left.angle < right.angle; });
  int direction_count = static_cast<int>(ordered.size());
  if (direction_count < 2)
    return;

  const int u_divs_per_wedge = std::max(1, segments);
  const int total_angular_samples = direction_count * u_divs_per_wedge;

  auto& topology = geo.topology();
  std::vector<std::vector<int>> ring_vertices;
  ring_vertices.reserve(static_cast<size_t>(segments));

  for (int ring_i = 1; ring_i <= segments; ++ring_i) {
    float t_raw = static_cast<float>(ring_i) / static_cast<float>(segments);
    float t_shaped = shape_profile_t(t_raw, profile);
    float radius = local_width * t_shaped;

    std::vector<int> point_ids(static_cast<size_t>(total_angular_samples), -1);
    struct PendingPoint {
      int k;
      Eigen::Vector3f pos;
    };
    std::vector<PendingPoint> pending;
    pending.reserve(static_cast<size_t>(total_angular_samples));

    for (int k = 0; k < total_angular_samples; ++k) {
      int wedge_idx = k / u_divs_per_wedge;
      int u_sub = k % u_divs_per_wedge;
      const OrderedFD& fd0 = ordered[wedge_idx];
      const OrderedFD& fd1 = ordered[(wedge_idx + 1) % direction_count];
      float u_t = static_cast<float>(u_sub) / static_cast<float>(u_divs_per_wedge);
      Eigen::Vector3f blended_dir = ((1.0F - u_t) * fd0.dir) + (u_t * fd1.dir);
      if (blended_dir.norm() < EPS_VERY_TINY)
        blended_dir = fd0.dir;
      blended_dir.normalize();
      if (ring_i == segments && reuse != nullptr && u_sub == 0) {
        auto itp = reuse->map.find(point_index);
        if (itp != reuse->map.end()) {
          auto itf = itp->second.find(fd0.face);
          if (itf != itp->second.end()) {
            point_ids[static_cast<size_t>(k)] = itf->second;
            continue;
          }
        }
      }
      pending.push_back({k, origin + radius * blended_dir});
    }

    if (!pending.empty()) {
      std::vector<Eigen::Vector3f> add_positions;
      add_positions.reserve(pending.size());
      for (const auto& pend : pending)
        add_positions.push_back(pend.pos);
      size_t base_point = append_points(geo, result_positions, add_positions);
      for (size_t i = 0; i < pending.size(); ++i) {
        point_ids[static_cast<size_t>(pending[i].k)] = static_cast<int>(base_point + i);
      }
    }

    size_t base_vertex = geo.vertex_count();
    geo.set_vertex_count(base_vertex + static_cast<size_t>(total_angular_samples));
    std::vector<int> ring_vert_ids;
    ring_vert_ids.reserve(static_cast<size_t>(total_angular_samples));
    for (int k = 0; k < total_angular_samples; ++k) {
      int v_id = static_cast<int>(base_vertex + k);
      ring_vert_ids.push_back(v_id);
      topology.set_vertex_point(v_id, point_ids[static_cast<size_t>(k)]);
    }
    ring_vertices.push_back(std::move(ring_vert_ids));

    if (ring_vertices.size() >= 2) {
      const auto& prev_ring = ring_vertices[ring_vertices.size() - 2];
      const auto& curr_ring = ring_vertices.back();
      for (int k = 0; k < total_angular_samples; ++k) {
        int k_next = (k + 1) % total_angular_samples;
        int v_prev0 = prev_ring[static_cast<size_t>(k)];
        int v_prev1 = prev_ring[static_cast<size_t>(k_next)];
        int v_curr1 = curr_ring[static_cast<size_t>(k_next)];
        int v_curr0 = curr_ring[static_cast<size_t>(k)];
        geo.add_primitive({v_prev0, v_prev1, v_curr1, v_curr0});
      }
    }
  }
}

// RingStart variant: omit apex fan; build only ring quads starting from first
// ring
static void add_vertex_bevel_patch_ringstart(core::GeometryContainer& geo,
                                             core::AttributeStorage<Eigen::Vector3f>*& result_positions,
                                             const core::AttributeStorage<Eigen::Vector3f>* input_positions,
                                             const std::vector<Eigen::Vector3f>& prim_normals, int point_index,
                                             const std::vector<int>& incident_faces,
                                             const std::vector<int>& neighbor_points, float bevel_width, int segments,
                                             float profile, const CornerReuseMap* reuse = nullptr) {
  if (segments < 1 || bevel_width <= 0.0F)
    return;
  const Eigen::Vector3f& origin = (*input_positions)[point_index];
  float min_half_edge = std::numeric_limits<float>::infinity();
  for (int neighbor_index : neighbor_points) {
    float distance = ((*input_positions)[neighbor_index] - origin).norm();
    min_half_edge = std::min(min_half_edge, (HALF * distance) - CLAMP_EPS);
  }
  float local_width = bevel_width;
  if (std::isfinite(min_half_edge))
    local_width = std::min(local_width, min_half_edge);
  if (local_width <= 0.0F)
    return;

  struct FaceDir {
    int face;
    Eigen::Vector3f n;
  };
  std::vector<FaceDir> face_dirs;
  face_dirs.reserve(incident_faces.size());
  Eigen::Vector3f normal_accum(0, 0, 0);
  for (int face_index : incident_faces) {
    Eigen::Vector3f face_normal = prim_normals[static_cast<size_t>(face_index)];
    if (face_normal.norm() < EPS_VERY_TINY)
      continue;
    face_normal.normalize();
    face_dirs.push_back({face_index, face_normal});
    normal_accum += face_normal;
  }
  if (face_dirs.size() < 2)
    return;
  Eigen::Vector3f average_normal = (normal_accum.norm() < EPS_VERY_TINY) ? face_dirs[0].n : normal_accum.normalized();
  Eigen::Vector3f helper_vec =
      (std::fabs(average_normal.x()) < BASIS_HELPER_THRESHOLD) ? Eigen::Vector3f(1, 0, 0) : Eigen::Vector3f(0, 1, 0);
  Eigen::Vector3f tangent_vec = average_normal.cross(helper_vec);
  if (tangent_vec.norm() < EPS_VERY_TINY)
    tangent_vec = average_normal.cross(Eigen::Vector3f(0, 0, 1));
  tangent_vec.normalize();
  Eigen::Vector3f bitangent_vec = average_normal.cross(tangent_vec);
  bitangent_vec.normalize();
  struct OrderedDir {
    float angle;
    Eigen::Vector3f dir;
  };
  std::vector<OrderedDir> ordered_dirs;
  ordered_dirs.reserve(face_dirs.size());
  for (auto& face_dir_entry : face_dirs) {
    float x_comp = face_dir_entry.n.dot(tangent_vec);
    float y_comp = face_dir_entry.n.dot(bitangent_vec);
    float angle = std::atan2(y_comp, x_comp);
    ordered_dirs.push_back({angle, face_dir_entry.n});
  }
  std::sort(ordered_dirs.begin(), ordered_dirs.end(),
            [](const OrderedDir& lhs, const OrderedDir& rhs) { return lhs.angle < rhs.angle; });
  int direction_count = static_cast<int>(ordered_dirs.size());

  auto& topology = geo.topology();
  // Store per ring vertex indices
  std::vector<std::vector<int>> ring_vertices;
  ring_vertices.reserve(static_cast<size_t>(segments));
  for (int ring_i = 1; ring_i <= segments; ++ring_i) {
    float t_raw = static_cast<float>(ring_i) / static_cast<float>(segments);
    float t_shaped = shape_profile_t(t_raw, profile);
    float radius = local_width * t_shaped;

    std::vector<int> vertex_ids_for_ring;
    vertex_ids_for_ring.reserve(static_cast<size_t>(direction_count));

    // Prepare pending positions if we need to create points
    struct Pending {
      int dir_i;
      Eigen::Vector3f pos;
    };
    std::vector<Pending> pending;

    // Either reuse or create points
    std::vector<int> point_ids_for_ring(direction_count, -1);
    for (int dir_i = 0; dir_i < direction_count; ++dir_i) {
      const auto& entry = ordered_dirs[dir_i];
      if (ring_i == segments && reuse != nullptr) {
        // Try reuse: find face index closest to this direction by normal match
        // We approximate by searching incident_faces for a normal equal to
        // entry.dir
        int matched_face = -1;
        // Incident faces are provided; pick face whose normal matches entry.dir
        // best
        float best_dot = -1.0F;
        for (int face_index : incident_faces) {
          Eigen::Vector3f face_nrm = prim_normals[static_cast<size_t>(face_index)];
          if (face_nrm.norm() < EPS_VERY_TINY)
            continue;
          face_nrm.normalize();
          float dot_candidate = std::clamp(face_nrm.dot(entry.dir), -1.0F, 1.0F);
          if (dot_candidate > best_dot) {
            best_dot = dot_candidate;
            matched_face = face_index;
          }
        }
        if (matched_face >= 0) {
          auto itp = reuse->map.find(point_index);
          if (itp != reuse->map.end()) {
            auto itf = itp->second.find(matched_face);
            if (itf != itp->second.end()) {
              point_ids_for_ring[dir_i] = itf->second;
            }
          }
        }
      }
      if (point_ids_for_ring[dir_i] < 0) {
        pending.push_back({dir_i, origin + radius * entry.dir});
      }
    }

    // Append any new points
    if (!pending.empty()) {
      std::vector<Eigen::Vector3f> to_add;
      to_add.reserve(pending.size());
      for (const auto& pend : pending)
        to_add.push_back(pend.pos);
      size_t base_point = append_points(geo, result_positions, to_add);
      for (size_t k = 0; k < pending.size(); ++k) {
        point_ids_for_ring[pending[k].dir_i] = static_cast<int>(base_point + k);
      }
    }

    // Create vertices for this ring
    size_t base_vertex = geo.vertex_count();
    geo.set_vertex_count(base_vertex + static_cast<size_t>(direction_count));
    std::vector<int> ring_vert_ids;
    ring_vert_ids.reserve(direction_count);
    for (int dir_i = 0; dir_i < direction_count; ++dir_i) {
      int v_id = static_cast<int>(base_vertex + dir_i);
      ring_vert_ids.push_back(v_id);
      topology.set_vertex_point(v_id, point_ids_for_ring[dir_i]);
    }
    ring_vertices.push_back(std::move(ring_vert_ids));

    // If not the first ring, connect to previous ring with quads
    if (ring_vertices.size() >= 2) {
      const auto& prev_ring = ring_vertices[ring_vertices.size() - 2];
      const auto& curr_ring = ring_vertices.back();
      for (int dir_i = 0; dir_i < direction_count; ++dir_i) {
        int dir_next = (dir_i + 1) % direction_count;
        int v_prev0 = prev_ring[dir_i];
        int v_prev1 = prev_ring[dir_next];
        int v_curr1 = curr_ring[dir_next];
        int v_curr0 = curr_ring[dir_i];
        geo.add_primitive({v_prev0, v_prev1, v_curr1, v_curr0});
      }
    }
  }
}

// Bevel modes
static std::shared_ptr<core::GeometryContainer> bevel_edges(const core::GeometryContainer& input, float bevel_width,
                                                            int segments, float angle_threshold, float profile,
                                                            bool clamp_overlap,
                                                            CornerReuseMap* corner_reuse = nullptr) {
  std::cout << "\n=== Edge Bevel Mode ===\n";
  if (bevel_width <= 0.0F || segments < 1)
    return std::make_shared<core::GeometryContainer>(input.clone());
  if (!input.has_point_attribute(core::standard_attrs::P)) {
    std::cerr << "Edge Bevel: Missing position attribute P\n";
    return nullptr;
  }
  const auto* positions_attr = input.get_point_attribute_typed<Eigen::Vector3f>(core::standard_attrs::P);
  if (positions_attr == nullptr) {
    std::cerr << "Edge Bevel: Position attribute retrieval failed\n";
    return nullptr;
  }
  auto result = std::make_shared<core::GeometryContainer>(input.clone());
  result->ensure_position_attribute();
  auto* result_positions = result->get_point_attribute_typed<Eigen::Vector3f>(core::standard_attrs::P);
  if (result_positions == nullptr) {
    std::cerr << "Edge Bevel: Failed to access result positions\n";
    return result;
  }
  std::vector<Eigen::Vector3f> prim_normals;
  std::vector<Eigen::Vector3f> prim_centroids;
  compute_prim_normals_centroids(input, positions_attr, prim_normals, prim_centroids);
  std::map<EdgeKey, EdgeInfo> edge_map;
  build_edge_map(input, edge_map);
  auto sharp_edges = find_sharp_edges(edge_map, prim_normals, angle_threshold);
  if (sharp_edges.empty()) {
    std::cout << "Edge Bevel: No sharp edges above threshold (" << angle_threshold << "°)\n";
    return std::make_shared<core::GeometryContainer>(input.clone());
  }
  for (const auto& edge_key : sharp_edges) {
    const EdgeInfo& info = edge_map[edge_key];
    if (info.faces.size() != 2)
      continue;
    int segs = std::max(1, segments);
    float width_for_edge = bevel_width;
    if (clamp_overlap) {
      const Eigen::Vector3f& pos_a = (*positions_attr)[edge_key.point_index_a];
      const Eigen::Vector3f& pos_b = (*positions_attr)[edge_key.point_index_b];
      float edge_len = (pos_b - pos_a).norm();
      width_for_edge = std::min(width_for_edge, (HALF * edge_len) - CLAMP_EPS);
      width_for_edge = std::max(width_for_edge, 0.0F);
    }
    if (segs == 1) {
      add_chamfer_quad_for_edge(*result, result_positions, prim_normals, edge_key, info, width_for_edge);
      // Record endpoints for corner stitching
      if (corner_reuse != nullptr) {
        // Points were appended last 4; stash mapping per original corner per
        // face
        int p_base = static_cast<int>(result->point_count() - 4);
        corner_reuse->map[edge_key.point_index_a][info.faces[0]] = p_base + 0;
        corner_reuse->map[edge_key.point_index_b][info.faces[0]] = p_base + 1;
        corner_reuse->map[edge_key.point_index_b][info.faces[1]] = p_base + 2;
        corner_reuse->map[edge_key.point_index_a][info.faces[1]] = p_base + 3;
      }
    } else {
      size_t before_points = result->point_count();
      add_chamfer_strip_for_edge(*result, result_positions, positions_attr, prim_normals, edge_key, info, segs,
                                 width_for_edge, profile, clamp_overlap);
      if (corner_reuse != nullptr) {
        // First slice offsets: two points added at indices before_points and
        // before_points+1
        if (result->point_count() >= before_points + 2) {
          corner_reuse->map[edge_key.point_index_a][info.faces[0]] = static_cast<int>(before_points + 0);
          corner_reuse->map[edge_key.point_index_b][info.faces[0]] = static_cast<int>(before_points + 1);
        }
        // Last slice offsets: two points at end of strip
        size_t expected_points = static_cast<size_t>((segments + 1) * 2);
        if (result->point_count() >= before_points + expected_points) {
          size_t last_a = before_points + static_cast<size_t>((2 * segments) + 0);
          size_t last_b = before_points + static_cast<size_t>((2 * segments) + 1);
          corner_reuse->map[edge_key.point_index_a][info.faces[1]] = static_cast<int>(last_a);
          corner_reuse->map[edge_key.point_index_b][info.faces[1]] = static_cast<int>(last_b);
        }
      }
    }
  }
  recompute_primitive_normals(*result);
  return result;
}

static std::shared_ptr<core::GeometryContainer>
bevel_vertices(const core::GeometryContainer& input, float bevel_width, int segments, float angle_threshold,
               float profile, BevelSOP::CornerStyle corner_style, const CornerReuseMap* reuse = nullptr,
               const core::GeometryContainer* topology_source = nullptr) {
  std::cout << "\n=== Vertex Bevel Mode (multi-ring prototype) ===\n";
  if (bevel_width <= 0.0F || segments < 1)
    return std::make_shared<core::GeometryContainer>(input.clone());
  // Use separate topology source (original pre-edge geometry) when provided for
  // candidate analysis
  const core::GeometryContainer& analysis_geo = (topology_source != nullptr) ? *topology_source : input;
  if (!analysis_geo.has_point_attribute(core::standard_attrs::P)) {
    std::cerr << "Vertex Bevel: Missing position attribute P\n";
    return nullptr;
  }
  const auto* positions_attr = analysis_geo.get_point_attribute_typed<Eigen::Vector3f>(core::standard_attrs::P);
  if (positions_attr == nullptr) {
    std::cerr << "Vertex Bevel: Position attribute retrieval failed\n";
    return nullptr;
  }
  auto result = std::make_shared<core::GeometryContainer>(input.clone());
  result->ensure_position_attribute();
  auto* result_positions = result->get_point_attribute_typed<Eigen::Vector3f>(core::standard_attrs::P);
  if (result_positions == nullptr) {
    std::cerr << "Vertex Bevel: Failed to access result positions\n";
    return result;
  }
  std::vector<Eigen::Vector3f> prim_normals;
  std::vector<Eigen::Vector3f> prim_centroids;
  compute_prim_normals_centroids(analysis_geo, positions_attr, prim_normals, prim_centroids);
  std::map<EdgeKey, EdgeInfo> edge_map;
  build_edge_map(analysis_geo, edge_map);
  auto sharp_edges = find_sharp_edges(edge_map, prim_normals, angle_threshold);
  std::vector<std::vector<int>> point_faces(analysis_geo.point_count());
  std::vector<std::vector<int>> point_neighbors(analysis_geo.point_count());
  for (size_t prim_index = 0; prim_index < analysis_geo.primitive_count(); ++prim_index) {
    const auto& primitive_vertices = analysis_geo.topology().get_primitive_vertices(prim_index);
    for (int vertex_index : primitive_vertices) {
      int point_index = analysis_geo.topology().get_vertex_point(vertex_index);
      point_faces[point_index].push_back(static_cast<int>(prim_index));
    }
  }
  for (const auto& edge_entry : edge_map) {
    point_neighbors[edge_entry.first.point_index_a].push_back(edge_entry.first.point_index_b);
    point_neighbors[edge_entry.first.point_index_b].push_back(edge_entry.first.point_index_a);
  }
  std::set<int> points_to_bevel;
  for (const auto& edge_key : sharp_edges) {
    points_to_bevel.insert(edge_key.point_index_a);
    points_to_bevel.insert(edge_key.point_index_b);
  }
  // In combined EdgeVertex mode, restrict to original corner points if reuse
  // map provided
  if (reuse != nullptr && topology_source != nullptr) {
    std::set<int> restricted;
    for (const auto& entry : reuse->map) {
      restricted.insert(entry.first);
    }
    points_to_bevel = std::move(restricted);
  }
  std::cout << "Vertex Bevel: candidate points = " << points_to_bevel.size() << "\n";
  for (int point_index : points_to_bevel) {
    if (corner_style == BevelSOP::CornerStyle::RingStart) {
      add_vertex_bevel_patch_ringstart(*result, result_positions, positions_attr, prim_normals, point_index,
                                       point_faces[point_index], point_neighbors[point_index], bevel_width, segments,
                                       profile, reuse);
    } else if (corner_style == BevelSOP::CornerStyle::Grid) {
      add_vertex_bevel_patch_grid(*result, result_positions, positions_attr, prim_normals, point_index,
                                  point_faces[point_index], point_neighbors[point_index], bevel_width, segments,
                                  profile, reuse);
    } else { // default ApexFan
      add_vertex_bevel_patch_apexfan(*result, result_positions, positions_attr, prim_normals, point_index,
                                     point_faces[point_index], point_neighbors[point_index], bevel_width, segments,
                                     profile);
    }
  }
  recompute_primitive_normals(*result);
  return result;
}

static std::shared_ptr<core::GeometryContainer> bevel_faces(const core::GeometryContainer& input, float /*bevel_width*/,
                                                            int /*segments*/, float /*angle_threshold*/) {
  std::cout << "Face Bevel: not implemented yet, pass-through\n";
  return std::make_shared<core::GeometryContainer>(input.clone());
}

// BevelSOP implementation
BevelSOP::BevelSOP(const std::string& name) : SOPNode(name, "Bevel") {
  input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);
  register_parameter(define_float_parameter("width", BevelSOP::DEFAULT_WIDTH)
                         .label("Width")
                         .category("Bevel")
                         .description("Bevel width")
                         .range(0.0, MAX_WIDTH_RANGE)
                         .build());
  register_parameter(define_int_parameter("segments", BevelSOP::DEFAULT_SEGMENTS)
                         .label("Segments")
                         .category("Bevel")
                         .description("Rounded bevel segments")
                         .range(1, MAX_SEGMENTS_RANGE)
                         .build());
  register_parameter(define_float_parameter("profile", BevelSOP::DEFAULT_PROFILE)
                         .label("Profile")
                         .category("Bevel")
                         .description("Profile shape (0=soft,1=sharp)")
                         .range(0.0, 1.0)
                         .build());
  register_parameter(define_int_parameter("mode", static_cast<int>(BevelType::Edge))
                         .label("Mode")
                         .category("Bevel")
                         .description("Bevel mode")
                         .options({"Vertex", "Edge", "Face", "EdgeVertex"})
                         .build());
  register_parameter(define_int_parameter("corner_style", static_cast<int>(CornerStyle::ApexFan))
                         .label("Corner Style")
                         .category("Bevel")
                         .description("Vertex corner patch topology")
                         .options({"ApexFan", "RingStart", "Grid"})
                         .build());
  register_parameter(define_bool_parameter("clamp_overlap", true)
                         .label("Clamp Overlap")
                         .category("Bevel")
                         .description("Prevent self-overlap")
                         .build());
  register_parameter(define_float_parameter("angle_limit", DEFAULT_ANGLE_LIMIT)
                         .label("Angle Limit")
                         .category("Limits")
                         .description("Minimum dihedral angle (deg)")
                         .range(0.0, MAX_ANGLE_LIMIT)
                         .build());
}

core::Result<std::shared_ptr<core::GeometryContainer>> BevelSOP::execute() {
  // Apply group filter if specified (keeps only grouped primitives)
  auto filter_result = apply_group_filter(0, core::ElementClass::PRIMITIVE, false);
  if (!filter_result.is_success()) {
    return {"BevelSOP requires input geometry"};
  }
  const auto& input = filter_result.get_value();

  float bevel_width = get_parameter<float>("width", BevelSOP::DEFAULT_WIDTH);
  int segments = get_parameter<int>("segments", BevelSOP::DEFAULT_SEGMENTS);
  float angle_threshold = get_parameter<float>("angle_limit", DEFAULT_ANGLE_LIMIT);
  bool clamp_overlap = get_parameter<bool>("clamp_overlap", true);
  float profile = get_parameter<float>("profile", BevelSOP::DEFAULT_PROFILE);
  auto mode = static_cast<BevelType>(get_parameter<int>("mode", static_cast<int>(BevelType::Edge)));
  auto corner_style =
      static_cast<CornerStyle>(get_parameter<int>("corner_style", static_cast<int>(CornerStyle::ApexFan)));

  std::shared_ptr<core::GeometryContainer> result;
  switch (mode) {
    case BevelType::Vertex:
      result = bevel_vertices(*input, bevel_width, segments, angle_threshold, profile, corner_style, nullptr);
      break;
    case BevelType::Edge:
      result = bevel_edges(*input, bevel_width, segments, angle_threshold, profile, clamp_overlap);
      break;
    case BevelType::Face:
      result = bevel_faces(*input, bevel_width, segments, angle_threshold);
      break;
    case BevelType::EdgeVertex: {
      CornerReuseMap reuse;
      auto after_edges = bevel_edges(*input, bevel_width, segments, angle_threshold, profile, clamp_overlap, &reuse);
      if (!after_edges) {
        return {"Edge bevel failed during EdgeVertex operation"};
      }
      // For stitching, prefer RingStart so outer ring can reuse edge endpoints.
      auto effective_corner_style = (corner_style == CornerStyle::ApexFan) ? CornerStyle::RingStart : corner_style;
      // Restrict vertex bevel candidates to ORIGINAL corners (use 'input' as
      // analysis topology)
      result = bevel_vertices(*after_edges, bevel_width, segments, angle_threshold, profile, effective_corner_style,
                              &reuse, input.get());
      break;
    }
  }
  if (!result) {
    return {"Bevel operation failed"};
  }
  return result;
}

} // namespace nodo::sop
