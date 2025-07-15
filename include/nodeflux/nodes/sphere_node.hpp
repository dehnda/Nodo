#pragma once

#include "../core/error.hpp"
#include "../core/mesh.hpp"
#include "../geometry/sphere_generator.hpp"
#include <optional>

namespace nodeflux::nodes {

/// @brief Node for generating sphere meshes
class SphereNode {
public:
  /// @brief Create a sphere node with UV sphere generation
  /// @param radius The radius of the sphere
  /// @param u_segments Number of horizontal segments
  /// @param v_segments Number of vertical segments
  SphereNode(double radius = 1.0, int u_segments = 32, int v_segments = 16);

  /// @brief Create a sphere node with icosphere generation
  /// @param radius The radius of the sphere
  /// @param subdivisions Number of subdivision levels
  /// @return SphereNode configured for icosphere generation
  static SphereNode create_icosphere(double radius = 1.0, int subdivisions = 2);

  /// @brief Generate the sphere mesh
  /// @return Generated mesh or nullopt on error
  std::optional<core::Mesh> generate();

  /// @brief Get the last error that occurred
  /// @return Reference to the last error
  const core::Error &last_error() const;

  // Parameter setters
  void set_radius(double radius) { radius_ = radius; }
  void set_u_segments(int segments) { u_segments_ = segments; }
  void set_v_segments(int segments) { v_segments_ = segments; }
  void set_subdivisions(int subdivisions) { subdivisions_ = subdivisions; }
  void set_use_icosphere(bool use_ico) { use_icosphere_ = use_ico; }

  // Parameter getters
  double radius() const { return radius_; }
  int u_segments() const { return u_segments_; }
  int v_segments() const { return v_segments_; }
  int subdivisions() const { return subdivisions_; }
  bool use_icosphere() const { return use_icosphere_; }

private:
  double radius_;
  int u_segments_;
  int v_segments_;
  int subdivisions_;
  bool use_icosphere_;
};

} // namespace nodeflux::nodes
