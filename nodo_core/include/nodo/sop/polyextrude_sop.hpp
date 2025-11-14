#pragma once

#include "nodo/core/geometry_container.hpp"
#include "nodo/core/mesh.hpp"
#include "nodo/sop/sop_node.hpp"

#include <memory>
#include <string>

namespace nodo::sop {

/**
 * @brief PolyExtrude - Extrude polygon faces or edges
 *
 * Creates extrusions by duplicating face/edge vertices and moving them along
 * normals or specified directions, generating side geometry.
 * Supports inset for creating beveled edges.
 */
class PolyExtrudeSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  /// Extrusion type modes
  enum class ExtrusionType {
    FACES = 0, ///< Extrude polygon faces (3+ vertex primitives)
    EDGES = 1, ///< Extrude edges (2 vertex primitives)
    POINTS = 2 ///< Extrude points (future feature)
  };

  explicit PolyExtrudeSOP(const std::string& name = "polyextrude");

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
  /**
   * @brief Extrude polygon faces (3+ vertices)
   */
  std::shared_ptr<core::GeometryContainer> extrude_faces();

  /**
   * @brief Extrude edges (2 vertices)
   */
  std::shared_ptr<core::GeometryContainer> extrude_edges();

  /**
   * @brief Extrude points (create line segments from points)
   */
  std::shared_ptr<core::GeometryContainer> extrude_points();

  float distance_ = 1.0F;
  float inset_ = 0.0F;
  bool individual_faces_ = true;
};

} // namespace nodo::sop
