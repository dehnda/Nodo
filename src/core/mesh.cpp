#include "nodeflux/core/mesh.hpp"
#include <Eigen/Geometry>
#include <iostream>

namespace nodeflux::core {

Mesh::Mesh(Vertices vertices, Faces faces)
    : vertices_(std::move(vertices)), faces_(std::move(faces)) {}

const Mesh::Normals &Mesh::face_normals() const {
  if (!face_normals_) {
    compute_face_normals();
  }
  return *face_normals_;
}

const Mesh::Normals &Mesh::vertex_normals() const {
  if (!vertex_normals_) {
    compute_vertex_normals();
  }
  return *vertex_normals_;
}

double Mesh::volume() const {
  if (!volume_) {
    compute_volume();
  }
  return *volume_;
}

double Mesh::surface_area() const {
  if (!surface_area_) {
    compute_surface_area();
  }
  return *surface_area_;
}

bool Mesh::is_valid() const {
  if (empty())
    return false;

  // Check that all face indices are valid
  const int max_vertex_index = static_cast<int>(vertices_.rows()) - 1;
  for (int i = 0; i < faces_.rows(); ++i) {
    for (int j = 0; j < 3; ++j) {
      if (faces_(i, j) < 0 || faces_(i, j) > max_vertex_index) {
        return false;
      }
    }
  }
  return true;
}

bool Mesh::is_manifold() const {
  // Simplified manifold check - would need edge-based topology for full check
  return is_valid() && !empty();
}

bool Mesh::is_closed() const {
  // Simplified closed check - would need proper edge analysis
  return is_valid() && face_count() > 0;
}

bool Mesh::is_watertight() const { return is_closed() && is_manifold(); }

void Mesh::transform(const Eigen::Affine3d &transform) {
  if (!empty()) {
    for (int i = 0; i < vertices_.rows(); ++i) {
      vertices_.row(i) = (transform * vertices_.row(i).transpose()).transpose();
    }
    invalidate_cache();
  }
}

void Mesh::translate(const Vector3d &translation) {
  if (!empty()) {
    for (int i = 0; i < vertices_.rows(); ++i) {
      vertices_.row(i) += translation.transpose();
    }
    invalidate_cache();
  }
}

void Mesh::scale(double factor) {
  if (!empty()) {
    vertices_ *= factor;
    invalidate_cache();
  }
}

void Mesh::scale(const Vector3d &factors) {
  if (!empty()) {
    for (int i = 0; i < vertices_.rows(); ++i) {
      vertices_.row(i) = vertices_.row(i).cwiseProduct(factors.transpose());
    }
    invalidate_cache();
  }
}

Mesh Mesh::transformed(const Eigen::Affine3d &transform) const {
  Mesh result = *this;
  result.transform(transform);
  return result;
}

Mesh Mesh::translated(const Vector3d &translation) const {
  Mesh result = *this;
  result.translate(translation);
  return result;
}

Mesh Mesh::scaled(double factor) const {
  Mesh result = *this;
  result.scale(factor);
  return result;
}

Mesh Mesh::scaled(const Vector3d &factors) const {
  Mesh result = *this;
  result.scale(factors);
  return result;
}

void Mesh::clear() {
  vertices_.resize(0, 3);
  faces_.resize(0, 3);
  invalidate_cache();
}

void Mesh::reserve_vertices(size_t count) {
  vertices_.conservativeResize(count, 3);
}

void Mesh::reserve_faces(size_t count) { faces_.conservativeResize(count, 3); }

bool Mesh::operator==(const Mesh &other) const {
  return vertices_.isApprox(other.vertices_) && faces_ == other.faces_;
}

void Mesh::invalidate_cache() const noexcept {
  face_normals_.reset();
  vertex_normals_.reset();
  volume_.reset();
  surface_area_.reset();
}

void Mesh::compute_face_normals() const {
  if (empty()) {
    face_normals_ = Normals::Zero(0, 3);
    return;
  }

  face_normals_ = Normals::Zero(faces_.rows(), 3);

  for (int i = 0; i < faces_.rows(); ++i) {
    const Vector3d v0 = vertices_.row(faces_(i, 0));
    const Vector3d v1 = vertices_.row(faces_(i, 1));
    const Vector3d v2 = vertices_.row(faces_(i, 2));

    const Vector3d normal = (v2 - v0).cross(v1 - v0).normalized();
    face_normals_->row(i) = normal;
  }
}

void Mesh::compute_vertex_normals() const {
  if (empty()) {
    vertex_normals_ = Normals::Zero(0, 3);
    return;
  }

  // Ensure face normals are computed
  if (!face_normals_) {
    compute_face_normals();
  }

  vertex_normals_ = Normals::Zero(vertices_.rows(), 3);

  // Accumulate face normals for each vertex
  for (int i = 0; i < faces_.rows(); ++i) {
    const Vector3d &face_normal = face_normals_->row(i);
    for (int j = 0; j < 3; ++j) {
      const int vertex_idx = faces_(i, j);
      vertex_normals_->row(vertex_idx) += face_normal;
    }
  }

  // Normalize vertex normals
  for (int i = 0; i < vertex_normals_->rows(); ++i) {
    const double norm = vertex_normals_->row(i).norm();
    if (norm > 1e-12) {
      vertex_normals_->row(i) /= norm;
    }
  }
}

void Mesh::compute_volume() const {
  if (empty()) {
    volume_ = 0.0;
    return;
  }

  double vol = 0.0;
  for (int i = 0; i < faces_.rows(); ++i) {
    const Vector3d v0 = vertices_.row(faces_(i, 0));
    const Vector3d v1 = vertices_.row(faces_(i, 1));
    const Vector3d v2 = vertices_.row(faces_(i, 2));

    // Volume contribution of tetrahedron formed by origin and triangle
    vol += v0.dot(v1.cross(v2));
  }

  volume_ = std::abs(vol) / 6.0;
}

void Mesh::compute_surface_area() const {
  if (empty()) {
    surface_area_ = 0.0;
    return;
  }

  double area = 0.0;
  for (int i = 0; i < faces_.rows(); ++i) {
    const Vector3d v0 = vertices_.row(faces_(i, 0));
    const Vector3d v1 = vertices_.row(faces_(i, 1));
    const Vector3d v2 = vertices_.row(faces_(i, 2));

    // Area of triangle
    area += 0.5 * (v1 - v0).cross(v2 - v0).norm();
  }

  surface_area_ = area;
}

} // namespace nodeflux::core
