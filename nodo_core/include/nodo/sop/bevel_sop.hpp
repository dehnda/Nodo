#pragma once

#include "nodo/core/geometry_container.hpp"
#include "nodo/sop/sop_node.hpp"
#include <memory>
#include <string>

namespace nodo::sop {

/**
 * @brief Bevel SOP - Creates beveled edges on mesh geometry
 *
 * Replaces sharp edges with smooth beveled faces to create rounded corners
 * and edges. Supports both edge and vertex beveling with customizable
 * width, segments, and profile.
 */
class BevelSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;
  static constexpr float DEFAULT_WIDTH = 0.1F;
  static constexpr int DEFAULT_SEGMENTS = 1;
  static constexpr float DEFAULT_PROFILE = 0.5F;

  enum class BevelType {
    Vertex = 0, // Bevel vertices (corners)
    Edge = 1,   // Bevel edges
    Face = 2    // Bevel faces (inset)
  };

  enum class LimitMethod {
    None = 0,  // No limit
    Angle = 1, // Limit by angle
    Weight = 2 // Limit by edge weight
  };

  explicit BevelSOP(const std::string &name = "bevel");

  /**
   * @brief Set bevel width
   */
  void set_width(float width) { set_parameter("width", width); }

  /**
   * @brief Set number of bevel segments
   */
  void set_segments(int segments) { set_parameter("segments", segments); }

  /**
   * @brief Set bevel profile (0.0 = linear, 0.5 = smooth, 1.0 = sharp)
   */
  void set_profile(float profile) { set_parameter("profile", profile); }

protected:
  /**
   * @brief Execute the bevel operation (SOPNode override)
   */
  std::shared_ptr<core::GeometryContainer> execute() override;
};

} // namespace nodo::sop
