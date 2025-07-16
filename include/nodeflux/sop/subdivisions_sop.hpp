#pragma once

#include "nodeflux/core/mesh.hpp"
#include <Eigen/Dense>
#include <optional>
#include <string>

namespace nodeflux::sop {

/**
 * @brief Subdivision Surface SOP - Applies Catmull-Clark subdivision to meshes
 *
 * Subdivides mesh faces to create smoother surfaces with higher polygon
 * density.
 */
class SubdivisionSOP {
private:
  std::string name_;
  int subdivision_levels_ = 1;
  bool preserve_boundaries_ = true;

public:
  explicit SubdivisionSOP(std::string name = "subdivision")
      : name_(std::move(name)) {}

  const std::string &get_name() const { return name_; }

  // Configuration methods
  void set_subdivision_levels(int levels) {
    subdivision_levels_ =
        std::max(0, std::min(levels, 4)); // Limit to reasonable range
  }
  void set_preserve_boundaries(bool preserve) {
    preserve_boundaries_ = preserve;
  }

  // Getters
  int get_subdivision_levels() const { return subdivision_levels_; }
  bool get_preserve_boundaries() const { return preserve_boundaries_; }

  /**
   * @brief Apply subdivision to input mesh
   */
  std::optional<core::Mesh> process(const core::Mesh &input_mesh);

private:
  std::optional<core::Mesh>
  apply_catmull_clark_subdivision(const core::Mesh &mesh);
};

} // namespace nodeflux::sop
