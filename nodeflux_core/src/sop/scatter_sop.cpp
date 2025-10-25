#include "../../include/nodeflux/sop/scatter_sop.hpp"
#include "../../include/nodeflux/core/math.hpp"
#include "../../include/nodeflux/core/standard_attributes.hpp"
#include <algorithm>
#include <numeric>

namespace nodeflux::sop {

namespace attrs = nodeflux::core::standard_attrs;

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

void ScatterSOP::scatter_points_on_mesh(
    const core::GeometryContainer &input_geo,
    core::GeometryContainer &output_geo, int point_count, int seed,
    float density, bool use_face_area) {

  // Setup random number generation
  std::mt19937 generator(seed);
  std::uniform_real_distribution<double> unit_dist(0.0, 1.0);

  // Extract topology from input geometry
  const auto &input_topo = input_geo.topology();
  size_t prim_count = input_topo.primitive_count();

  if (prim_count == 0) {
    return; // No primitives to scatter on
  }

  // Get input positions
  auto *input_positions =
      input_geo.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (!input_positions) {
    throw std::runtime_error(
        "ScatterSOP: Input geometry has no position attribute");
  }

  // Build face areas for weighted distribution
  std::vector<double> face_areas;
  std::vector<double> cumulative_areas;

  if (use_face_area) {
    face_areas = calculate_face_areas_from_container(input_geo);
    cumulative_areas.resize(face_areas.size());
    std::partial_sum(face_areas.begin(), face_areas.end(),
                     cumulative_areas.begin());
  }

  // Apply density scaling
  int actual_point_count = static_cast<int>(point_count * density);
  actual_point_count = std::max(1, actual_point_count);

  // Setup geometry container with just points (no primitives for point cloud)
  output_geo.set_point_count(actual_point_count);
  output_geo.set_vertex_count(0); // Point cloud - no vertices needed

  // Add position attribute (required)
  output_geo.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *positions = output_geo.get_point_attribute_typed<core::Vec3f>(attrs::P);

  // Add custom attributes for metadata
  output_geo.add_point_attribute("id", core::AttributeType::INT);
  auto *point_ids = output_geo.get_point_attribute_typed<int>("id");

  output_geo.add_point_attribute("source_face", core::AttributeType::INT);
  auto *source_faces = output_geo.get_point_attribute_typed<int>("source_face");

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
      face_index = std::min(face_index, static_cast<int>(prim_count - 1));
    } else {
      // Uniform face selection
      std::uniform_int_distribution<int> face_dist(
          0, static_cast<int>(prim_count - 1));
      face_index = face_dist(generator);
    }

    // Get primitive vertices (assuming triangles for now)
    auto prim_verts = input_topo.get_primitive_vertices(face_index);
    if (prim_verts.size() < 3) {
      continue; // Skip non-triangular primitives
    }

    // Get the 3 points of the triangle
    int v0_idx = input_topo.get_vertex_point(prim_verts[0]);
    int v1_idx = input_topo.get_vertex_point(prim_verts[1]);
    int v2_idx = input_topo.get_vertex_point(prim_verts[2]);

    const auto &p0 = (*input_positions)[v0_idx];
    const auto &p1 = (*input_positions)[v1_idx];
    const auto &p2 = (*input_positions)[v2_idx];

    // Convert to Vector3 for calculation
    core::Vector3 vertex_0(p0.x(), p0.y(), p0.z());
    core::Vector3 vertex_1(p1.x(), p1.y(), p1.z());
    core::Vector3 vertex_2(p2.x(), p2.y(), p2.z());

    // Generate random point on triangle
    core::Vector3 scattered_point =
        random_point_on_triangle(vertex_0, vertex_1, vertex_2, generator);

    // Store position in attribute storage
    positions->set(point_idx,
                   core::Vec3f(scattered_point.x(), scattered_point.y(),
                               scattered_point.z()));

    // Store metadata attributes
    point_ids->set(point_idx, point_idx);
    source_faces->set(point_idx, face_index);
  }

  // Validate topology (optional but good practice)
  if (!output_geo.topology().validate()) {
    throw std::runtime_error("ScatterSOP: Generated invalid topology");
  }
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

