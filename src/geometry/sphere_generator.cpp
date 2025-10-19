#include "../../include/nodeflux/geometry/sphere_generator.hpp"
#include <cmath>
#include <map>
#include <numbers>

namespace nodeflux::geometry {

// Thread-local storage for error reporting
thread_local core::Error SphereGenerator::last_error_{
    core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"};

// Constants for default parameters
constexpr int DEFAULT_U_SEGMENTS = 32;
constexpr int DEFAULT_V_SEGMENTS = 16;
constexpr int DEFAULT_SUBDIVISIONS = 2;
constexpr int MAX_SUBDIVISIONS = 6;

std::optional<core::Mesh> SphereGenerator::generate_uv_sphere(double radius,
                                                              int u_segments,
                                                              int v_segments) {

  if (radius <= 0.0) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidFormat,
                               "Sphere radius must be positive"});
    return std::nullopt;
  }

  if (u_segments < 3 || v_segments < 2) {
    set_last_error(core::Error{
        core::ErrorCategory::Validation, core::ErrorCode::InvalidFormat,
        "UV sphere requires at least 3 u_segments and 2 v_segments"});
    return std::nullopt;
  }

  core::Mesh mesh;

  // Calculate number of vertices and faces
  const int num_vertices = (v_segments - 1) * u_segments + 2; // +2 for poles
  const int num_faces = 2 * u_segments * (v_segments - 1);

  mesh.vertices().resize(num_vertices, 3);
  mesh.faces().resize(num_faces, 3);

  // Generate vertices
  int vertex_index = 0;

  // Top pole
  mesh.vertices()(vertex_index++, 0) = 0.0;
  mesh.vertices()(vertex_index - 1, 1) = radius;
  mesh.vertices()(vertex_index - 1, 2) = 0.0;

  // Middle rings
  for (int ring = 1; ring < v_segments; ++ring) {
    const double phi = std::numbers::pi * static_cast<double>(ring) /
                       static_cast<double>(v_segments);
    const double coord_y = radius * std::cos(phi);
    const double ring_radius = radius * std::sin(phi);

    for (int segment = 0; segment < u_segments; ++segment) {
      const double theta = 2.0 * std::numbers::pi *
                           static_cast<double>(segment) /
                           static_cast<double>(u_segments);
      const double coord_x = ring_radius * std::cos(theta);
      const double coord_z = ring_radius * std::sin(theta);

      mesh.vertices()(vertex_index, 0) = coord_x;
      mesh.vertices()(vertex_index, 1) = coord_y;
      mesh.vertices()(vertex_index, 2) = coord_z;
      ++vertex_index;
    }
  }

  // Bottom pole
  mesh.vertices()(vertex_index, 0) = 0.0;
  mesh.vertices()(vertex_index, 1) = -radius;
  mesh.vertices()(vertex_index, 2) = 0.0;

  // Generate faces
  int face_index = 0;

  // Top cap faces
  for (int segment = 0; segment < u_segments; ++segment) {
    const int next_segment = (segment + 1) % u_segments;
    mesh.faces()(face_index, 0) = 0; // Top pole
    mesh.faces()(face_index, 1) = 1 + segment;
    mesh.faces()(face_index, 2) = 1 + next_segment;
    ++face_index;
  }

  // Middle faces
  for (int ring = 0; ring < v_segments - 2; ++ring) {
    for (int segment = 0; segment < u_segments; ++segment) {
      const int next_segment = (segment + 1) % u_segments;
      const int current_ring = 1 + ring * u_segments;
      const int next_ring = 1 + (ring + 1) * u_segments;

      // First triangle
      mesh.faces()(face_index, 0) = current_ring + segment;
      mesh.faces()(face_index, 1) = next_ring + segment;
      mesh.faces()(face_index, 2) = current_ring + next_segment;
      ++face_index;

      // Second triangle
      mesh.faces()(face_index, 0) = current_ring + next_segment;
      mesh.faces()(face_index, 1) = next_ring + segment;
      mesh.faces()(face_index, 2) = next_ring + next_segment;
      ++face_index;
    }
  }

  // Bottom cap faces
  const int bottom_pole = num_vertices - 1;
  const int last_ring = 1 + (v_segments - 2) * u_segments;
  for (int segment = 0; segment < u_segments; ++segment) {
    const int next_segment = (segment + 1) % u_segments;
    mesh.faces()(face_index, 0) = bottom_pole; // Bottom pole
    mesh.faces()(face_index, 1) = last_ring + next_segment;
    mesh.faces()(face_index, 2) = last_ring + segment;
    ++face_index;
  }

  return mesh;
}

