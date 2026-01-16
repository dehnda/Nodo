#pragma once

#include "nodo/core/attribute_types.hpp"

#include "../core/geometry_attributes.hpp"
#include "../core/geometry_container.hpp"
#include "../core/standard_attributes.hpp"
#include "sop_node.hpp"

#include <memory>
#include <random>

namespace nodo::sop {

/**
 * @brief Scatter random points on mesh surfaces
 *
 * This SOP generates random points distributed across the surface of input
 * geometry, with support for density control, seed values, and attribute-driven
 * distribution.
 *
 * Migrated to use unified attribute system (GeometryContainer).
 */
class ScatterSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

private:
  // Default scatter parameters
  static constexpr int DEFAULT_POINT_COUNT = 100;
  static constexpr int DEFAULT_SEED = 42;
  static constexpr float DEFAULT_DENSITY = 1.0F;

public:
  explicit ScatterSOP(const std::string& node_name = "scatter");

  /**
   * @brief Generate scattered points on input geometry
   * Uses new GeometryContainer attribute system
   */
  std::shared_ptr<core::GeometryContainer> execute() override {
    // Get input geometry from port 0 (execution engine uses numeric indices)
    auto input_data = get_input_data(0);
    if (input_data == nullptr) {
      return nullptr;
    }

    auto* input_geo = input_data->get_point_attribute_typed<core::Vec3f>("P");
    if (input_geo == nullptr || input_geo->size() == 0) {
      return nullptr;
    }

    // Get parameters
    int point_count = get_parameter<int>("point_count");
    int seed = get_parameter<int>("seed");
    float density = get_parameter<float>("density");
    bool use_face_area = get_parameter<bool>("use_face_area");

    // Create output GeometryContainer
    auto output_geo = std::make_shared<core::GeometryContainer>();

    // Generate scattered points using new attribute system
    scatter_points_on_mesh(*input_data, *output_geo, point_count, seed, density, use_face_area);

    return output_geo;
  }

  /**
   * @brief Scatter points across mesh surface (new attribute system)
   */
  void scatter_points_on_mesh(const core::GeometryContainer& input_geo, core::GeometryContainer& output_geo,
                              int point_count, int seed, float density, bool use_face_area);

private:
  /**
   * @brief Generate random point on triangle face
   */
  core::Vector3 random_point_on_triangle(const core::Vector3& v0, const core::Vector3& v1, const core::Vector3& v2,
                                         std::mt19937& generator);
};

} // namespace nodo::sop
