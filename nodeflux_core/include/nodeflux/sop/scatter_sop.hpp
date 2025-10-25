#pragma once

#include "../core/geometry_attributes.hpp"
#include "../core/geometry_container.hpp"
#include "../core/standard_attributes.hpp"
#include "sop_node.hpp"
#include <random>

namespace nodeflux::sop {

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
private:
  // Default scatter parameters
  static constexpr int DEFAULT_POINT_COUNT = 100;
  static constexpr int DEFAULT_SEED = 42;
  static constexpr float DEFAULT_DENSITY = 1.0F;

public:
  explicit ScatterSOP(const std::string &node_name = "scatter");

  /**
   * @brief Generate scattered points on input geometry
   * Uses new GeometryContainer attribute system
   */
  std::shared_ptr<GeometryData> execute() override {
    auto input_data = get_input_data("input");
    if (input_data == nullptr) {
      return nullptr;
    }

    auto input_mesh = input_data->get_mesh();
    if (!input_mesh || input_mesh->empty()) {
      return nullptr;
    }

    // Get parameters
    int point_count = get_parameter<int>("point_count");
    int seed = get_parameter<int>("seed");
    float density = get_parameter<float>("density");
    bool use_face_area = get_parameter<bool>("use_face_area");

    // Convert input to GeometryContainer
    auto input_container = convert_to_container(*input_data);
    if (!input_container) {
      return nullptr;
    }

    // Create output GeometryContainer
    core::GeometryContainer output_geo;

    // Generate scattered points using new attribute system
    scatter_points_on_mesh(*input_container, output_geo, point_count, seed,
                           density, use_face_area);

    // Convert to GeometryData for compatibility (temporary bridge)
    return convert_from_container(output_geo);
  }

  /**
   * @brief Scatter points across mesh surface (new attribute system)
   */
  void scatter_points_on_mesh(const core::GeometryContainer &input_geo,
                              core::GeometryContainer &output_geo,
                              int point_count, int seed, float density,
                              bool use_face_area);

  /**
   * @brief Convert old GeometryData to GeometryContainer (temporary bridge)
   */
  std::unique_ptr<core::GeometryContainer>
  convert_to_container(const GeometryData &old_data);

  /**
   * @brief Convert GeometryContainer to GeometryData (temporary bridge)
   *
   * This allows compatibility with existing pipeline while we migrate SOPs.
   * Public so execution engine can use it during migration phase.
   */
  std::shared_ptr<GeometryData>
  convert_from_container(const core::GeometryContainer &container);

private:
  /**
   * @brief Calculate face areas for area-weighted distribution
   */
  std::vector<double> calculate_face_areas(const core::Mesh &mesh);

  /**
   * @brief Calculate face areas from GeometryContainer
   */
  std::vector<double>
  calculate_face_areas_from_container(const core::GeometryContainer &geo);

  /**
   * @brief Generate random point on triangle face
   */
  core::Vector3 random_point_on_triangle(const core::Vector3 &v0,
                                         const core::Vector3 &v1,
                                         const core::Vector3 &v2,
                                         std::mt19937 &generator);
};

} // namespace nodeflux::sop
