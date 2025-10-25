#pragma once

#include "nodeflux/core/geometry_container.hpp"
#include "nodeflux/core/mesh.hpp"
#include "nodeflux/sop/sop_node.hpp"
#include <memory>
#include <string>

namespace nodeflux::sop {

/**
 * @brief PolyExtrude - Extrude individual polygon faces
 *
 * Creates extrusions by duplicating face vertices and moving them along face
 * normals, generating side geometry. Supports inset for creating beveled edges.
 */
class PolyExtrudeSOP : public SOPNode {
public:
  explicit PolyExtrudeSOP(const std::string &name = "polyextrude");

  // Parameters
  void set_distance(float distance) {
    if (distance_ != distance) {
      distance_ = distance;
      mark_dirty();
    }
  }

  void set_inset(float inset) {
    if (inset_ != inset) {
      inset_ = inset;
      mark_dirty();
    }
  }

  void set_individual_faces(bool individual) {
    if (individual_faces_ != individual) {
      individual_faces_ = individual;
      mark_dirty();
    }
  }

  // Getters
  float get_distance() const { return distance_; }
  float get_inset() const { return inset_; }
  bool get_individual_faces() const { return individual_faces_; }

protected:
  /**
   * @brief Execute the poly-extrude operation (SOPNode override)
   */
  std::shared_ptr<core::GeometryContainer> execute() override;

private:
  float distance_ = 1.0F;
  float inset_ = 0.0F;
  bool individual_faces_ = true;
};

} // namespace nodeflux::sop
