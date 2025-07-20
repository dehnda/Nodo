#include "../../include/nodeflux/spatial/bvh.hpp"
#include <algorithm>
#include <chrono>
#include <limits>
#include <numeric>

namespace nodeflux::spatial {

// AABB Implementation
AABB::AABB(const Eigen::Vector3d &min_pt, const Eigen::Vector3d &max_pt)
    : min_point(min_pt), max_point(max_pt) {}

AABB AABB::from_points(const std::vector<Eigen::Vector3d> &points) {
  if (points.empty()) {
    return AABB{Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero()};
  }

  Eigen::Vector3d min_pt = points[0];
  Eigen::Vector3d max_pt = points[0];

  for (const auto &point : points) {
    min_pt = min_pt.cwiseMin(point);
    max_pt = max_pt.cwiseMax(point);
  }

  return AABB{min_pt, max_pt};
}

AABB AABB::from_mesh(const core::Mesh &mesh) {
  if (mesh.vertices().rows() == 0) {
    return AABB{Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero()};
  }

  Eigen::Vector3d min_pt = mesh.vertices().row(0);
  Eigen::Vector3d max_pt = mesh.vertices().row(0);

  for (int i = 1; i < mesh.vertices().rows(); ++i) {
    const Eigen::Vector3d vertex = mesh.vertices().row(i);
    min_pt = min_pt.cwiseMin(vertex);
    max_pt = max_pt.cwiseMax(vertex);
  }

  return AABB{min_pt, max_pt};
}

bool AABB::intersects(const AABB &other) const {
  return (min_point.x() <= other.max_point.x() &&
          max_point.x() >= other.min_point.x()) &&
         (min_point.y() <= other.max_point.y() &&
          max_point.y() >= other.min_point.y()) &&
         (min_point.z() <= other.max_point.z() &&
          max_point.z() >= other.min_point.z());
}

bool AABB::contains(const Eigen::Vector3d &point) const {
  return point.x() >= min_point.x() && point.x() <= max_point.x() &&
         point.y() >= min_point.y() && point.y() <= max_point.y() &&
         point.z() >= min_point.z() && point.z() <= max_point.z();
}

void AABB::expand(const AABB &other) {
  min_point = min_point.cwiseMin(other.min_point);
  max_point = max_point.cwiseMax(other.max_point);
}

void AABB::expand(const Eigen::Vector3d &point) {
  min_point = min_point.cwiseMin(point);
  max_point = max_point.cwiseMax(point);
}

Eigen::Vector3d AABB::center() const { return (min_point + max_point) * 0.5; }

Eigen::Vector3d AABB::size() const { return max_point - min_point; }

double AABB::surface_area() const {
  const Eigen::Vector3d extent = size();
  return 2.0 * (extent.x() * extent.y() + extent.y() * extent.z() +
                extent.z() * extent.x());
}

double AABB::volume() const {
  const Eigen::Vector3d extent = size();
  return extent.x() * extent.y() * extent.z();
}

bool AABB::is_valid() const {
  return min_point.x() <= max_point.x() && min_point.y() <= max_point.y() &&
         min_point.z() <= max_point.z();
}

// BVH Implementation
bool BVH::build(const core::Mesh &mesh, const BuildParams &params) {
  auto start_time = std::chrono::high_resolution_clock::now();

  mesh_ = &mesh;
  params_ = params;
  stats_ = Stats{};

  if (mesh.faces().rows() == 0) {
    root_ = nullptr;
    return false;
  }

  // Create list of all triangle indices
  std::vector<int> all_triangles(mesh.faces().rows());
  std::iota(all_triangles.begin(), all_triangles.end(), 0);

  // Calculate root bounding box
  AABB root_bounds = AABB::from_mesh(mesh);

  // Build BVH recursively
  root_ = build_recursive(all_triangles, root_bounds, 0);

  auto end_time = std::chrono::high_resolution_clock::now();
  stats_.build_time_ms =
      std::chrono::duration<double, std::milli>(end_time - start_time).count();

  return root_ != nullptr;
}

std::unique_ptr<BVHNode>
BVH::build_recursive(const std::vector<int> &triangle_indices,
                     const AABB &node_bounds, int depth) {
  auto node = std::make_unique<BVHNode>(node_bounds);
  stats_.total_nodes++;
  stats_.max_depth = std::max(stats_.max_depth, depth);

  // Check termination criteria
  if (triangle_indices.size() <=
          static_cast<size_t>(params_.max_triangles_per_leaf) ||
      depth >= params_.max_depth) {

    node->is_leaf = true;
    node->triangle_indices = triangle_indices;
    stats_.leaf_nodes++;
    return node;
  }

  // Split triangles
  auto [left_triangles, right_triangles] =
      params_.use_sah ? split_triangles_sah(triangle_indices, node_bounds)
                      : split_triangles_midpoint(triangle_indices, node_bounds);

  // If split failed, make this a leaf
  if (left_triangles.empty() || right_triangles.empty()) {
    node->is_leaf = true;
    node->triangle_indices = triangle_indices;
    stats_.leaf_nodes++;
    return node;
  }

  // Build child nodes
  AABB left_bounds = calculate_triangle_bounds(left_triangles);
  AABB right_bounds = calculate_triangle_bounds(right_triangles);

  node->left_child = build_recursive(left_triangles, left_bounds, depth + 1);
  node->right_child = build_recursive(right_triangles, right_bounds, depth + 1);

  return node;
}

AABB BVH::calculate_triangle_bounds(
    const std::vector<int> &triangle_indices) const {
  if (triangle_indices.empty()) {
    return AABB{Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero()};
  }

  // Get first triangle's first vertex
  int first_triangle = triangle_indices[0];
  int v0_idx = mesh_->faces()(first_triangle, 0);
  Eigen::Vector3d min_pt = mesh_->vertices().row(v0_idx);
  Eigen::Vector3d max_pt = min_pt;

  // Expand to include all triangle vertices
  for (int tri_idx : triangle_indices) {
    for (int vertex_idx = 0; vertex_idx < 3; ++vertex_idx) {
      int v_idx = mesh_->faces()(tri_idx, vertex_idx);
      const Eigen::Vector3d vertex = mesh_->vertices().row(v_idx);
      min_pt = min_pt.cwiseMin(vertex);
      max_pt = max_pt.cwiseMax(vertex);
    }
  }

  return AABB{min_pt, max_pt};
}

std::pair<std::vector<int>, std::vector<int>>
BVH::split_triangles_sah(const std::vector<int> &triangle_indices,
                         const AABB &node_bounds) const {
  const int num_buckets = 12;
  double best_cost = std::numeric_limits<double>::infinity();
  int best_split_axis = 0;
  int best_split_bucket = 0;

  // Try each axis
  for (int axis = 0; axis < 3; ++axis) {
    // Calculate triangle centroids for this axis
    std::vector<std::pair<double, int>> centroids;
    centroids.reserve(triangle_indices.size());

    for (int tri_idx : triangle_indices) {
      Eigen::Vector3d centroid = Eigen::Vector3d::Zero();
      for (int vertex_idx = 0; vertex_idx < 3; ++vertex_idx) {
        int v_idx = mesh_->faces()(tri_idx, vertex_idx);
        centroid += mesh_->vertices().row(v_idx).transpose();
      }
      centroid /= 3.0;
      centroids.emplace_back(centroid[axis], tri_idx);
    }

    // Sort by centroid position along axis
    std::sort(centroids.begin(), centroids.end());

    // Try different split positions
    for (int bucket = 1; bucket < num_buckets; ++bucket) {
      const size_t split_pos = (centroids.size() * bucket) / num_buckets;

      // Calculate cost using surface area heuristic
      const double left_count = static_cast<double>(split_pos);
      const double right_count =
          static_cast<double>(centroids.size() - split_pos);

      // Simple cost estimation (can be improved with actual surface areas)
      const double cost = left_count + right_count;

      if (cost < best_cost) {
        best_cost = cost;
        best_split_axis = axis;
        best_split_bucket = bucket;
      }
    }
  }

  // Perform the best split
  std::vector<std::pair<double, int>> centroids;
  centroids.reserve(triangle_indices.size());

  for (int tri_idx : triangle_indices) {
    Eigen::Vector3d centroid = Eigen::Vector3d::Zero();
    for (int vertex_idx = 0; vertex_idx < 3; ++vertex_idx) {
      int v_idx = mesh_->faces()(tri_idx, vertex_idx);
      centroid += mesh_->vertices().row(v_idx).transpose();
    }
    centroid /= 3.0;
    centroids.emplace_back(centroid[best_split_axis], tri_idx);
  }

  std::sort(centroids.begin(), centroids.end());

  const size_t split_pos = (centroids.size() * best_split_bucket) / num_buckets;

  std::vector<int> left_triangles, right_triangles;
  left_triangles.reserve(split_pos);
  right_triangles.reserve(centroids.size() - split_pos);

  for (size_t i = 0; i < split_pos; ++i) {
    left_triangles.push_back(centroids[i].second);
  }
  for (size_t i = split_pos; i < centroids.size(); ++i) {
    right_triangles.push_back(centroids[i].second);
  }

  return {left_triangles, right_triangles};
}

std::pair<std::vector<int>, std::vector<int>>
BVH::split_triangles_midpoint(const std::vector<int> &triangle_indices,
                              const AABB &node_bounds) const {
  // Find longest axis
  Eigen::Vector3d extent = node_bounds.size();
  int longest_axis = 0;
  if (extent.y() > extent.x())
    longest_axis = 1;
  if (extent.z() > extent[longest_axis])
    longest_axis = 2;

  double split_pos = node_bounds.center()[longest_axis];

  std::vector<int> left_triangles, right_triangles;

  for (int tri_idx : triangle_indices) {
    // Calculate triangle centroid
    Eigen::Vector3d centroid = Eigen::Vector3d::Zero();
    for (int vertex_idx = 0; vertex_idx < 3; ++vertex_idx) {
      int v_idx = mesh_->faces()(tri_idx, vertex_idx);
      centroid += mesh_->vertices().row(v_idx).transpose();
    }
    centroid /= 3.0;

    if (centroid[longest_axis] < split_pos) {
      left_triangles.push_back(tri_idx);
    } else {
      right_triangles.push_back(tri_idx);
    }
  }

  return {left_triangles, right_triangles};
}

std::optional<BVH::RayHit> BVH::intersect_ray(const Ray &ray) const {
  if (!root_)
    return std::nullopt;

  RayHit closest_hit;
  if (intersect_ray_recursive(root_.get(), ray, closest_hit)) {
    return closest_hit;
  }

  return std::nullopt;
}

bool BVH::ray_aabb_intersect(const Ray &ray, const AABB &aabb, double &t_near,
                             double &t_far) const {
  t_near = ray.t_min;
  t_far = ray.t_max;

  for (int axis = 0; axis < 3; ++axis) {
    if (std::abs(ray.direction[axis]) < 1e-9) {
      // Ray is parallel to slab
      if (ray.origin[axis] < aabb.min_point[axis] ||
          ray.origin[axis] > aabb.max_point[axis]) {
        return false;
      }
    } else {
      double inv_dir = 1.0 / ray.direction[axis];
      double t1 = (aabb.min_point[axis] - ray.origin[axis]) * inv_dir;
      double t2 = (aabb.max_point[axis] - ray.origin[axis]) * inv_dir;

      if (t1 > t2)
        std::swap(t1, t2);

      t_near = std::max(t_near, t1);
      t_far = std::min(t_far, t2);

      if (t_near > t_far)
        return false;
    }
  }

  return t_far >= 0.0;
}

bool BVH::ray_triangle_intersect(const Ray &ray, int triangle_index,
                                 RayHit &hit) const {
  // Get triangle vertices
  const auto &face = mesh_->faces().row(triangle_index);
  Eigen::Vector3d v0 = mesh_->vertices().row(face[0]);
  Eigen::Vector3d v1 = mesh_->vertices().row(face[1]);
  Eigen::Vector3d v2 = mesh_->vertices().row(face[2]);

  // Möller-Trumbore algorithm
  Eigen::Vector3d edge1 = v1 - v0;
  Eigen::Vector3d edge2 = v2 - v0;
  Eigen::Vector3d h = ray.direction.cross(edge2);
  double det = edge1.dot(h);

  const double epsilon = 1e-9;
  if (std::abs(det) < epsilon)
    return false; // Ray parallel to triangle

  double inv_det = 1.0 / det;
  Eigen::Vector3d distance = ray.origin - v0;
  double u_coord = distance.dot(h) * inv_det;

  if (u_coord < 0.0 || u_coord > 1.0)
    return false;

  Eigen::Vector3d q_vec = distance.cross(edge1);
  double v_coord = ray.direction.dot(q_vec) * inv_det;

  if (v_coord < 0.0 || u_coord + v_coord > 1.0)
    return false;

  double t_val = edge2.dot(q_vec) * inv_det;

  if (t_val > epsilon && t_val < hit.t) {
    hit.t = t_val;
    hit.triangle_index = triangle_index;
    hit.point = ray.origin + t_val * ray.direction;
    hit.normal = edge1.cross(edge2).normalized();
    hit.barycentric = Eigen::Vector2d(u_coord, v_coord);
    return true;
  }

  return false;
}

bool BVH::intersect_ray_recursive(const BVHNode *node, const Ray &ray,
                                  RayHit &closest_hit) const {
  if (!node)
    return false;

  double t_near, t_far;
  if (!ray_aabb_intersect(ray, node->bounding_box, t_near, t_far)) {
    return false;
  }

  if (node->is_leaf) {
    bool hit_any = false;
    for (int tri_idx : node->triangle_indices) {
      if (ray_triangle_intersect(ray, tri_idx, closest_hit)) {
        hit_any = true;
      }
    }
    return hit_any;
  }

  bool hit_left =
      intersect_ray_recursive(node->left_child.get(), ray, closest_hit);
  bool hit_right =
      intersect_ray_recursive(node->right_child.get(), ray, closest_hit);

  return hit_left || hit_right;
}

std::vector<int> BVH::query_aabb(const AABB &aabb) const {
  std::vector<int> results;
  if (root_) {
    query_aabb_recursive(root_.get(), aabb, results);
  }
  return results;
}

void BVH::query_aabb_recursive(const BVHNode *node, const AABB &query_aabb,
                               std::vector<int> &results) const {
  if (!node || !node->bounding_box.intersects(query_aabb)) {
    return;
  }

  if (node->is_leaf) {
    results.insert(results.end(), node->triangle_indices.begin(),
                   node->triangle_indices.end());
    return;
  }

  query_aabb_recursive(node->left_child.get(), query_aabb, results);
  query_aabb_recursive(node->right_child.get(), query_aabb, results);
}

std::optional<std::pair<Eigen::Vector3d, int>>
BVH::closest_point(const Eigen::Vector3d &point) const {
  // This is a simplified implementation - could be optimized further
  if (!root_ || mesh_->faces().rows() == 0) {
    return std::nullopt;
  }

  double min_distance = std::numeric_limits<double>::infinity();
  Eigen::Vector3d closest_pt;
  int closest_triangle = -1;

  // Query all triangles (could be optimized with spatial search)
  for (int tri_idx = 0; tri_idx < mesh_->faces().rows(); ++tri_idx) {
    const auto &face = mesh_->faces().row(tri_idx);
    Eigen::Vector3d v0 = mesh_->vertices().row(face[0]);
    Eigen::Vector3d v1 = mesh_->vertices().row(face[1]);
    Eigen::Vector3d v2 = mesh_->vertices().row(face[2]);

    // Simple point-to-triangle distance (can be improved)
    Eigen::Vector3d centroid = (v0 + v1 + v2) / 3.0;
    double distance = (point - centroid).norm();

    if (distance < min_distance) {
      min_distance = distance;
      closest_pt = centroid;
      closest_triangle = tri_idx;
    }
  }

  return std::make_pair(closest_pt, closest_triangle);
}

void BVH::clear() {
  root_.reset();
  mesh_ = nullptr;
  stats_ = Stats{};
}

} // namespace nodeflux::spatial
