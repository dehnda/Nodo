#include "../../include/nodeflux/geometry/sphere_generator.hpp"
#include <cmath>
#include <map>
#include <numbers>

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::geometry {

// Thread-local storage for error reporting
thread_local core::Error SphereGenerator::last_error_{
    core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"};

// Constants for default parameters
constexpr int DEFAULT_U_SEGMENTS = 32;
constexpr int DEFAULT_V_SEGMENTS = 16;
constexpr int DEFAULT_SUBDIVISIONS = 2;
constexpr int MAX_SUBDIVISIONS = 6;

std::optional<core::GeometryContainer>
SphereGenerator::generate_uv_sphere(double radius, int u_segments,
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

  // Calculate number of vertices and faces
  const int num_vertices = ((v_segments - 1) * u_segments) + 2; // +2 for poles
  const int num_faces = 2 * u_segments * (v_segments - 1);

  // Create GeometryContainer
  core::GeometryContainer container;

  // Set up topology
  container.set_point_count(num_vertices);
  container.set_vertex_count(num_vertices); // 1:1 mapping for UV sphere

  auto &topology = container.topology();

  // Build primitive vertex lists (3 vertices per triangle)
  std::vector<std::vector<int>> primitive_vertices;
  primitive_vertices.reserve(num_faces);

  // Temporary storage for positions and normals (we'll calculate both)
  std::vector<core::Vec3f> positions;
  positions.reserve(num_vertices);

  // Generate vertex positions
  // Top pole
  positions.push_back({0.0F, static_cast<float>(radius), 0.0F});

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

      positions.push_back({static_cast<float>(coord_x),
                           static_cast<float>(coord_y),
                           static_cast<float>(coord_z)});
    }
  }

  // Bottom pole
  positions.push_back({0.0F, static_cast<float>(-radius), 0.0F});

  // Generate faces
  // Top cap faces
  for (int segment = 0; segment < u_segments; ++segment) {
    const int next_segment = (segment + 1) % u_segments;
    primitive_vertices.push_back({0, 1 + segment, 1 + next_segment});
  }

  // Middle faces
  for (int ring = 0; ring < v_segments - 2; ++ring) {
    for (int segment = 0; segment < u_segments; ++segment) {
      const int next_segment = (segment + 1) % u_segments;
      const int current_ring = 1 + (ring * u_segments);
      const int next_ring = 1 + ((ring + 1) * u_segments);

      // First triangle
      primitive_vertices.push_back({current_ring + segment, next_ring + segment,
                                    current_ring + next_segment});

      // Second triangle
      primitive_vertices.push_back({current_ring + next_segment,
                                    next_ring + segment,
                                    next_ring + next_segment});
    }
  }

  // Bottom cap faces
  const int bottom_pole = num_vertices - 1;
  const int last_ring = 1 + ((v_segments - 2) * u_segments);
  for (int segment = 0; segment < u_segments; ++segment) {
    const int next_segment = (segment + 1) % u_segments;
    primitive_vertices.push_back(
        {bottom_pole, last_ring + next_segment, last_ring + segment});
  }

  // Update counts to actual size
  const size_t actual_point_count = positions.size();
  container.set_point_count(actual_point_count);
  container.set_vertex_count(actual_point_count); // 1:1 mapping

  // Set up 1:1 vertex→point mapping
  for (size_t i = 0; i < actual_point_count; ++i) {
    topology.set_vertex_point(i, static_cast<int>(i));
  }

  // Set topology primitives
  for (const auto &prim_verts : primitive_vertices) {
    topology.add_primitive(prim_verts);
  }

  // Create P (position) attribute
  container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage != nullptr) {
    auto p_span = p_storage->values_writable();
    std::copy(positions.begin(), positions.end(), p_span.begin());
  }

  // Create N (normal) attribute - for spheres, normals point from center
  container.add_point_attribute(attrs::N, core::AttributeType::VEC3F);
  auto *n_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::N);
  if (n_storage != nullptr) {
    auto n_span = n_storage->values_writable();
    for (size_t i = 0; i < positions.size(); ++i) {
      // Normalize position to get normal (sphere centered at origin)
      core::Vec3f normal = positions[i];
      const float length =
          std::sqrt((normal[0] * normal[0]) + (normal[1] * normal[1]) +
                    (normal[2] * normal[2]));
      if (length > 0.0F) {
        normal[0] /= length;
        normal[1] /= length;
        normal[2] /= length;
      }
      n_span[i] = normal;
    }
  }

  return container;
}

std::optional<core::GeometryContainer>
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

  // Scale to desired radius and convert to GeometryContainer
  core::GeometryContainer container;
  container.set_point_count(vertices.size());
  container.set_vertex_count(vertices.size()); // 1:1 mapping for icosphere

  auto &topology = container.topology();

  // Set up 1:1 vertex→point mapping
  for (size_t i = 0; i < vertices.size(); ++i) {
    topology.set_vertex_point(i, static_cast<int>(i));
  }

  // Add primitives
  for (const auto &face : faces) {
    topology.add_primitive({face[0], face[1], face[2]});
  }

  // Create position attribute
  container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage != nullptr) {
    auto p_span = p_storage->values_writable();
    for (size_t i = 0; i < vertices.size(); ++i) {
      const auto scaled_vertex = vertices[i] * radius;
      p_span[i] = {static_cast<float>(scaled_vertex.x()),
                   static_cast<float>(scaled_vertex.y()),
                   static_cast<float>(scaled_vertex.z())};
    }
  }

  // Create normal attribute (for icosphere, normals are normalized positions)
  container.add_point_attribute(attrs::N, core::AttributeType::VEC3F);
  auto *n_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::N);
  if (n_storage != nullptr) {
    auto n_span = n_storage->values_writable();
    for (size_t i = 0; i < vertices.size(); ++i) {
      // vertices are already normalized unit vectors
      n_span[i] = {static_cast<float>(vertices[i].x()),
                   static_cast<float>(vertices[i].y()),
                   static_cast<float>(vertices[i].z())};
    }
  }

  return container;
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
