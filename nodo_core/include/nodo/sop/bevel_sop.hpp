#pragma once

#include "nodo/core/attribute_types.hpp"
#include "nodo/core/geometry_container.hpp"
#include "nodo/sop/sop_node.hpp"

#include <cstdint>
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

  enum class BevelType : std::uint8_t {
    Vertex = 0,    // Bevel vertices (corners)
    Edge = 1,      // Bevel edges
    Face = 2,      // Bevel faces (inset)
    EdgeVertex = 3 // Combined: edges + stitched vertex corners
  };

  // Corner patch topology when in Vertex mode
  enum class CornerStyle : std::uint8_t {
    ApexFan = 0,   // Current behavior: apex vertex + fan + ring quads
    RingStart = 1, // Remove apex fan; start patch at first ring (n-gon or quads)
    Grid = 2       // (Planned) Grid style spherical-ish patch (segments^2 like)
  };

  enum class LimitMethod : std::uint8_t {
    None = 0,  // No limit
    Angle = 1, // Limit by angle
    Weight = 2 // Limit by edge weight
  };

  explicit BevelSOP(const std::string& name = "bevel");

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
