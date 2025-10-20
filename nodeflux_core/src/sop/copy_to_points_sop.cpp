#include "../../include/nodeflux/sop/copy_to_points_sop.hpp"
#include "../../include/nodeflux/core/math.hpp"

namespace nodeflux::sop {

// Constants for copy operations
constexpr float DEFAULT_SCALE_MULTIPLIER = 0.1F;
constexpr double UP_VECTOR_Y = 1.0;

void CopyToPointsSOP::copy_template_to_points(
    const GeometryData &points_data, const GeometryData &template_data,
    GeometryData &output_data, bool use_point_normals, bool use_point_scale,
    float uniform_scale, const std::string &scale_attribute) {

  auto template_mesh = template_data.get_mesh();
  if (!template_mesh || template_mesh->empty()) {
    return; // No template geometry
  }

  const auto &template_vertices = template_mesh->vertices();
  const auto &template_faces = template_mesh->faces();

  // Get point positions from scattered points
  auto points_mesh = points_data.get_mesh();
  if (!points_mesh || points_mesh->empty()) {
    return; // No points to copy to
  }

  size_t point_count = points_mesh->vertices().rows();
  const auto &point_vertices = points_mesh->vertices();

  // Calculate output mesh dimensions
  size_t vertices_per_instance = template_vertices.rows();
  size_t faces_per_instance = template_faces.rows();
  size_t total_vertices = point_count * vertices_per_instance;
  size_t total_faces = point_count * faces_per_instance;

  // Create output mesh
  core::Mesh::Vertices output_vertices(total_vertices, 3);
  core::Mesh::Faces output_faces(total_faces, 3);

  // Prepare attribute arrays
  std::vector<GeometryData::AttributeValue> instance_ids;
  std::vector<GeometryData::AttributeValue> material_ids;
  instance_ids.reserve(total_vertices);
  material_ids.reserve(total_faces);

  // Copy template to each point
  for (size_t point_idx = 0; point_idx < point_count; ++point_idx) {
    // Get point position
    core::Vector3 point_position(point_vertices(point_idx, 0),
                                 point_vertices(point_idx, 1),
                                 point_vertices(point_idx, 2));

    // Use default normal (up)
    core::Vector3 normal(0.0, UP_VECTOR_Y, 0.0);

    // Calculate scale for this instance
    float point_scale = uniform_scale;

    // Transform template vertices for this point
    size_t vertex_offset = point_idx * vertices_per_instance;
    transform_template_for_point(*template_mesh, point_position, normal,
                                 point_scale, output_vertices, vertex_offset);

    // Copy and offset faces
    size_t face_offset = point_idx * faces_per_instance;
    for (size_t face_idx = 0; face_idx < faces_per_instance; ++face_idx) {
      const auto &template_face = template_faces.row(face_idx);
      size_t output_face_idx = face_offset + face_idx;

      output_faces(output_face_idx, 0) =
          template_face[0] + static_cast<int>(vertex_offset);
      output_faces(output_face_idx, 1) =
          template_face[1] + static_cast<int>(vertex_offset);
      output_faces(output_face_idx, 2) =
          template_face[2] + static_cast<int>(vertex_offset);

      // Store face material ID
      material_ids.push_back(static_cast<int>(point_idx));
    }

    // Set vertex attributes for this instance
    for (size_t vertex_idx = 0; vertex_idx < vertices_per_instance;
         ++vertex_idx) {
      instance_ids.push_back(static_cast<int>(point_idx));
    }
  }

  // Create final output mesh
  auto output_mesh =
      std::make_shared<core::Mesh>(output_vertices, output_faces);
  output_data.set_mesh(output_mesh);

  // Set attributes
  output_data.set_vertex_attribute("instance_id", instance_ids);
  output_data.set_face_attribute("material_id", material_ids);
}

void CopyToPointsSOP::transform_template_for_point(
    const core::Mesh &template_mesh, const core::Vector3 &point_position,
    const core::Vector3 &point_normal, float point_scale,
    core::Mesh::Vertices &output_vertices, size_t vertex_offset) {

  const auto &template_vertices = template_mesh.vertices();

  // Create transformation matrix (simplified - just scale and translate)
  core::Matrix3 scale_matrix =
      core::Matrix3::Identity() * static_cast<double>(point_scale);

  // Transform each template vertex
  for (int vertex_idx = 0; vertex_idx < template_vertices.rows();
       ++vertex_idx) {
    // Get template vertex
    core::Vector3 template_vertex(template_vertices(vertex_idx, 0),
                                  template_vertices(vertex_idx, 1),
                                  template_vertices(vertex_idx, 2));

    // Apply scale
    core::Vector3 scaled_vertex = scale_matrix * template_vertex;

    // Apply translation
    core::Vector3 final_vertex = scaled_vertex + point_position;

    // Store in output
    size_t output_idx = vertex_offset + vertex_idx;
    output_vertices(output_idx, 0) = final_vertex.x();
    output_vertices(output_idx, 1) = final_vertex.y();
    output_vertices(output_idx, 2) = final_vertex.z();
  }
}

} // namespace nodeflux::sop