std::optional<core::Mesh>
SphereGenerator::generate_icosphere(double radius, int subdivisions) {

  if (radius <= 0.0) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidFormat,
                               "Sphere radius must be positive"});
    return std::nullopt;
  }

  if (subdivisions < 0 || subdivisions > MAX_SUBDIVISIONS) {
    set_last_error(core::Error{
        core::ErrorCategory::Validation, core::ErrorCode::InvalidFormat,
        "Icosphere subdivisions must be between 0 and 6"});
    return std::nullopt;
  }

  // Golden ratio for icosahedron construction
  constexpr double golden_ratio = 1.618033988749895;
  const double icosa_scale = 1.0 / std::sqrt(golden_ratio * golden_ratio + 1.0);

  // Initial icosahedron vertices - these are magic numbers for the icosahedron
  std::vector<Eigen::Vector3d> vertices = {
      {-icosa_scale, golden_ratio * icosa_scale, 0},
      {icosa_scale, golden_ratio * icosa_scale, 0},
      {-icosa_scale, -golden_ratio * icosa_scale, 0},
      {icosa_scale, -golden_ratio * icosa_scale, 0},
      {0, -icosa_scale, golden_ratio * icosa_scale},
      {0, icosa_scale, golden_ratio * icosa_scale},
      {0, -icosa_scale, -golden_ratio * icosa_scale},
      {0, icosa_scale, -golden_ratio * icosa_scale},
      {golden_ratio * icosa_scale, 0, -icosa_scale},
      {golden_ratio * icosa_scale, 0, icosa_scale},
      {-golden_ratio * icosa_scale, 0, -icosa_scale},
      {-golden_ratio * icosa_scale, 0, icosa_scale}};

  // Initial icosahedron faces - these indices define the icosahedron topology
  std::vector<std::array<int, 3>> faces = {
      {0, 11, 5}, {0, 5, 1},  {0, 1, 7},   {0, 7, 10}, {0, 10, 11},
      {1, 5, 9},  {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
      {3, 9, 4},  {3, 4, 2},  {3, 2, 6},   {3, 6, 8},  {3, 8, 9},
      {4, 9, 5},  {2, 4, 11}, {6, 2, 10},  {8, 6, 7},  {9, 8, 1}};

  // Normalize initial vertices to unit sphere
  for (auto &vertex : vertices) {
    vertex = normalize_vertex(vertex, 1.0);
  }

  // Subdivide
  for (int level = 0; level < subdivisions; ++level) {
    std::vector<std::array<int, 3>> new_faces;
    new_faces.reserve(faces.size() * 4);

    std::map<std::pair<int, int>, int> edge_vertex_map;

    for (const auto &face : faces) {
      // Get or create midpoint vertices
      auto get_midpoint = [&](int vertex1, int vertex2) -> int {
        if (vertex1 > vertex2)
          std::swap(vertex1, vertex2);
        auto key = std::make_pair(vertex1, vertex2);

        auto it = edge_vertex_map.find(key);
        if (it != edge_vertex_map.end()) {
          return it->second;
        }

        // Create new vertex at midpoint
        Eigen::Vector3d midpoint =
            (vertices[vertex1] + vertices[vertex2]) * 0.5;
        midpoint = normalize_vertex(midpoint, 1.0);

        int new_index = static_cast<int>(vertices.size());
        vertices.push_back(midpoint);
        edge_vertex_map[key] = new_index;
        return new_index;
      };

      int mid01 = get_midpoint(face[0], face[1]);
      int mid12 = get_midpoint(face[1], face[2]);
      int mid20 = get_midpoint(face[2], face[0]);

      // Create 4 new faces
      new_faces.push_back({face[0], mid01, mid20});
      new_faces.push_back({face[1], mid12, mid01});
      new_faces.push_back({face[2], mid20, mid12});
      new_faces.push_back({mid01, mid12, mid20});
    }

    faces = std::move(new_faces);
  }

  // Scale to desired radius and convert to mesh
  core::Mesh mesh;
  mesh.vertices().resize(vertices.size(), 3);
  mesh.faces().resize(faces.size(), 3);

  for (size_t i = 0; i < vertices.size(); ++i) {
    const auto scaled_vertex = vertices[i] * radius;
    mesh.vertices()(i, 0) = scaled_vertex.x();
    mesh.vertices()(i, 1) = scaled_vertex.y();
    mesh.vertices()(i, 2) = scaled_vertex.z();
  }

  for (size_t i = 0; i < faces.size(); ++i) {
    mesh.faces()(i, 0) = faces[i][0];
    mesh.faces()(i, 1) = faces[i][1];
    mesh.faces()(i, 2) = faces[i][2];
  }

  return mesh;
}

const core::Error &SphereGenerator::last_error() { return last_error_; }

void SphereGenerator::set_last_error(const core::Error &error) {
  last_error_ = error;
}

Eigen::Vector3d SphereGenerator::normalize_vertex(const Eigen::Vector3d &vertex,
                                                  double radius) {
  return vertex.normalized() * radius;
}

} // namespace nodeflux::geometry
