#include "nodo/geometry/mesh_generator.hpp"

#include "nodo/core/math.hpp"
#include "nodo/geometry/sphere_generator.hpp"

#include <array>
#include <cmath>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::geometry {

// Thread-local storage for error reporting
thread_local core::Error MeshGenerator::last_error_{
    core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"};

core::GeometryContainer MeshGenerator::box(const Eigen::Vector3d& min_corner,
                                           const Eigen::Vector3d& max_corner) {
  core::GeometryContainer container;

  // Set up topology - 8 vertices, 12 triangular faces
  auto& topology = container.topology();
  topology.set_point_count(8);

  // Store positions
  std::vector<core::Vec3f> positions = {
      {static_cast<float>(min_corner.x()), static_cast<float>(min_corner.y()),
       static_cast<float>(min_corner.z())}, // 0
      {static_cast<float>(max_corner.x()), static_cast<float>(min_corner.y()),
       static_cast<float>(min_corner.z())}, // 1
      {static_cast<float>(max_corner.x()), static_cast<float>(max_corner.y()),
       static_cast<float>(min_corner.z())}, // 2
      {static_cast<float>(min_corner.x()), static_cast<float>(max_corner.y()),
       static_cast<float>(min_corner.z())}, // 3
      {static_cast<float>(min_corner.x()), static_cast<float>(min_corner.y()),
       static_cast<float>(max_corner.z())}, // 4
      {static_cast<float>(max_corner.x()), static_cast<float>(min_corner.y()),
       static_cast<float>(max_corner.z())}, // 5
      {static_cast<float>(max_corner.x()), static_cast<float>(max_corner.y()),
       static_cast<float>(max_corner.z())}, // 6
      {static_cast<float>(min_corner.x()), static_cast<float>(max_corner.y()),
       static_cast<float>(max_corner.z())} // 7
  };

  // Add faces (12 triangles, 2 per cube face)
  // Front face (z = min)
  topology.add_primitive({0, 1, 2});
  topology.add_primitive({0, 2, 3});
  // Back face (z = max)
  topology.add_primitive({4, 7, 6});
  topology.add_primitive({4, 6, 5});
  // Left face (x = min)
  topology.add_primitive({0, 3, 7});
  topology.add_primitive({0, 7, 4});
  // Right face (x = max)
  topology.add_primitive({1, 5, 6});
  topology.add_primitive({1, 6, 2});
  // Bottom face (y = min)
  topology.add_primitive({0, 4, 5});
  topology.add_primitive({0, 5, 1});
  // Top face (y = max)
  topology.add_primitive({3, 2, 6});
  topology.add_primitive({3, 6, 7});

  // Add P (position) attribute
  container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto* p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage != nullptr) {
    auto p_span = p_storage->values_writable();
    std::copy(positions.begin(), positions.end(), p_span.begin());
  }

  // Add N (normal) attribute - box has face normals, but we'll use per-vertex
  // averaged normals For a box, corner vertices touch 3 faces, so we average
  // those face normals
  container.add_point_attribute(attrs::N, core::AttributeType::VEC3F);
  auto* n_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::N);
  if (n_storage != nullptr) {
    auto n_span = n_storage->values_writable();

    // Face normals for a box (6 faces)
    const core::Vec3f normals[8] = {
        {-1.0F, -1.0F, -1.0F}, // 0: min corner (front-bottom-left)
        {1.0F, -1.0F, -1.0F},  // 1: front-bottom-right
        {1.0F, 1.0F, -1.0F},   // 2: front-top-right
        {-1.0F, 1.0F, -1.0F},  // 3: front-top-left
        {-1.0F, -1.0F, 1.0F},  // 4: back-bottom-left
        {1.0F, -1.0F, 1.0F},   // 5: back-bottom-right
        {1.0F, 1.0F, 1.0F},    // 6: back-top-right
        {-1.0F, 1.0F, 1.0F}    // 7: back-top-left
    };

    // Normalize and copy
    for (size_t i = 0; i < 8; ++i) {
      core::Vec3f normal = normals[i];
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
MeshGenerator::sphere(const Eigen::Vector3d& center, double radius,
                      int subdivisions) {
  if (!validate_sphere_params(radius, subdivisions)) {
    return std::nullopt;
  }

  // Use the proper SphereGenerator for icosphere generation
  auto result = SphereGenerator::generate_icosphere(radius, subdivisions);
  if (!result.has_value()) {
    return std::nullopt;
  }

  // Get the container
  auto container = std::move(result.value());

  // Translate the sphere to the desired center if needed
  if (center != Eigen::Vector3d::Zero()) {
    auto* positions =
        container.get_point_attribute_typed<core::Vec3f>(attrs::P);
    if (positions != nullptr) {
      auto p_span = positions->values_writable();
      const core::Vec3f offset{static_cast<float>(center.x()),
                               static_cast<float>(center.y()),
                               static_cast<float>(center.z())};
      for (size_t i = 0; i < p_span.size(); ++i) {
        p_span[i][0] += offset[0];
        p_span[i][1] += offset[1];
        p_span[i][2] += offset[2];
      }
    }
  }

  return container;
}

std::optional<core::GeometryContainer>
MeshGenerator::cylinder(const Eigen::Vector3d& bottom_center,
                        const Eigen::Vector3d& top_center, double radius,
                        int segments) {
  if (!validate_cylinder_params(radius, segments)) {
    return std::nullopt;
  }

  return generate_cylinder_geometry(bottom_center, top_center, radius,
                                    segments);
}

const core::Error& MeshGenerator::last_error() {
  return last_error_;
}

core::GeometryContainer
MeshGenerator::generate_icosphere(const Eigen::Vector3d& center, double radius,
                                  [[maybe_unused]] int subdivisions) {
  // Simple octahedron approximation projected to sphere
  // This is a simplified implementation - for real icosphere use
  // SphereGenerator

  core::GeometryContainer container;
  auto& topology = container.topology();

  // Octahedron has 6 vertices
  topology.set_point_count(6);

  // Store positions (octahedron vertices)
  std::vector<core::Vec3f> positions = {
      {static_cast<float>(center.x() + radius), static_cast<float>(center.y()),
       static_cast<float>(center.z())}, // +X
      {static_cast<float>(center.x() - radius), static_cast<float>(center.y()),
       static_cast<float>(center.z())}, // -X
      {static_cast<float>(center.x()), static_cast<float>(center.y() + radius),
       static_cast<float>(center.z())}, // +Y
      {static_cast<float>(center.x()), static_cast<float>(center.y() - radius),
       static_cast<float>(center.z())}, // -Y
      {static_cast<float>(center.x()), static_cast<float>(center.y()),
       static_cast<float>(center.z() + radius)}, // +Z
      {static_cast<float>(center.x()), static_cast<float>(center.y()),
       static_cast<float>(center.z() - radius)} // -Z
  };

  // Octahedron faces (8 faces)
  topology.add_primitive({0, 2, 4}); // +X, +Y, +Z
  topology.add_primitive({0, 4, 3}); // +X, +Z, -Y
  topology.add_primitive({0, 3, 5}); // +X, -Y, -Z
  topology.add_primitive({0, 5, 2}); // +X, -Z, +Y
  topology.add_primitive({1, 4, 2}); // -X, +Z, +Y
  topology.add_primitive({1, 3, 4}); // -X, -Y, +Z
  topology.add_primitive({1, 5, 3}); // -X, -Z, -Y
  topology.add_primitive({1, 2, 5}); // -X, +Y, -Z

  // Add P (position) attribute
  container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto* p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage != nullptr) {
    auto p_span = p_storage->values_writable();
    std::copy(positions.begin(), positions.end(), p_span.begin());
  }

  // Add N (normal) attribute - for octahedron, normals point from center
  container.add_point_attribute(attrs::N, core::AttributeType::VEC3F);
  auto* n_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::N);
  if (n_storage != nullptr) {
    auto n_span = n_storage->values_writable();
    const core::Vec3f center_vec{static_cast<float>(center.x()),
                                 static_cast<float>(center.y()),
                                 static_cast<float>(center.z())};

    for (size_t i = 0; i < positions.size(); ++i) {
      core::Vec3f normal = {positions[i][0] - center_vec[0],
                            positions[i][1] - center_vec[1],
                            positions[i][2] - center_vec[2]};
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

core::GeometryContainer
MeshGenerator::generate_cylinder_geometry(const Eigen::Vector3d& bottom_center,
                                          const Eigen::Vector3d& top_center,
                                          double radius, int segments) {
  core::GeometryContainer container;
  auto& topology = container.topology();

  // Vertices: 2 centers + segments for bottom ring + segments for top ring
  const int num_vertices = (segments * 2) + 2;
  topology.set_point_count(num_vertices);

  std::vector<core::Vec3f> positions;
  positions.reserve(num_vertices);

  // Bottom center (index 0)
  positions.push_back({static_cast<float>(bottom_center.x()),
                       static_cast<float>(bottom_center.y()),
                       static_cast<float>(bottom_center.z())});

  // Top center (index 1)
  positions.push_back({static_cast<float>(top_center.x()),
                       static_cast<float>(top_center.y()),
                       static_cast<float>(top_center.z())});

  // Bottom ring (indices 2 to 2+segments-1)
  for (int i = 0; i < segments; ++i) {
    const double angle = nodo::core::math::TAU * static_cast<double>(i) /
                         static_cast<double>(segments);
    const Eigen::Vector3d offset(radius * std::cos(angle),
                                 radius * std::sin(angle), 0);
    const Eigen::Vector3d pos = bottom_center + offset;
    positions.push_back({static_cast<float>(pos.x()),
                         static_cast<float>(pos.y()),
                         static_cast<float>(pos.z())});
  }

  // Top ring (indices 2+segments to 2+segments*2-1)
  for (int i = 0; i < segments; ++i) {
    const double angle = nodo::core::math::TAU * static_cast<double>(i) /
                         static_cast<double>(segments);
    const Eigen::Vector3d offset(radius * std::cos(angle),
                                 radius * std::sin(angle), 0);
    const Eigen::Vector3d pos = top_center + offset;
    positions.push_back({static_cast<float>(pos.x()),
                         static_cast<float>(pos.y()),
                         static_cast<float>(pos.z())});
  }

  // Generate faces
  // Bottom cap
  for (int i = 0; i < segments; ++i) {
    const int next = (i + 1) % segments;
    topology.add_primitive({0, 2 + i, 2 + next});
  }

  // Top cap
  for (int i = 0; i < segments; ++i) {
    const int next = (i + 1) % segments;
    topology.add_primitive({1, 2 + segments + next, 2 + segments + i});
  }

  // Side faces (two triangles per segment)
  for (int i = 0; i < segments; ++i) {
    const int next = (i + 1) % segments;
    const int bottom_i = 2 + i;
    const int bottom_next = 2 + next;
    const int top_i = 2 + segments + i;
    const int top_next = 2 + segments + next;

    topology.add_primitive({bottom_i, top_i, top_next});
    topology.add_primitive({bottom_i, top_next, bottom_next});
  }

  // Add P (position) attribute
  container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto* p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage != nullptr) {
    auto p_span = p_storage->values_writable();
    std::copy(positions.begin(), positions.end(), p_span.begin());
  }

  // Add N (normal) attribute
  // Centers have normals pointing down/up, ring vertices have radial normals
  container.add_point_attribute(attrs::N, core::AttributeType::VEC3F);
  auto* n_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::N);
  if (n_storage != nullptr) {
    auto n_span = n_storage->values_writable();

    const Eigen::Vector3d axis = (top_center - bottom_center).normalized();

    // Bottom center normal (pointing down)
    n_span[0] = {static_cast<float>(-axis.x()), static_cast<float>(-axis.y()),
                 static_cast<float>(-axis.z())};

    // Top center normal (pointing up)
    n_span[1] = {static_cast<float>(axis.x()), static_cast<float>(axis.y()),
                 static_cast<float>(axis.z())};

    // Bottom ring normals (radial outward)
    for (int i = 0; i < segments; ++i) {
      const double angle = nodo::core::math::TAU * static_cast<double>(i) /
                           static_cast<double>(segments);
      const float nx = static_cast<float>(std::cos(angle));
      const float ny = static_cast<float>(std::sin(angle));
      n_span[2 + i] = {nx, ny, 0.0F};
    }

    // Top ring normals (same radial pattern)
    for (int i = 0; i < segments; ++i) {
      const double angle = nodo::core::math::TAU * static_cast<double>(i) /
                           static_cast<double>(segments);
      const float nx = static_cast<float>(std::cos(angle));
      const float ny = static_cast<float>(std::sin(angle));
      n_span[2 + segments + i] = {nx, ny, 0.0F};
    }
  }

  return container;
}

bool MeshGenerator::validate_sphere_params(double radius, int subdivisions) {
  if (radius <= 0.0) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidMesh,
                               "Sphere radius must be positive"});
    return false;
  }

  if (subdivisions < 0 || subdivisions > 5) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidMesh,
                               "Sphere subdivisions must be between 0 and 5"});
    return false;
  }

  return true;
}

bool MeshGenerator::validate_cylinder_params(double radius, int segments) {
  if (radius <= 0.0) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidMesh,
                               "Cylinder radius must be positive"});
    return false;
  }

  if (segments < 3) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidMesh,
                               "Cylinder must have at least 3 segments"});
    return false;
  }

  return true;
}

void MeshGenerator::set_last_error(const core::Error& error) {
  last_error_ = error;
}

} // namespace nodo::geometry