std::vector<double> ScatterSOP::calculate_face_areas_from_container(
    const core::GeometryContainer &geo) {
  const auto &topo = geo.topology();
  auto *positions = geo.get_point_attribute_typed<core::Vec3f>(attrs::P);

  if (!positions) {
    return {};
  }

  std::vector<double> areas;
  areas.reserve(topo.primitive_count());

  for (size_t prim_idx = 0; prim_idx < topo.primitive_count(); ++prim_idx) {
    auto prim_verts = topo.get_primitive_vertices(prim_idx);

    if (prim_verts.size() < 3) {
      areas.push_back(0.0); // Invalid primitive
      continue;
    }

    // Get triangle vertices (first 3 points)
    int v0_idx = topo.get_vertex_point(prim_verts[0]);
    int v1_idx = topo.get_vertex_point(prim_verts[1]);
    int v2_idx = topo.get_vertex_point(prim_verts[2]);

    const auto &p0 = (*positions)[v0_idx];
    const auto &p1 = (*positions)[v1_idx];
    const auto &p2 = (*positions)[v2_idx];

    core::Vector3 vertex_0(p0.x(), p0.y(), p0.z());
    core::Vector3 vertex_1(p1.x(), p1.y(), p1.z());
    core::Vector3 vertex_2(p2.x(), p2.y(), p2.z());

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

std::shared_ptr<GeometryData>
ScatterSOP::convert_from_container(const core::GeometryContainer &container) {
  // Temporary bridge: Convert GeometryContainer back to old GeometryData
  // This allows scatter SOP to work with existing pipeline during migration

  auto output_data = std::make_shared<GeometryData>();

  // Get positions from container
  auto *positions = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (positions == nullptr) {
    return output_data; // No positions, return empty
  }

  size_t point_count = container.topology().point_count();

  // Create mesh with points (no faces for point cloud)
  core::Mesh::Vertices vertices(point_count, 3);
  core::Mesh::Faces faces(0, 3);

  // Copy positions
  for (size_t i = 0; i < point_count; ++i) {
    const auto &pos = (*positions)[i];
    vertices(i, 0) = pos.x();
    vertices(i, 1) = pos.y();
    vertices(i, 2) = pos.z();
  }

  auto mesh = std::make_shared<core::Mesh>(vertices, faces);
  output_data->set_mesh(mesh);

  // Copy custom attributes (id, source_face)
  auto *point_ids = container.get_point_attribute_typed<int>("id");
  auto *source_faces = container.get_point_attribute_typed<int>("source_face");

  if (point_ids) {
    std::vector<GeometryData::AttributeValue> ids;
    ids.reserve(point_count);
    for (size_t i = 0; i < point_count; ++i) {
      ids.push_back((*point_ids)[i]);
    }
    output_data->set_vertex_attribute("point_index", ids);
  }

  if (source_faces) {
    std::vector<GeometryData::AttributeValue> faces_attr;
    faces_attr.reserve(point_count);
    for (size_t i = 0; i < point_count; ++i) {
      faces_attr.push_back((*source_faces)[i]);
    }
    output_data->set_vertex_attribute("source_face", faces_attr);
  }

  return output_data;
}

std::unique_ptr<core::GeometryContainer>
ScatterSOP::convert_to_container(const GeometryData &old_data) {
  auto container = std::make_unique<core::GeometryContainer>();

  // Get old mesh
  auto mesh = old_data.get_mesh();
  if (!mesh) {
    return nullptr;
  }

  const auto &vertices = mesh->vertices();
  const auto &faces = mesh->faces();

  // Set topology
  size_t point_count = vertices.rows();
  size_t face_count = faces.rows();

  container->set_point_count(point_count);

  // Calculate total vertices
  size_t total_vertices = 0;
  for (int i = 0; i < faces.rows(); ++i) {
    total_vertices += faces.cols(); // Assuming fixed size (triangles/quads)
  }
  container->set_vertex_count(total_vertices);

  // Build topology
  size_t vert_idx = 0;
  for (int face_idx = 0; face_idx < faces.rows(); ++face_idx) {
    std::vector<int> prim_verts;

    for (int j = 0; j < faces.cols(); ++j) {
      int point_idx = faces(face_idx, j);
      container->topology().set_vertex_point(vert_idx, point_idx);
      prim_verts.push_back(static_cast<int>(vert_idx));
      ++vert_idx;
    }

    container->add_primitive(prim_verts);
  }

  // Copy positions
  container->add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *positions = container->get_point_attribute_typed<core::Vec3f>(attrs::P);

  for (int i = 0; i < vertices.rows(); ++i) {
    (*positions)[i] = vertices.row(i).cast<float>();
  }

  return container;
}

} // namespace nodeflux::sop
