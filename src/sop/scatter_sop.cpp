#include "../../include/nodeflux/sop/scatter_sop.hpp"
#include "../../include/nodeflux/core/math.hpp"
#include <algorithm>
#include <numeric>

namespace nodeflux::sop {

// Constants for scatter operations
constexpr double TRIANGLE_AREA_FACTOR = 0.5;
constexpr double BARYCENTRIC_MIN = 0.0;
constexpr double BARYCENTRIC_NORMALIZE = 1.0;

void ScatterSOP::scatter_points_on_mesh(const core::Mesh &input_mesh,
                                        GeometryData &output_data,
                                        int point_count, int seed,
                                        float density, bool use_face_area) {

  // Setup random number generation
  std::mt19937 generator(seed);
  std::uniform_real_distribution<double> unit_dist(0.0, 1.0);

  const auto &vertices = input_mesh.vertices();
  const auto &faces = input_mesh.faces();

  if (faces.rows() == 0) {
    return; // No faces to scatter on
  }

  // Calculate face areas for weighted distribution
  std::vector<double> face_areas;
  std::vector<double> cumulative_areas;

  if (use_face_area) {
    face_areas = calculate_face_areas(input_mesh);
    cumulative_areas.resize(face_areas.size());
    std::partial_sum(face_areas.begin(), face_areas.end(),
                     cumulative_areas.begin());
  }

  // Apply density scaling
  int actual_point_count = static_cast<int>(point_count * density);
  actual_point_count = std::max(1, actual_point_count);

  // Create output mesh with scattered points
  core::Mesh::Vertices scattered_vertices(actual_point_count, 3);

  // Initialize output attributes
  output_data->attributes.initialize_standard_attributes(actual_point_count, 0);

  // Add scatter-specific attributes
  output_data->attributes.add_attribute<int>(
      "point_index", core::AttributeClass::VERTEX, actual_point_count);
  output_data->attributes.add_attribute<int>(
      "source_face", core::AttributeClass::VERTEX, actual_point_count);
  output_data->attributes.add_attribute<core::Vector3>(
      "barycentric", core::AttributeClass::VERTEX, actual_point_count);

  // Generate scattered points
  for (int point_idx = 0; point_idx < actual_point_count; ++point_idx) {
    // Select face (weighted by area if enabled)
    int face_index;
    if (use_face_area && !cumulative_areas.empty()) {
      double random_area = unit_dist(generator) * cumulative_areas.back();
      auto area_iterator = std::lower_bound(
          cumulative_areas.begin(), cumulative_areas.end(), random_area);
      face_index = static_cast<int>(
          std::distance(cumulative_areas.begin(), area_iterator));
      face_index = std::min(face_index, static_cast<int>(faces.rows() - 1));
    } else {
      // Uniform face selection
      std::uniform_int_distribution<int> face_dist(
          0, static_cast<int>(faces.rows() - 1));
      face_index = face_dist(generator);
    }

    // Get triangle vertices
    const auto &face = faces.row(face_index);
    core::Vector3 vertex_0(vertices(face[0], 0), vertices(face[0], 1),
                           vertices(face[0], 2));
    core::Vector3 vertex_1(vertices(face[1], 0), vertices(face[1], 1),
                           vertices(face[1], 2));
    core::Vector3 vertex_2(vertices(face[2], 0), vertices(face[2], 1),
                           vertices(face[2], 2));

    // Generate random point on triangle
    core::Vector3 scattered_point =
        random_point_on_triangle(vertex_0, vertex_1, vertex_2, generator);

    // Store point in output mesh
    scattered_vertices(point_idx, 0) = scattered_point.x();
    scattered_vertices(point_idx, 1) = scattered_point.y();
    scattered_vertices(point_idx, 2) = scattered_point.z();

    // Set attributes
    output_data->attributes.set_position(point_idx, scattered_point);
    output_data->attributes.set_attribute("point_index", point_idx, point_idx);
    output_data->attributes.set_attribute("source_face", point_idx, face_index);

    // Calculate barycentric coordinates for attribute interpolation
    // (This is simplified - proper barycentric calculation would be more
    // complex)
    double u_coord = unit_dist(generator);
    double v_coord = unit_dist(generator);
    if (u_coord + v_coord > BARYCENTRIC_NORMALIZE) {
      u_coord = BARYCENTRIC_NORMALIZE - u_coord;
      v_coord = BARYCENTRIC_NORMALIZE - v_coord;
    }
    double w_coord = BARYCENTRIC_NORMALIZE - u_coord - v_coord;

    core::Vector3 barycentric_coords(u_coord, v_coord, w_coord);
    output_data->attributes.set_attribute("barycentric", point_idx,
                                          barycentric_coords);

    // Set default normal (up)
    output_data->attributes.set_normal(point_idx, core::Vector3(0.0, 0.0, 1.0));

    // Set color based on point index for visualization
    double color_ratio = static_cast<double>(point_idx) /
                         static_cast<double>(actual_point_count - 1);
    core::Vector3 point_color(color_ratio, 0.5, 1.0 - color_ratio);
    output_data->attributes.set_color(point_idx, point_color);
  }

  // Create output mesh (points only, no faces)
  core::Mesh::Faces empty_faces(0, 3);
  output_data->mesh = core::Mesh(scattered_vertices, empty_faces);
}

std::vector<double> ScatterSOP::calculate_face_areas(const core::Mesh &mesh) {
  const auto &vertices = mesh.vertices();
  const auto &faces = mesh.faces();

  std::vector<double> areas;
  areas.reserve(faces.rows());

  for (int face_idx = 0; face_idx < faces.rows(); ++face_idx) {
    const auto &face = faces.row(face_idx);

    core::Vector3 vertex_0(vertices(face[0], 0), vertices(face[0], 1),
                           vertices(face[0], 2));
    core::Vector3 vertex_1(vertices(face[1], 0), vertices(face[1], 1),
                           vertices(face[1], 2));
    core::Vector3 vertex_2(vertices(face[2], 0), vertices(face[2], 1),
                           vertices(face[2], 2));

    // Calculate triangle area using cross product
    core::Vector3 edge_1 = vertex_1 - vertex_0;
    core::Vector3 edge_2 = vertex_2 - vertex_0;
    double area = TRIANGLE_AREA_FACTOR * edge_1.cross(edge_2).norm();

    areas.push_back(area);
  }

  return areas;
}

core::Vector3 ScatterSOP::random_point_on_triangle(
    const core::Vector3 &vertex_0, const core::Vector3 &vertex_1,
    const core::Vector3 &vertex_2, std::mt19937 &generator) {
  std::uniform_real_distribution<double> unit_dist(0.0, 1.0);

  // Generate random barycentric coordinates
  double u_coord = unit_dist(generator);
  double v_coord = unit_dist(generator);

  // Ensure point is inside triangle
  if (u_coord + v_coord > BARYCENTRIC_NORMALIZE) {
    u_coord = BARYCENTRIC_NORMALIZE - u_coord;
    v_coord = BARYCENTRIC_NORMALIZE - v_coord;
  }

  double w_coord = BARYCENTRIC_NORMALIZE - u_coord - v_coord;

  // Interpolate position using barycentric coordinates
  return w_coord * vertex_0 + u_coord * vertex_1 + v_coord * vertex_2;
}

void ScatterSOP::interpolate_attributes_at_point(
    const core::GeometryAttributes &input_attrs,
    core::GeometryAttributes &output_attrs, int face_index,
    const core::Vector3 &barycentric_coords, size_t output_point_index) {
  // This would interpolate vertex attributes from the input mesh to the
  // scattered point Implementation would depend on which attributes need
  // interpolation For now, we'll leave this as a placeholder for future
  // enhancement
}

} // namespace nodeflux::sop
