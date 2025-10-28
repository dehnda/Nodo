#include "nodo/core/element_topology.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace nodo::core {

void ElementTopology::set_point_count(size_t count) { point_count_ = count; }

void ElementTopology::set_vertex_count(size_t count) {
  vertex_count_ = count;
  vertex_points_.resize(count, -1); // -1 = unassigned
}

void ElementTopology::set_primitive_count(size_t count) {
  primitive_count_ = count;
  primitive_vertices_.resize(count);
}

void ElementTopology::reserve_vertices(size_t capacity) {
  vertex_points_.reserve(capacity);
}

void ElementTopology::reserve_primitives(size_t capacity) {
  primitive_vertices_.reserve(capacity);
}

int ElementTopology::get_vertex_point(size_t vertex_idx) const {
  if (vertex_idx >= vertex_count_) {
    throw std::out_of_range("Vertex index out of range");
  }
  return vertex_points_[vertex_idx];
}

void ElementTopology::set_vertex_point(size_t vertex_idx, int point_idx) {
  if (vertex_idx >= vertex_count_) {
    throw std::out_of_range("Vertex index out of range");
  }
  if (point_idx >= static_cast<int>(point_count_) && point_idx != -1) {
    throw std::out_of_range("Point index out of range");
  }
  vertex_points_[vertex_idx] = point_idx;
}

std::span<const int> ElementTopology::get_vertex_points() const {
  return vertex_points_;
}

std::span<int> ElementTopology::get_vertex_points_writable() {
  return vertex_points_;
}

const std::vector<int> &
ElementTopology::get_primitive_vertices(size_t prim_idx) const {
  if (prim_idx >= primitive_count_) {
    throw std::out_of_range("Primitive index out of range");
  }
  return primitive_vertices_[prim_idx];
}

void ElementTopology::set_primitive_vertices(
    size_t prim_idx, const std::vector<int> &vertices) {
  if (prim_idx >= primitive_count_) {
    throw std::out_of_range("Primitive index out of range");
  }

  // Validate that all vertices are in range
  for (int v : vertices) {
    if (v >= static_cast<int>(vertex_count_) || v < 0) {
      throw std::out_of_range("Vertex index in primitive out of range");
    }
  }

  primitive_vertices_[prim_idx] = vertices;
}

size_t ElementTopology::add_primitive(const std::vector<int> &vertices) {
  // Validate vertices
  for (int v : vertices) {
    if (v >= static_cast<int>(vertex_count_) || v < 0) {
      throw std::out_of_range("Vertex index in primitive out of range");
    }
  }

  primitive_vertices_.push_back(vertices);
  primitive_count_ = primitive_vertices_.size();
  return primitive_count_ - 1;
}

size_t ElementTopology::get_primitive_vertex_count(size_t prim_idx) const {
  if (prim_idx >= primitive_count_) {
    throw std::out_of_range("Primitive index out of range");
  }
  return primitive_vertices_[prim_idx].size();
}

bool ElementTopology::validate() const {
  // Check vertex→point references
  for (size_t i = 0; i < vertex_count_; ++i) {
    int point_idx = vertex_points_[i];
    if (point_idx != -1 &&
        (point_idx < 0 || point_idx >= static_cast<int>(point_count_))) {
      return false;
    }
  }

  // Check primitive→vertex references
  for (size_t i = 0; i < primitive_count_; ++i) {
    const auto &verts = primitive_vertices_[i];
    if (verts.empty()) {
      return false; // Primitives must have at least one vertex
    }
    for (int v : verts) {
      if (v < 0 || v >= static_cast<int>(vertex_count_)) {
        return false;
      }
    }
  }

  return true;
}

void ElementTopology::clear() {
  point_count_ = 0;
  vertex_count_ = 0;
  primitive_count_ = 0;
  vertex_points_.clear();
  primitive_vertices_.clear();
}

ElementTopology::Stats ElementTopology::compute_stats() const {
  Stats stats;
  stats.points = point_count_;
  stats.vertices = vertex_count_;
  stats.primitives = primitive_count_;

  if (primitive_count_ == 0) {
    stats.min_prim_verts = 0;
    stats.max_prim_verts = 0;
    stats.avg_prim_verts = 0.0;
  } else {
    stats.min_prim_verts = std::numeric_limits<size_t>::max();
    stats.max_prim_verts = 0;
    size_t total_verts = 0;

    for (size_t i = 0; i < primitive_count_; ++i) {
      size_t vert_count = primitive_vertices_[i].size();
      stats.min_prim_verts = std::min(stats.min_prim_verts, vert_count);
      stats.max_prim_verts = std::max(stats.max_prim_verts, vert_count);
      total_verts += vert_count;
    }

    stats.avg_prim_verts = static_cast<double>(total_verts) / primitive_count_;
  }

  return stats;
}

} // namespace nodo::core
