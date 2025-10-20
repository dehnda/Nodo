#pragma once

#include "nodeflux/core/mesh.hpp"
#include <memory>
#include <optional>
#include <string>

namespace nodeflux::sop {

/**
 * @brief PolyExtrude - Extrude individual polygon faces
 *
 * Creates extrusions by duplicating face vertices and moving them along face
 * normals, generating side geometry. Supports inset for creating beveled edges.
 */
class PolyExtrudeSOP {
public:
  explicit PolyExtrudeSOP(std::string name);

  // Input mesh
  void set_input_mesh(std::shared_ptr<core::Mesh> mesh);

  // Parameters
  void set_distance(float distance);
  void set_inset(float inset);
  void set_individual_faces(bool individual);

  // Execution
  std::optional<core::Mesh> execute();
  std::shared_ptr<core::Mesh> cook();

  // Query
  [[nodiscard]] const std::string &name() const { return name_; }
  [[nodiscard]] bool is_dirty() const { return is_dirty_; }

private:
  void mark_dirty() { is_dirty_ = true; }

  std::string name_;
  std::shared_ptr<core::Mesh> input_mesh_;
  std::shared_ptr<core::Mesh> cached_result_;

  float distance_{1.0F};
  float inset_{0.0F};
  bool individual_faces_{true};
  bool is_dirty_{true};
};

} // namespace nodeflux::sop
