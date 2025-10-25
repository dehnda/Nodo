#pragma once

#include "nodeflux/core/geometry_container.hpp"
#include "sop_node.hpp"
#include <memory>

namespace nodeflux::sop {

/**
 * @brief Example SOP: Scale geometry uniformly or non-uniformly
 *
 * This example demonstrates the modern GeometryContainer workflow:
 * 1. Get input as GeometryContainer
 * 2. Clone or create new geometry
 * 3. Read/write attributes using typed accessors
 * 4. Use standard attribute helpers (positions(), normals(), etc.)
 * 5. Return GeometryContainer
 *
 * Key patterns shown:
 * - Working with point attributes ("P")
 * - Optional vertex attribute processing ("N")
 * - Type-safe attribute access
 * - Standard attribute helpers
 * - GeometryContainer cloning
 */
class ScaleSOP : public SOPNode {
public:
  explicit ScaleSOP(const std::string &name);
  ~ScaleSOP() override = default;

  // Parameter setters
  void set_scale(float sx, float sy, float sz);
  void set_uniform_scale(float scale);
  void set_scale_from_origin(bool enabled) { scale_from_origin_ = enabled; }
  void set_normalize_normals(bool enabled) { normalize_normals_ = enabled; }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override;

private:
  // Scale factors
  float scale_x_ = 1.0F;
  float scale_y_ = 1.0F;
  float scale_z_ = 1.0F;

  // Options
  bool scale_from_origin_ = true;
  bool normalize_normals_ = true;
};

} // namespace nodeflux::sop
