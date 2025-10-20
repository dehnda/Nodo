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

  const auto &template_mesh = template_data.mesh;
  const auto &template_vertices = template_mesh.vertices();
  const auto &template_faces = template_mesh.faces();

  if (template_vertices.rows() == 0) {
    return; // No template geometry
  }

  // Get point positions from scattered points
  size_t point_count = points_data.mesh.vertices().rows();
  if (point_count == 0) {
    return; // No points to copy to
  }

  // Calculate output mesh dimensions
  size_t vertices_per_instance = template_vertices.rows();
  size_t faces_per_instance = template_faces.rows();
  size_t total_vertices = point_count * vertices_per_instance;
  size_t total_faces = point_count * faces_per_instance;

  // Create output mesh
  core::Mesh::Vertices output_vertices(total_vertices, 3);
  core::Mesh::Faces output_faces(total_faces, 3);

  // Initialize output attributes
  output_data->attributes.initialize_standard_attributes(total_vertices,
                                                         total_faces);

  // Add instance-specific attributes
  output_data->attributes.add_attribute<int>(
      "instance_id", core::AttributeClass::VERTEX, total_vertices);
  output_data->attributes.add_attribute<int>(
      "original_point_index", core::AttributeClass::VERTEX, total_vertices);
  output_data->attributes.add_attribute<float>(
      "instance_scale", core::AttributeClass::VERTEX, total_vertices);

  // Copy template to each point
  for (size_t point_idx = 0; point_idx < point_count; ++point_idx) {
    // Get point data
    auto point_position = points_data.attributes.get_position(point_idx);
    auto point_normal = points_data.attributes.get_normal(point_idx);

    if (!point_position.has_value()) {
      continue; // Skip invalid points
    }

    // Calculate scale for this instance
    float point_scale =
        calculate_point_scale(points_data.attributes, point_idx,
                              scale_attribute, uniform_scale, use_point_scale);

    // Use default normal if not available
    core::Vector3 normal =
        point_normal.value_or(core::Vector3(0.0, UP_VECTOR_Y, 0.0));

    // Transform template vertices for this point
    size_t vertex_offset = point_idx * vertices_per_instance;
    transform_template_for_point(template_mesh, point_position.value(), normal,
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

      // Set face attributes
      output_data->attributes.set_attribute("material_id", output_face_idx,
                                            static_cast<int>(point_idx));
    }

    // Set vertex attributes for this instance
    for (size_t vertex_idx = 0; vertex_idx < vertices_per_instance;
         ++vertex_idx) {
      size_t output_vertex_idx = vertex_offset + vertex_idx;

      // Set position from transformed vertices
      core::Vector3 vertex_position(output_vertices(output_vertex_idx, 0),
                                    output_vertices(output_vertex_idx, 1),
                                    output_vertices(output_vertex_idx, 2));
      output_data->attributes.set_position(output_vertex_idx, vertex_position);

      // Set instance attributes
      output_data->attributes.set_attribute("instance_id", output_vertex_idx,
                                            static_cast<int>(point_idx));
      output_data->attributes.set_attribute("original_point_index",
                                            output_vertex_idx,
                                            static_cast<int>(point_idx));
      output_data->attributes.set_attribute("instance_scale", output_vertex_idx,
                                            point_scale);

      // Set color based on instance (for visualization)
      double color_ratio =
          static_cast<double>(point_idx) / static_cast<double>(point_count - 1);
      core::Vector3 instance_color(color_ratio, 0.3, 1.0 - color_ratio);
      output_data->attributes.set_color(output_vertex_idx, instance_color);

      // Set normal (simplified - should be transformed properly)
      output_data->attributes.set_normal(output_vertex_idx, normal);
    }
  }

  // Create final output mesh
  output_data->mesh = core::Mesh(output_vertices, output_faces);
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

float CopyToPointsSOP::calculate_point_scale(
    const core::GeometryAttributes &point_attrs, size_t point_index,
    const std::string &scale_attribute, float uniform_scale,
    bool use_point_scale) {

  if (!use_point_scale) {
    return uniform_scale;
  }

  // Try to get scale from specified attribute
  if (!scale_attribute.empty()) {
    if (scale_attribute == "point_index") {
      // Scale based on point index
      auto point_index_attr =
          point_attrs.get_attribute<int>("point_index", point_index);
      if (point_index_attr.has_value()) {
        // Scale from 0.1 to 2.0 based on index
        float scale_factor = DEFAULT_SCALE_MULTIPLIER +
                             (static_cast<float>(point_index_attr.value()) *
                              DEFAULT_SCALE_MULTIPLIER);
        return scale_factor * uniform_scale;
      }
    } else {
      // Try to get float attribute
      auto scale_value =
          point_attrs.get_attribute<float>(scale_attribute, point_index);
      if (scale_value.has_value()) {
        return scale_value.value() * uniform_scale;
      }
    }
  }

  // Default to uniform scale
  return uniform_scale;
}

} // namespace nodeflux::sop
