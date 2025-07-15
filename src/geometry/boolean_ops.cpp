#include "../../include/nodeflux/geometry/boolean_ops.hpp"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/orient_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Surface_mesh.h>
#include <array>
#include <vector>

using K = CGAL::Exact_predicates_inexact_constructions_kernel;
using Point_3 = K::Point_3;
using Surface_mesh = CGAL::Surface_mesh<Point_3>;

namespace nodeflux::geometry {

// Thread-local storage for error reporting
thread_local core::Error BooleanOps::last_error_{
    core::ErrorCategory::Unknown, core::ErrorCode::Unknown, "No error"};

std::optional<core::Mesh> BooleanOps::union_meshes(const core::Mesh &a,
                                                   const core::Mesh &b) {
  if (!are_compatible(a, b)) {
    set_last_error(core::Error{
        core::ErrorCategory::Validation, core::ErrorCode::InvalidMesh,
        "Meshes are not compatible for boolean operations"});
    return std::nullopt;
  }

  return cgal_boolean_operation(a, b, 0); // 0 = union
}

std::optional<core::Mesh> BooleanOps::intersect_meshes(const core::Mesh &a,
                                                       const core::Mesh &b) {
  if (!are_compatible(a, b)) {
    set_last_error(core::Error{
        core::ErrorCategory::Validation, core::ErrorCode::InvalidMesh,
        "Meshes are not compatible for boolean operations"});
    return std::nullopt;
  }

  return cgal_boolean_operation(a, b, 1); // 1 = intersection
}

std::optional<core::Mesh> BooleanOps::difference_meshes(const core::Mesh &a,
                                                        const core::Mesh &b) {
  if (!are_compatible(a, b)) {
    set_last_error(core::Error{
        core::ErrorCategory::Validation, core::ErrorCode::InvalidMesh,
        "Meshes are not compatible for boolean operations"});
    return std::nullopt;
  }

  return cgal_boolean_operation(a, b, 2); // 2 = difference
}

const core::Error &BooleanOps::last_error() { return last_error_; }

bool BooleanOps::are_compatible(const core::Mesh &a,
                                const core::Mesh &b) noexcept {
  return validate_mesh(a) && validate_mesh(b);
}

bool BooleanOps::validate_mesh(const core::Mesh &mesh) {
  if (mesh.vertices().rows() < 3 || mesh.faces().rows() < 1) {
    set_last_error(core::Error{core::ErrorCategory::Validation,
                               core::ErrorCode::EmptyMesh,
                               "Mesh has insufficient vertices or faces"});
    return false;
  }

  // Basic validation - ensure faces reference valid vertices
  const int num_vertices = static_cast<int>(mesh.vertices().rows());
  for (int i = 0; i < mesh.faces().rows(); ++i) {
    for (int j = 0; j < 3; ++j) {
      if (mesh.faces()(i, j) >= num_vertices || mesh.faces()(i, j) < 0) {
        set_last_error(core::Error{core::ErrorCategory::Validation,
                                   core::ErrorCode::InvalidMesh,
                                   "Face references invalid vertex index"});
        return false;
      }
    }
  }

  return true;
}

std::optional<core::Mesh>
BooleanOps::cgal_boolean_operation(const core::Mesh &a, const core::Mesh &b,
                                   int operation_type) {

  try {
    // Convert Eigen meshes to CGAL Surface_mesh
    Surface_mesh cgal_a, cgal_b, result;

    // Convert mesh A
    std::vector<Point_3> points_a;
    std::vector<std::array<std::size_t, 3>> faces_a;

    for (int i = 0; i < a.vertices().rows(); ++i) {
      points_a.emplace_back(a.vertices()(i, 0), a.vertices()(i, 1),
                            a.vertices()(i, 2));
    }

    for (int i = 0; i < a.faces().rows(); ++i) {
      faces_a.push_back({static_cast<std::size_t>(a.faces()(i, 0)),
                         static_cast<std::size_t>(a.faces()(i, 1)),
                         static_cast<std::size_t>(a.faces()(i, 2))});
    }

    CGAL::Polygon_mesh_processing::orient_polygon_soup(points_a, faces_a);
    CGAL::Polygon_mesh_processing::polygon_soup_to_polygon_mesh(
        points_a, faces_a, cgal_a);

    // Convert mesh B
    std::vector<Point_3> points_b;
    std::vector<std::array<std::size_t, 3>> faces_b;
    points_b.reserve(b.vertices().rows());
    faces_b.reserve(b.faces().rows());

    for (int i = 0; i < b.vertices().rows(); ++i) {
      points_b.emplace_back(b.vertices()(i, 0), b.vertices()(i, 1),
                            b.vertices()(i, 2));
    }

    for (int i = 0; i < b.faces().rows(); ++i) {
      faces_b.push_back({static_cast<std::size_t>(b.faces()(i, 0)),
                         static_cast<std::size_t>(b.faces()(i, 1)),
                         static_cast<std::size_t>(b.faces()(i, 2))});
    }

    CGAL::Polygon_mesh_processing::orient_polygon_soup(points_b, faces_b);
    CGAL::Polygon_mesh_processing::polygon_soup_to_polygon_mesh(
        points_b, faces_b, cgal_b);

    // Ensure consistent orientation
    if (!CGAL::Polygon_mesh_processing::is_outward_oriented(cgal_a)) {
      CGAL::Polygon_mesh_processing::reverse_face_orientations(cgal_a);
    }
    if (!CGAL::Polygon_mesh_processing::is_outward_oriented(cgal_b)) {
      CGAL::Polygon_mesh_processing::reverse_face_orientations(cgal_b);
    }

    // Perform boolean operation using visitor pattern for better control
    bool success = false;

    switch (operation_type) {
    case 0: { // Union
      Surface_mesh temp_a = cgal_a;
      Surface_mesh temp_b = cgal_b;
      success = CGAL::Polygon_mesh_processing::corefine_and_compute_union(
          temp_a, temp_b, result);
      break;
    }
    case 1: { // Intersection
      Surface_mesh temp_a = cgal_a;
      Surface_mesh temp_b = cgal_b;
      success =
          CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(
              temp_a, temp_b, result);
      break;
    }
    case 2: { // Difference
      Surface_mesh temp_a = cgal_a;
      Surface_mesh temp_b = cgal_b;
      success = CGAL::Polygon_mesh_processing::corefine_and_compute_difference(
          temp_a, temp_b, result);
      break;
    }
    default:
      set_last_error(core::Error{core::ErrorCategory::CGAL,
                                 core::ErrorCode::Unknown,
                                 "Unknown boolean operation type"});
      return std::nullopt;
    }

    if (!success || result.is_empty()) {
      set_last_error(core::Error{
          core::ErrorCategory::CGAL, core::ErrorCode::BooleanOperationFailed,
          "CGAL boolean operation failed or returned empty result"});
      return std::nullopt;
    }

    // Convert result back to Eigen mesh
    core::Mesh output;

    const auto num_vertices = result.number_of_vertices();
    const auto num_faces = result.number_of_faces();

    output.vertices().resize(num_vertices, 3);
    output.faces().resize(num_faces, 3);

    // Copy vertices
    std::size_t v_idx = 0;
    for (auto vh : result.vertices()) {
      const auto &point = result.point(vh);
      output.vertices()(v_idx, 0) = point.x();
      output.vertices()(v_idx, 1) = point.y();
      output.vertices()(v_idx, 2) = point.z();
      ++v_idx;
    }

    // Copy faces
    std::size_t f_idx = 0;
    for (auto fh : result.faces()) {
      std::size_t i = 0;
      for (auto vh : vertices_around_face(result.halfedge(fh), result)) {
        output.faces()(f_idx, i) = static_cast<int>(vh.idx());
        ++i;
      }
      ++f_idx;
    }

    return output;

  } catch (const std::exception &e) {
    set_last_error(core::Error{core::ErrorCategory::CGAL,
                               core::ErrorCode::Unknown,
                               std::string("CGAL exception: ") + e.what()});
    return std::nullopt;
  }
}

void BooleanOps::set_last_error(const core::Error &error) {
  last_error_ = error;
}

} // namespace nodeflux::geometry
