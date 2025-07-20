#include "nodeflux/sop/array_sop.hpp"
#include "nodeflux/core/math.hpp"
#include "nodeflux/core/types.hpp"

#include <cmath>
#include <iostream>

namespace nodeflux::sop {

std::optional<core::Mesh> ArraySOP::process(const core::Mesh &input_mesh) {
  switch (array_type_) {
  case ArrayType::LINEAR:
    return create_linear_array(input_mesh);
  case ArrayType::RADIAL:
    return create_radial_array(input_mesh);
  case ArrayType::GRID:
    return create_grid_array(input_mesh);
  default:
    std::cerr << "ArraySOP: Unknown array type\n";
    return std::nullopt;
  }
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
          core::Vector3(radial_radius_ * std::cos(angle_rad),
                        radial_radius_ * std::sin(angle_rad), 0.0);
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

} // namespace nodeflux::sop
