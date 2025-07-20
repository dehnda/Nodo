#include "nodeflux/sop/mirror_sop.hpp"
#include "nodeflux/core/math.hpp"
#include <chrono>
#include <iostream>

namespace nodeflux::sop {

MirrorSOP::MirrorSOP(std::string name) : name_(std::move(name)) {}

void MirrorSOP::set_plane(MirrorPlane plane) {
  if (plane_ != plane) {
    plane_ = plane;
    mark_dirty();
  }
}

void MirrorSOP::set_custom_plane(const core::Vector3 &point,
                                 const core::Vector3 &normal) {
  if (custom_point_ != point || custom_normal_ != normal) {
    custom_point_ = point;
    custom_normal_ = normal.normalized();
    mark_dirty();
  }
}

void MirrorSOP::set_keep_original(bool keep_original) {
  if (keep_original_ != keep_original) {
    keep_original_ = keep_original;
    mark_dirty();
  }
}

void MirrorSOP::set_input_mesh(std::shared_ptr<core::Mesh> mesh) {
  if (input_mesh_ != mesh) {
    input_mesh_ = std::move(mesh);
    mark_dirty();
  }
}

std::optional<core::Mesh> MirrorSOP::execute() {
  if (!input_mesh_) {
    return std::nullopt;
  }

  try {
    const auto &original_vertices = input_mesh_->vertices();
    const auto &original_faces = input_mesh_->faces();

    // Determine mirror plane
    core::Vector3 plane_point;
    core::Vector3 plane_normal;

    switch (plane_) {
    case MirrorPlane::XY:
      plane_point = core::Vector3::Zero();
      plane_normal = core::Vector3::UnitZ();
      break;
    case MirrorPlane::XZ:
      plane_point = core::Vector3::Zero();
      plane_normal = core::Vector3::UnitY();
      break;
    case MirrorPlane::YZ:
      plane_point = core::Vector3::Zero();
      plane_normal = core::Vector3::UnitX();
      break;
    case MirrorPlane::CUSTOM:
      plane_point = custom_point_;
      plane_normal = custom_normal_;
      break;
    default:
      return std::nullopt;
    }

    // Create mirrored vertices matrix
    core::Mesh::Vertices mirrored_vertices(original_vertices.rows(), 3);

    for (int i = 0; i < original_vertices.rows(); ++i) {
      core::Vector3 vertex = original_vertices.row(i);
      core::Vector3 mirrored = core::math::mirror_point_across_plane(
          vertex, plane_point, plane_normal);
      mirrored_vertices.row(i) = mirrored;
    }

    // Build result mesh
    if (keep_original_) {
      // Combine original and mirrored
      const int total_vertices =
          original_vertices.rows() + mirrored_vertices.rows();
      const int total_faces = original_faces.rows() + original_faces.rows();

      core::Mesh::Vertices result_vertices(total_vertices, 3);
      core::Mesh::Faces result_faces(total_faces, 3);

      // Copy original data
      result_vertices.topRows(original_vertices.rows()) = original_vertices;
      result_faces.topRows(original_faces.rows()) = original_faces;

      // Add mirrored vertices
      result_vertices.bottomRows(mirrored_vertices.rows()) = mirrored_vertices;

      // Add mirrored faces (with flipped winding)
      const int vertex_offset = original_vertices.rows();
      for (int i = 0; i < original_faces.rows(); ++i) {
        result_faces(original_faces.rows() + i, 0) =
            original_faces(i, 0) + vertex_offset;
        result_faces(original_faces.rows() + i, 1) =
            original_faces(i, 2) + vertex_offset; // Flip winding
        result_faces(original_faces.rows() + i, 2) =
            original_faces(i, 1) + vertex_offset;
      }

      return core::Mesh(std::move(result_vertices), std::move(result_faces));
    }

    // Only mirrored version with flipped faces
    core::Mesh::Faces flipped_faces(original_faces.rows(), 3);
    for (int i = 0; i < original_faces.rows(); ++i) {
      flipped_faces(i, 0) = original_faces(i, 0);
      flipped_faces(i, 1) = original_faces(i, 2); // Flip winding
      flipped_faces(i, 2) = original_faces(i, 1);
    }
    return core::Mesh(std::move(mirrored_vertices), std::move(flipped_faces));

  } catch (const std::exception &exception) {
    return std::nullopt;
  }
}

std::shared_ptr<core::Mesh> MirrorSOP::cook() {
  if (!is_dirty_ && cached_result_) {
    std::cout << "MirrorSOP '" << name_ << "': Using cached result\n";
    return cached_result_;
  }

  std::cout << "MirrorSOP '" << name_ << "': Computing mirror across "
            << plane_to_string(plane_) << " plane...\n";

  auto start_time = std::chrono::steady_clock::now();

  auto result = execute();
  if (!result) {
    std::cerr << "MirrorSOP '" << name_ << "': Operation failed\n";
    return nullptr;
  }

  cached_result_ = std::make_shared<core::Mesh>(std::move(*result));
  is_dirty_ = false;

  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);

  std::cout << "MirrorSOP '" << name_ << "': Completed in " << duration.count()
            << "ms\n";
  std::cout << "  Result: " << cached_result_->vertices().rows()
            << " vertices, " << cached_result_->faces().rows() << " faces\n";

  return cached_result_;
}

std::string MirrorSOP::plane_to_string(MirrorPlane plane) {
  switch (plane) {
  case MirrorPlane::XY:
    return "XY";
  case MirrorPlane::XZ:
    return "XZ";
  case MirrorPlane::YZ:
    return "YZ";
  case MirrorPlane::CUSTOM:
    return "Custom";
  default:
    return "Unknown";
  }
}

std::vector<core::Vector3>
MirrorSOP::mirror_vertices(const std::vector<core::Vector3> &vertices,
                           const core::Vector3 &plane_point,
                           const core::Vector3 &plane_normal) const {

  std::vector<core::Vector3> mirrored_vertices;
  mirrored_vertices.reserve(vertices.size());

  for (const auto &vertex : vertices) {
    // Use the new math utility function
    core::Vector3 mirrored = core::math::mirror_point_across_plane(
        vertex, plane_point, plane_normal);
    mirrored_vertices.push_back(mirrored);
  }

  return mirrored_vertices;
}

} // namespace nodeflux::sop
