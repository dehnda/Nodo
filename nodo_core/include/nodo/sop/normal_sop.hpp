#pragma once

#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief Compute surface normals for geometry
 *
 * Supports multiple normal computation modes:
 * - Vertex normals (smooth shading)
 * - Face normals (flat shading)
 * - Point normals (shared)
 *
 * With various weighting schemes and cusp angle control
 */
class NormalSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit NormalSOP(const std::string& name = "normal");

  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override;

private:
  void compute_vertex_normals(core::GeometryContainer& geo, int weighting, float cusp_angle, bool reverse) const;

  void compute_face_normals(core::GeometryContainer& geo, bool reverse) const;

  void compute_point_normals(core::GeometryContainer& geo, int weighting, float cusp_angle, bool reverse) const;
};

} // namespace nodo::sop
