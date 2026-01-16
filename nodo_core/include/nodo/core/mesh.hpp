#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <memory>
#include <optional>
#include <vector>

namespace nodo::core {

/**
 * @brief Modern mesh representation with value semantics and lazy evaluation
 *
 * This class provides a clean, efficient interface for 3D mesh data with
 * automatic caching of computed properties and RAII resource management.
 */
class Mesh {
public:
  using Vertices = Eigen::Matrix<double, Eigen::Dynamic, 3, Eigen::RowMajor>;
  using Faces = Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor>;
  using Normals = Eigen::Matrix<double, Eigen::Dynamic, 3, Eigen::RowMajor>;
  using Vector3d = Eigen::Vector3d;

  // Constructors
  Mesh() = default;
  Mesh(Vertices vertices, Faces faces);
  Mesh(const Mesh&) = default;
  Mesh(Mesh&&) = default;
  Mesh& operator=(const Mesh&) = default;
  Mesh& operator=(Mesh&&) = default;
  ~Mesh() = default;

  // Const access to data
  [[nodiscard]] const Vertices& vertices() const noexcept { return vertices_; }
  [[nodiscard]] const Faces& faces() const noexcept { return faces_; }

  // Mutable access (invalidates cache)
  Vertices& vertices() {
    invalidate_cache();
    return vertices_;
  }
  Faces& faces() {
    invalidate_cache();
    return faces_;
  }

  // Computed properties (cached)
  [[nodiscard]] const Normals& face_normals() const;
  [[nodiscard]] const Normals& vertex_normals() const;
  [[nodiscard]] double volume() const;
  [[nodiscard]] double surface_area() const;

  // Basic queries
  [[nodiscard]] bool empty() const noexcept {
    // A mesh is empty only if it has no vertices
    // Point clouds have vertices but no faces, and are NOT empty
    return vertices_.rows() == 0;
  }
  [[nodiscard]] size_t vertex_count() const noexcept { return vertices_.rows(); }
  [[nodiscard]] size_t face_count() const noexcept { return faces_.rows(); }

  // Mesh validation
  [[nodiscard]] bool is_valid() const;
  [[nodiscard]] bool is_manifold() const;
  [[nodiscard]] bool is_closed() const;
  [[nodiscard]] bool is_watertight() const;

  // In-place transformations
  void transform(const Eigen::Affine3d& transform);
  void translate(const Vector3d& translation);
  void scale(double factor);
  void scale(const Vector3d& factors);

  // Mesh operations that return new meshes
  [[nodiscard]] Mesh transformed(const Eigen::Affine3d& transform) const;
  [[nodiscard]] Mesh translated(const Vector3d& translation) const;
  [[nodiscard]] Mesh scaled(double factor) const;
  [[nodiscard]] Mesh scaled(const Vector3d& factors) const;

  // Utility methods
  void clear();
  void reserve_vertices(size_t count);
  void reserve_faces(size_t count);

  // Equality comparison
  bool operator==(const Mesh& other) const;
  bool operator!=(const Mesh& other) const { return !(*this == other); }

private:
  Vertices vertices_;
  Faces faces_;

  // Mutable cached computations
  mutable std::optional<Normals> face_normals_;
  mutable std::optional<Normals> vertex_normals_;
  mutable std::optional<double> volume_;
  mutable std::optional<double> surface_area_;

  void invalidate_cache() const noexcept;
  void compute_face_normals() const;
  void compute_vertex_normals() const;
  void compute_volume() const;
  void compute_surface_area() const;
};

} // namespace nodo::core
