#include "../../include/nodeflux/sop/scatter_sop.hpp"
#include "../../include/nodeflux/core/math.hpp"
#include <algorithm>
#include <numeric>

namespace nodeflux::sop {

// Constants for scatter operations
constexpr double TRIANGLE_AREA_FACTOR = 0.5;
constexpr double BARYCENTRIC_MIN = 0.0;
constexpr double BARYCENTRIC_NORMALIZE = 1.0;

ScatterSOP::ScatterSOP(const std::string &node_name)
    : SOPNode(node_name, "Scatter") {
  // Add input port
  input_ports_.add_port("input", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_int_parameter("point_count", 100)
                         .label("Point Count")
                         .range(1, 10000)
                         .category("Scatter")
                         .build());

  register_parameter(define_int_parameter("seed", 42)
                         .label("Seed")
                         .range(0, 99999)
                         .category("Scatter")
                         .build());

  register_parameter(define_float_parameter("density", 1.0F)
                         .label("Density")
                         .range(0.0, 10.0)
                         .category("Scatter")
                         .build());

  register_parameter(define_int_parameter("use_face_area", 1)
                         .label("Use Face Area")
                         .range(0, 1)
                         .category("Scatter")
                         .build());
}

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
  core::Mesh::Faces scattered_faces(0, 3); // Point cloud - no faces

  // Prepare attribute arrays
  std::vector<GeometryData::AttributeValue> point_indices;
  std::vector<GeometryData::AttributeValue> source_faces_attr;
  point_indices.reserve(actual_point_count);
  source_faces_attr.reserve(actual_point_count);

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

    // Store attributes
    point_indices.push_back(point_idx);
    source_faces_attr.push_back(face_index);
  }

  // Create output mesh (points only, no faces)
  auto scattered_mesh =
      std::make_shared<core::Mesh>(scattered_vertices, scattered_faces);
  output_data.set_mesh(scattered_mesh);

  // Set attributes
  output_data.set_vertex_attribute("point_index", point_indices);
  output_data.set_vertex_attribute("source_face", source_faces_attr);
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

} // namespace nodeflux::sop
