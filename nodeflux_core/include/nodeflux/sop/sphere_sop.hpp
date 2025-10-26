#pragma once

#include "../core/geometry_container.hpp"
#include "../geometry/sphere_generator.hpp"
#include "sop_node.hpp"

namespace nodeflux {
namespace sop {

/**
 * @brief Sphere generator SOP node
 *
 * Generates UV spheres with smooth shading. The sphere is created with
 * point-based normals (averaged across shared vertices) for smooth appearance.
 *
 * ## Hard Edges (Future Feature)
 * To implement hard edges (faceted look), we would need to:
 * 1. Add a "cusp_angle" parameter (angle threshold for hard vs soft edges)
 * 2. Split vertices where adjacent face normals differ by > cusp_angle
 * 3. Store normals as VERTEX attributes (not POINT attributes)
 * 4. Each face corner gets its own vertex with the face normal
 *
 * Example workflow:
 * ```
 * // In execute():
 * auto container = sphere_generator->generate();
 * if (cusp_angle < 180.0f) {
 *   utils::compute_hard_edges(*container, cusp_angle);
 * }
 * ```
 *
 * This requires implementing vertex attribute support in GeometryContainer,
 * which is currently set up for it but not fully utilized yet.
 */
class SphereSOP : public SOPNode {
private:
  // Default sphere parameters
  static constexpr float DEFAULT_RADIUS = 1.0F;
  static constexpr int DEFAULT_SEGMENTS = 32;
  static constexpr int DEFAULT_RINGS = 16;

public:
  explicit SphereSOP(const std::string &node_name = "sphere")
      : SOPNode(node_name, "SphereSOP") {

    // Set default parameters
    set_parameter("radius", DEFAULT_RADIUS);
    set_parameter("segments", DEFAULT_SEGMENTS);
    set_parameter("rings", DEFAULT_RINGS);
  }

  /**
   * @brief Set sphere radius
   */
  void set_radius(float radius) { set_parameter("radius", radius); }

  /**
   * @brief Set sphere resolution
   */
  void set_resolution(int segments, int rings) {
    set_parameter("segments", segments);
    set_parameter("rings", rings);
  }

protected:
  /**
   * @brief Execute sphere generation
   */
  std::shared_ptr<core::GeometryContainer> execute() override {
    const float radius = get_parameter<float>("radius", DEFAULT_RADIUS);
    const int segments = get_parameter<int>("segments", DEFAULT_SEGMENTS);
    const int rings = get_parameter<int>("rings", DEFAULT_RINGS);

    try {
      // Generate sphere using CPU generator (returns GeometryContainer
      // directly)
      auto result = geometry::SphereGenerator::generate_uv_sphere(
          static_cast<double>(radius), segments, rings);

      if (!result.has_value()) {
        set_error("Sphere generation failed");
        return nullptr;
      }

      return std::make_shared<core::GeometryContainer>(
          std::move(result.value()));

    } catch (const std::exception &exception) {
      set_error("Exception during sphere generation: " +
                std::string(exception.what()));
      return nullptr;
    }
  }
};

} // namespace sop
} // namespace nodeflux
