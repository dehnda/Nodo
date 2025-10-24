#include "nodeflux/sop/subdivisions_sop.hpp"
#include "nodeflux/core/math.hpp"
#include "nodeflux/core/types.hpp"
#include <vector>

namespace nodeflux::sop {

SubdivisionSOP::SubdivisionSOP(const std::string &name)
    : SOPNode(name, "Subdivision") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(
      define_int_parameter("subdivision_levels", 1)
          .label("Subdivision Levels")
          .range(0, 5)
          .category("Subdivision")
          .build());

  register_parameter(
      define_int_parameter("preserve_boundaries", 1)
          .label("Preserve Boundaries")
          .range(0, 1)
          .category("Subdivision")
          .build());
}

std::shared_ptr<GeometryData> SubdivisionSOP::execute() {
  // Sync member variables from parameter system
  subdivision_levels_ = get_parameter<int>("subdivision_levels", 1);
  preserve_boundaries_ = (get_parameter<int>("preserve_boundaries", 1) != 0);

  // Get input geometry
  auto input_geo = get_input_data(0);
  if (!input_geo) {
    set_error("No input geometry connected");
    return nullptr;
  }

  auto input_mesh = input_geo->get_mesh();
  if (!input_mesh) {
    set_error("Input geometry does not contain a mesh");
    return nullptr;
  }

  if (subdivision_levels_ == 0) {
    // No subdivision, return copy
    return std::make_shared<GeometryData>(input_mesh);
  }

  // Apply subdivision iteratively
  core::Mesh result = *input_mesh;
  for (int level = 0; level < subdivision_levels_; ++level) {
    auto subdivided = apply_catmull_clark_subdivision(result);
    if (!subdivided) {
      set_error("Subdivision failed");
      return nullptr;
    }
    result = std::move(*subdivided);
  }

  auto result_mesh = std::make_shared<core::Mesh>(std::move(result));
  return std::make_shared<GeometryData>(result_mesh);
}

std::optional<core::Mesh>
SubdivisionSOP::apply_catmull_clark_subdivision(const core::Mesh &mesh) {
  const auto &vertices = mesh.vertices();
  const auto &faces = mesh.faces();

  // Simple approach: refine each triangle by adding midpoints and face center
  std::vector<core::Vector3> new_vertices;
  std::vector<core::Vector3i> new_faces;

  // Add original vertices
  for (int i = 0; i < vertices.rows(); ++i) {
    new_vertices.push_back(vertices.row(i));
  }

  // Process each face
  for (int face_idx = 0; face_idx < faces.rows(); ++face_idx) {
    const auto &face = faces.row(face_idx);

    // Get face vertices
    core::Vector3 vertex_0 = vertices.row(face(0));
    core::Vector3 vertex_1 = vertices.row(face(1));
    core::Vector3 vertex_2 = vertices.row(face(2));

    // Calculate face center using utility function
    core::Vector3 face_center = core::math::triangle_centroid(vertex_0, vertex_1, vertex_2);
    int face_center_idx = static_cast<int>(new_vertices.size());
    new_vertices.push_back(face_center);

    // Calculate edge midpoints using utility function
    core::Vector3 edge01_mid = core::math::midpoint(vertex_0, vertex_1);
    core::Vector3 edge12_mid = core::math::midpoint(vertex_1, vertex_2);
    core::Vector3 edge20_mid = core::math::midpoint(vertex_2, vertex_0);

    int edge01_idx = static_cast<int>(new_vertices.size());
    int edge12_idx = static_cast<int>(new_vertices.size() + 1);
    int edge20_idx = static_cast<int>(new_vertices.size() + 2);

    new_vertices.push_back(edge01_mid);
    new_vertices.push_back(edge12_mid);
    new_vertices.push_back(edge20_mid);

    // Create new faces (split triangle into 6 triangles)
    new_faces.emplace_back(face(0), edge01_idx, edge20_idx);
    new_faces.emplace_back(edge01_idx, face(1), edge12_idx);
    new_faces.emplace_back(edge20_idx, edge12_idx, face(2));
    new_faces.emplace_back(edge01_idx, edge12_idx, face_center_idx);
    new_faces.emplace_back(edge12_idx, edge20_idx, face_center_idx);
    new_faces.emplace_back(edge20_idx, edge01_idx, face_center_idx);
  }

  // Convert to Eigen matrices
  core::Mesh::Vertices output_vertices(new_vertices.size(), 3);
  core::Mesh::Faces output_faces(new_faces.size(), 3);

  for (size_t i = 0; i < new_vertices.size(); ++i) {
    output_vertices.row(i) = new_vertices[i];
  }

  for (size_t i = 0; i < new_faces.size(); ++i) {
    output_faces.row(i) = new_faces[i];
  }

  return core::Mesh(std::move(output_vertices), std::move(output_faces));
}

} // namespace nodeflux::sop
