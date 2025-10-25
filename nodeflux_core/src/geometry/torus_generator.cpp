#include "../../include/nodeflux/geometry/torus_generator.hpp"
#include <Eigen/Dense>
#include <cmath>
#include <numbers>

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::geometry {

thread_local core::Error TorusGenerator::last_error_{
    core::ErrorCategory::Geometry, core::ErrorCode::Unknown, ""};

std::optional<core::GeometryContainer>
TorusGenerator::generate(double major_radius, double minor_radius,
                         int major_segments, int minor_segments) {
  // Validate parameters
  if (major_radius <= 0.0) {
    last_error_ = core::ErrorUtils::make_error(
        core::ErrorCategory::Geometry, core::ErrorCode::InvalidMesh,
        "Major radius must be positive", "TorusGenerator::generate");
    return std::nullopt;
  }

  if (minor_radius <= 0.0) {
    last_error_ = core::ErrorUtils::make_error(
        core::ErrorCategory::Geometry, core::ErrorCode::InvalidMesh,
        "Minor radius must be positive", "TorusGenerator::generate");
    return std::nullopt;
  }

  if (major_segments < 3) {
    last_error_ = core::ErrorUtils::make_error(
        core::ErrorCategory::Geometry, core::ErrorCode::InvalidMesh,
        "Major segments must be at least 3", "TorusGenerator::generate");
    return std::nullopt;
  }

  if (minor_segments < 3) {
    last_error_ = core::ErrorUtils::make_error(
        core::ErrorCategory::Geometry, core::ErrorCode::InvalidMesh,
        "Minor segments must be at least 3", "TorusGenerator::generate");
    return std::nullopt;
  }

  core::GeometryContainer container;
  auto &topology = container.topology();

  // Calculate number of vertices
  const int num_vertices = major_segments * minor_segments;
  topology.set_point_count(num_vertices);

  // Generate vertices and normals
  std::vector<core::Vec3f> positions;
  std::vector<core::Vec3f> normals;
  positions.reserve(num_vertices);
  normals.reserve(num_vertices);

  const double major_angle_step = 2.0 * std::numbers::pi / major_segments;
  const double minor_angle_step = 2.0 * std::numbers::pi / minor_segments;

  for (int major_idx = 0; major_idx < major_segments; ++major_idx) {
    const double major_angle =
        static_cast<double>(major_idx) * major_angle_step;
    const double cos_major = std::cos(major_angle);
    const double sin_major = std::sin(major_angle);

    for (int minor_idx = 0; minor_idx < minor_segments; ++minor_idx) {
      const double minor_angle =
          static_cast<double>(minor_idx) * minor_angle_step;
      const double cos_minor = std::cos(minor_angle);
      const double sin_minor = std::sin(minor_angle);

      // Calculate vertex position
      const double pos_x =
          (major_radius + (minor_radius * cos_minor)) * cos_major;
      const double pos_y =
          (major_radius + (minor_radius * cos_minor)) * sin_major;
      const double pos_z = minor_radius * sin_minor;

      positions.push_back({static_cast<float>(pos_x), static_cast<float>(pos_y),
                           static_cast<float>(pos_z)});

      // Calculate normal (points radially outward from tube center)
      const double normal_x = cos_minor * cos_major;
      const double normal_y = cos_minor * sin_major;
      const double normal_z = sin_minor;

      normals.push_back({static_cast<float>(normal_x),
                         static_cast<float>(normal_y),
                         static_cast<float>(normal_z)});
    }
  }

  // Generate faces
  for (int major_idx = 0; major_idx < major_segments; ++major_idx) {
    const int next_major = (major_idx + 1) % major_segments;

    for (int minor_idx = 0; minor_idx < minor_segments; ++minor_idx) {
      const int next_minor = (minor_idx + 1) % minor_segments;

      // Current quad vertices
      const int vert_00 = (major_idx * minor_segments) + minor_idx;
      const int vert_01 = (major_idx * minor_segments) + next_minor;
      const int vert_11 = (next_major * minor_segments) + next_minor;
      const int vert_10 = (next_major * minor_segments) + minor_idx;

      // Create two triangles for the quad
      topology.add_primitive({vert_00, vert_01, vert_11});
      topology.add_primitive({vert_00, vert_11, vert_10});
    }
  }

  // Add P (position) attribute
  container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage != nullptr) {
    auto p_span = p_storage->values_writable();
    std::copy(positions.begin(), positions.end(), p_span.begin());
  }

  // Add N (normal) attribute
  container.add_point_attribute(attrs::N, core::AttributeType::VEC3F);
  auto *n_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::N);
  if (n_storage != nullptr) {
    auto n_span = n_storage->values_writable();
    std::copy(normals.begin(), normals.end(), n_span.begin());
  }

  // Clear any previous error
  last_error_ =
      core::Error{core::ErrorCategory::Geometry, core::ErrorCode::Unknown, ""};
  return container;
}

const core::Error &TorusGenerator::last_error() { return last_error_; }

} // namespace nodeflux::geometry
