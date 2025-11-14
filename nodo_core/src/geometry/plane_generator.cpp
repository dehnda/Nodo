#include "nodo/geometry/plane_generator.hpp"

namespace attrs = nodo::core::standard_attrs;

namespace nodo::geometry {

// Thread-local storage for error reporting
thread_local core::Error PlaneGenerator::last_error_{
    core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"};

std::optional<core::GeometryContainer>
PlaneGenerator::generate(double width, double height, int width_segments,
                         int height_segments) {
  if (width <= 0.0) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidFormat,
                               "Plane width must be positive"});
    return std::nullopt;
  }

  if (height <= 0.0) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidFormat,
                               "Plane height must be positive"});
    return std::nullopt;
  }

  if (width_segments < 1) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidFormat,
                               "Plane requires at least 1 width segment"});
    return std::nullopt;
  }

  if (height_segments < 1) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::InvalidFormat,
                               "Plane requires at least 1 height segment"});
    return std::nullopt;
  }

  core::GeometryContainer container;

  // Calculate vertices and faces
  const int vertices_per_row = width_segments + 1;
  const int vertices_per_col = height_segments + 1;
  const int total_vertices = vertices_per_row * vertices_per_col;

  container.set_point_count(total_vertices);
  container.set_vertex_count(total_vertices); // 1:1 mapping for plane

  auto& topology = container.topology();

  // Set up 1:1 vertex→point mapping
  for (size_t i = 0; i < static_cast<size_t>(total_vertices); ++i) {
    topology.set_vertex_point(i, static_cast<int>(i));
  }

  // Generate vertices
  std::vector<core::Vec3f> positions;
  positions.reserve(total_vertices);

  const double half_width = width * 0.5;
  const double half_height = height * 0.5;

  for (int row = 0; row < vertices_per_col; ++row) {
    const double coord_z =
        -half_height +
        (static_cast<double>(row) / static_cast<double>(height_segments)) *
            height;

    for (int col = 0; col < vertices_per_row; ++col) {
      const double coord_x =
          -half_width +
          (static_cast<double>(col) / static_cast<double>(width_segments)) *
              width;

      positions.push_back({static_cast<float>(coord_x),
                           0.0F, // Plane lies in XZ plane
                           static_cast<float>(coord_z)});
    }
  }

  // Generate quad faces
  for (int row = 0; row < height_segments; ++row) {
    for (int col = 0; col < width_segments; ++col) {
      const int top_left = (row * vertices_per_row) + col;
      const int top_right = top_left + 1;
      const int bottom_left = ((row + 1) * vertices_per_row) + col;
      const int bottom_right = bottom_left + 1;

      // Single quad (counter-clockwise winding)
      container.add_primitive({top_left, bottom_left, bottom_right, top_right});
    }
  }

  // Add P (position) attribute
  container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto* p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage != nullptr) {
    auto p_span = p_storage->values_writable();
    std::copy(positions.begin(), positions.end(), p_span.begin());
  }

  // Add N (normal) attribute - plane has uniform normal pointing up (Y+)
  container.add_point_attribute(attrs::N, core::AttributeType::VEC3F);
  auto* n_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::N);
  if (n_storage != nullptr) {
    auto n_span = n_storage->values_writable();
    const core::Vec3f up_normal{0.0F, 1.0F, 0.0F};
    for (size_t i = 0; i < positions.size(); ++i) {
      n_span[i] = up_normal;
    }
  }

  return container;
}

const core::Error& PlaneGenerator::last_error() {
  return last_error_;
}

void PlaneGenerator::set_last_error(const core::Error& error) {
  last_error_ = error;
}

} // namespace nodo::geometry
