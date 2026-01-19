#pragma once

#include "sop_node.hpp"

#include <memory>

namespace nodo::sop {

/**
 * @brief UV Unwrap SOP - Automatic UV unwrapping using xatlas
 *
 * Generates UV coordinates for a mesh using automatic unwrapping.
 * Creates vertex attribute "uv" (VEC2F) suitable for texture mapping.
 */
class UVUnwrapSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit UVUnwrapSOP(const std::string& name = "uv_unwrap");
  ~UVUnwrapSOP() override = default;

  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override;

private:
  /**
   * @brief Convert GeometryContainer to xatlas mesh format
   */
  void geometry_to_xatlas(const core::GeometryContainer& geo, void* xatlas_mesh_decl);

  /**
   * @brief Extract UV coordinates from xatlas back to GeometryContainer
   */
  void xatlas_to_geometry(const void* xatlas_atlas, core::GeometryContainer& geo);
};

} // namespace nodo::sop
