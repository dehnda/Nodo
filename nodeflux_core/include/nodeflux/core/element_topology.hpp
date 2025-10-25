#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace nodeflux::core {

/**
 * @brief Element topology model for geometry
 *
 * Represents the fundamental topology structure of geometry with three levels:
 * - Points: Unique positions in space (shared by vertices)
 * - Vertices: Corners of primitives, reference points, can have unique attributes
 * - Primitives: Faces/polygons defined by ordered vertex lists
 *
 * This separation allows:
 * - Split normals/UVs (vertex attributes differ even for same point)
 * - N-gon support (primitives with variable vertex counts)
 * - Efficient attribute storage and interpolation
 */
class ElementTopology {
public:
  ElementTopology() = default;
  ~ElementTopology() = default;

  // Element counts
  size_t point_count() const { return point_count_; }
  size_t vertex_count() const { return vertex_count_; }
  size_t primitive_count() const { return primitive_count_; }

  /**
   * @brief Set element counts and allocate storage
   */
  void set_point_count(size_t count);
  void set_vertex_count(size_t count);
  void set_primitive_count(size_t count);

  /**
   * @brief Reserve capacity to avoid reallocation
   */
  void reserve_vertices(size_t capacity);
  void reserve_primitives(size_t capacity);

  // Vertex → Point mapping (which point does this vertex reference?)
  int get_vertex_point(size_t vertex_idx) const;
  void set_vertex_point(size_t vertex_idx, int point_idx);

  /**
   * @brief Get all vertex→point mappings as a span
   * @return Span of point indices (length = vertex_count)
   */
  std::span<const int> get_vertex_points() const;
  std::span<int> get_vertex_points_writable();

  // Primitive → Vertex mapping (which vertices form this primitive?)
  const std::vector<int> &get_primitive_vertices(size_t prim_idx) const;
  void set_primitive_vertices(size_t prim_idx, const std::vector<int> &vertices);

  /**
   * @brief Add a new primitive with the given vertices
   * @param vertices Ordered list of vertex indices forming this primitive
   * @return Index of the newly added primitive
   */
  size_t add_primitive(const std::vector<int> &vertices);

  /**
   * @brief Get vertex count for a specific primitive
   */
  size_t get_primitive_vertex_count(size_t prim_idx) const;

  /**
   * @brief Check if topology is valid
   * @return true if all vertex→point references are in range and all primitives
   * are valid
   */
  bool validate() const;

  /**
   * @brief Clear all topology data
   */
  void clear();

  /**
   * @brief Compute statistics for debugging
   */
  struct Stats {
    size_t points;
    size_t vertices;
    size_t primitives;
    size_t min_prim_verts;
    size_t max_prim_verts;
    double avg_prim_verts;
  };
  Stats compute_stats() const;

private:
  size_t point_count_ = 0;
  size_t vertex_count_ = 0;
  size_t primitive_count_ = 0;

  // Vertex → Point mapping (vertex_points_[vertex_idx] = point_idx)
  std::vector<int> vertex_points_;

  // Primitive → Vertex mapping (variable-length, supports N-gons)
  // primitive_vertices_[prim_idx] = [v0, v1, v2, ...] (ordered CCW)
  std::vector<std::vector<int>> primitive_vertices_;
};

} // namespace nodeflux::core
