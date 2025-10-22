#include "nodeflux/sop/array_sop.hpp"
#include "nodeflux/core/math.hpp"
#include "nodeflux/core/types.hpp"
#include "nodeflux/sop/geometry_data.hpp"

#include <cmath>
#include <iostream>

namespace nodeflux::sop {

std::shared_ptr<GeometryData> ArraySOP::execute() {
  // Get input geometry from port
  auto input_geo = get_input_data("mesh");
  if (!input_geo) {
    set_error("No input geometry connected");
    return nullptr;
  }

  // Extract mesh from GeometryData
  auto input_mesh = input_geo->get_mesh();
  if (!input_mesh) {
    set_error("Input geometry does not contain a mesh");
    return nullptr;
  }

  // Store input sizes for attribute tracking
  size_t input_vert_count = input_mesh->vertices().rows();
  size_t input_face_count = input_mesh->faces().rows();

  // Execute the appropriate array operation
  std::optional<core::Mesh> result_mesh;
  int effective_count = count_;

  switch (array_type_) {
  case ArrayType::LINEAR:
    result_mesh = create_linear_array(*input_mesh);
    break;
  case ArrayType::RADIAL:
    result_mesh = create_radial_array(*input_mesh);
    break;
  case ArrayType::GRID:
    effective_count = grid_size_.x() * grid_size_.y();
    result_mesh = create_grid_array(*input_mesh);
    break;
  default:
    set_error("Unknown array type");
    return nullptr;
  }

  // Check if operation succeeded
  if (!result_mesh) {
    set_error("Array operation failed");
    return nullptr;
  }

  // Wrap result in GeometryData
  auto output_mesh = std::make_shared<core::Mesh>(std::move(*result_mesh));
  auto output_data = std::make_shared<GeometryData>(output_mesh);

  // Add instance tracking attributes
  add_instance_attributes(output_data, input_vert_count, input_face_count,
                          effective_count);

  return output_data;
}

std::optional<core::Mesh>
ArraySOP::create_linear_array(const core::Mesh &input_mesh) {
  const auto &input_vertices = input_mesh.vertices();
  const auto &input_faces = input_mesh.faces();

  // Calculate total size
  int total_vertices = input_vertices.rows() * count_;
  int total_faces = input_faces.rows() * count_;

  // Prepare output matrices
  core::Mesh::Vertices output_vertices(total_vertices, 3);
  core::Mesh::Faces output_faces(total_faces, 3);

  // Copy geometry for each array element
  for (int i = 0; i < count_; ++i) {
    // Calculate offset for this copy
    core::Vector3 current_offset =
        (linear_offset_ * static_cast<float>(i)).cast<double>();

    // Copy vertices with offset
    int vertex_start = i * input_vertices.rows();
    for (int v = 0; v < input_vertices.rows(); ++v) {
      output_vertices.row(vertex_start + v) =
          input_vertices.row(v) + current_offset.transpose();
    }

    // Copy faces with vertex index offset
    int face_start = i * input_faces.rows();
    int vertex_offset = i * input_vertices.rows();
    for (int f = 0; f < input_faces.rows(); ++f) {
      output_faces.row(face_start + f) =
          input_faces.row(f).array() + vertex_offset;
    }
  }

  return core::Mesh(std::move(output_vertices), std::move(output_faces));
}

std::optional<core::Mesh>
ArraySOP::create_radial_array(const core::Mesh &input_mesh) {
  const auto &input_vertices = input_mesh.vertices();
  const auto &input_faces = input_mesh.faces();

  // Calculate total size
  int total_vertices = input_vertices.rows() * count_;
  int total_faces = input_faces.rows() * count_;

  // Prepare output matrices
  core::Mesh::Vertices output_vertices(total_vertices, 3);
  core::Mesh::Faces output_faces(total_faces, 3);

  // Copy geometry for each array element
  for (int i = 0; i < count_; ++i) {
    // Calculate rotation angle
    double angle_rad = core::math::degrees_to_radians(angle_step_ * i);
    auto rotation = core::math::rotation_z(angle_rad);

    // Calculate position offset (radial positioning)
    core::Vector3 position_offset = radial_center_.cast<double>();
    if (radial_radius_ > 0.0F) {
      position_offset +=
          core::math::circular_offset_2d(radial_radius_, angle_rad);
    }

    // Copy vertices with rotation and translation
    int vertex_start = i * input_vertices.rows();
    for (int v = 0; v < input_vertices.rows(); ++v) {
      // Get the original vertex and transform it
      core::Vector3 original_vertex = input_vertices.row(v).transpose();
      core::Vector3 transformed_vertex = core::math::apply_transform(
          original_vertex, rotation, position_offset);
      output_vertices.row(vertex_start + v) = transformed_vertex.transpose();
    }

    // Copy faces with vertex index offset
    int face_start = i * input_faces.rows();
    int vertex_offset = i * input_vertices.rows();
    for (int f = 0; f < input_faces.rows(); ++f) {
      output_faces.row(face_start + f) =
          input_faces.row(f).array() + vertex_offset;
    }
  }

  return core::Mesh(std::move(output_vertices), std::move(output_faces));
}

std::optional<core::Mesh>
ArraySOP::create_grid_array(const core::Mesh &input_mesh) {
  const auto &input_vertices = input_mesh.vertices();
  const auto &input_faces = input_mesh.faces();

  int grid_count = grid_size_.x() * grid_size_.y();

  // Calculate total size
  int total_vertices = input_vertices.rows() * grid_count;
  int total_faces = input_faces.rows() * grid_count;

  // Prepare output matrices
  core::Mesh::Vertices output_vertices(total_vertices, 3);
  core::Mesh::Faces output_faces(total_faces, 3);

  int copy_index = 0;

  // Copy geometry for each grid position
  for (int y = 0; y < grid_size_.y(); ++y) {
    for (int x = 0; x < grid_size_.x(); ++x) {
      // Calculate grid position offset
      core::Vector3 grid_offset(x * grid_spacing_.x(), y * grid_spacing_.y(),
                                0.0);

      // Copy vertices with offset
      int vertex_start = copy_index * input_vertices.rows();
      for (int v = 0; v < input_vertices.rows(); ++v) {
        output_vertices.row(vertex_start + v) =
            input_vertices.row(v) + grid_offset.transpose();
      }

      // Copy faces with vertex index offset
      int face_start = copy_index * input_faces.rows();
      int vertex_offset = copy_index * input_vertices.rows();
      for (int f = 0; f < input_faces.rows(); ++f) {
        output_faces.row(face_start + f) =
            input_faces.row(f).array() + vertex_offset;
      }

      copy_index++;
    }
  }

  return core::Mesh(std::move(output_vertices), std::move(output_faces));
}

void ArraySOP::add_instance_attributes(std::shared_ptr<GeometryData> geo_data,
                                        size_t verts_per_instance,
                                        size_t faces_per_instance,
                                        int instance_count) {
  // Add per-vertex instance ID attribute
  GeometryData::AttributeArray vertex_instance_ids;
  vertex_instance_ids.reserve(verts_per_instance * instance_count);

  for (int copy = 0; copy < instance_count; ++copy) {
    for (size_t v = 0; v < verts_per_instance; ++v) {
      vertex_instance_ids.push_back(copy);
    }
  }
  geo_data->set_vertex_attribute("instance_id", vertex_instance_ids);

  // Add per-face instance ID attribute
  GeometryData::AttributeArray face_instance_ids;
  face_instance_ids.reserve(faces_per_instance * instance_count);

  for (int copy = 0; copy < instance_count; ++copy) {
    for (size_t f = 0; f < faces_per_instance; ++f) {
      face_instance_ids.push_back(copy);
    }
  }
  geo_data->set_face_attribute("instance_id", face_instance_ids);

  // Add global metadata
  geo_data->set_global_attribute("instance_count", instance_count);
  geo_data->set_global_attribute("array_type", static_cast<int>(array_type_));
}

} // namespace nodeflux::sop
