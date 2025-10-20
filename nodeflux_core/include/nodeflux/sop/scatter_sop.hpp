#pragma once

#include "../core/geometry_attributes.hpp"
#include "sop_node.hpp"
#include <random>

namespace nodeflux::sop {

/**
 * @brief Scatter random points on mesh surfaces
 *
 * This SOP generates random points distributed across the surface of input
 * geometry, with support for density control, seed values, and attribute-driven
 * distribution.
 */
class ScatterSOP : public SOPNode {
private:
  // Default scatter parameters
  static constexpr int DEFAULT_POINT_COUNT = 100;
  static constexpr int DEFAULT_SEED = 42;
  static constexpr float DEFAULT_DENSITY = 1.0F;

public:
  explicit ScatterSOP(const std::string &node_name = "scatter")
      : SOPNode(node_name, "ScatterSOP") {

    // Set default parameters
    set_parameter("point_count", DEFAULT_POINT_COUNT);
    set_parameter("seed", DEFAULT_SEED);
    set_parameter("density", DEFAULT_DENSITY);
    set_parameter("use_face_area", true); // Weight by face area
  }

  /**
   * @brief Generate scattered points on input geometry
   */
  std::shared_ptr<GeometryData> get_output_data() override {
    auto input_data = get_input_data(0);
    if (input_data == nullptr) {
      return nullptr;
    }

    // Get parameters
    int point_count = get_parameter<int>("point_count");
    int seed = get_parameter<int>("seed");
    float density = get_parameter<float>("density");
    bool use_face_area = get_parameter<bool>("use_face_area");

    // Create output geometry
    auto output_data = std::make_shared<GeometryData>();

    // Generate scattered points
    scatter_points_on_mesh(input_data->mesh, *output_data, point_count, seed,
                           density, use_face_area);

    return output_data;
  }

private:
  /**
   * @brief Scatter points across mesh surface
   */
  void scatter_points_on_mesh(const core::Mesh &input_mesh,
                              GeometryData &output_data, int point_count,
                              int seed, float density, bool use_face_area);

  /**
   * @brief Calculate face areas for area-weighted distribution
   */
  std::vector<double> calculate_face_areas(const core::Mesh &mesh);

  /**
   * @brief Generate random point on triangle face
   */
  core::Vector3 random_point_on_triangle(const core::Vector3 &v0,
                                         const core::Vector3 &v1,
                                         const core::Vector3 &v2,
                                         std::mt19937 &generator);

  /**
   * @brief Interpolate attributes at scattered point
   */
  void interpolate_attributes_at_point(
      const core::GeometryAttributes &input_attrs,
      core::GeometryAttributes &output_attrs, int face_index,
      const core::Vector3 &barycentric_coords, size_t output_point_index);
};

} // namespace nodeflux::sop
