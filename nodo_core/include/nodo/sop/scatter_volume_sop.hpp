#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

#include <random>

namespace nodo::sop {

/**
 * @brief Scatter Volume SOP - Scatter points within a bounding box or volume
 *
 * Creates randomly distributed points within a 3D volume defined by a bounding
 * box. Useful for:
 * - Volume filling (particles, smoke, fog)
 * - Random point generation in space
 * - Procedural placement within bounds
 *
 * Different from ScatterSOP which scatters on surfaces.
 */
class ScatterVolumeSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit ScatterVolumeSOP(const std::string& node_name = "scatter_volume")
      : SOPNode(node_name, "Scatter Volume") {
    // Required input: use input geometry bounding box
    input_ports_.add_port("0", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);

    // Number of points
    register_parameter(define_int_parameter("count", 100)
                           .label("Point Count")
                           .range(1, 1000000)
                           .category("Distribution")
                           .description("Number of points to scatter")
                           .build());

    // Seed for repeatability
    register_parameter(define_int_parameter("seed", 12345)
                           .label("Random Seed")
                           .range(0, 999999)
                           .category("Distribution")
                           .description("Random seed for reproducible results")
                           .build());

    // Distribution mode
    register_parameter(define_int_parameter("distribution_mode", 0)
                           .label("Distribution")
                           .options({"Random", "Uniform Grid", "Poisson Disk"})
                           .category("Distribution")
                           .description("Point distribution algorithm")
                           .build());

    // Volume mode
    register_parameter(
        define_int_parameter("volume_mode", 0)
            .label("Volume Mode")
            .options({"Bounding Box", "Inside Mesh"})
            .category("Distribution")
            .description("Scatter in bounding box or only inside mesh volume")
            .build());

    // Minimum distance (for Poisson disk)
    register_parameter(
        define_float_parameter("min_distance", 0.1f)
            .label("Min Distance")
            .range(0.001f, 10.0f)
            .category("Distribution")
            .visible_when("distribution_mode", 2)
            .description("Minimum distance between points (Poisson disk)")
            .build());
  }

  InputConfig get_input_config() const override {
    // Required single input
    return InputConfig(InputType::SINGLE, 1, 1, 1);
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    // Get input geometry
    auto input = get_input_data(0);
    if (!input || input->point_count() == 0) {
      set_error("Scatter Volume: Input geometry required");
      return nullptr;
    }

    // Get parameters
    const int count = get_parameter<int>("count", 100);
    const int seed = get_parameter<int>("seed", 12345);
    const int distribution_mode = get_parameter<int>("distribution_mode", 0);
    const int volume_mode = get_parameter<int>("volume_mode", 0);

    // Compute bounding box from input geometry
    auto* P = input->get_point_attribute_typed<Eigen::Vector3f>("P");
    if (!P) {
      set_error("Scatter Volume: Input geometry has no P attribute");
      return nullptr;
    }

    Eigen::Vector3f min_bounds(std::numeric_limits<float>::max(),
                               std::numeric_limits<float>::max(),
                               std::numeric_limits<float>::max());
    Eigen::Vector3f max_bounds(std::numeric_limits<float>::lowest(),
                               std::numeric_limits<float>::lowest(),
                               std::numeric_limits<float>::lowest());

    for (size_t i = 0; i < input->point_count(); ++i) {
      const auto& pos = (*P)[i];
      min_bounds = min_bounds.cwiseMin(pos);
      max_bounds = max_bounds.cwiseMax(pos);
    }

    // Create output geometry
    auto output = std::make_shared<core::GeometryContainer>();
    output->set_point_count(count);
    output->add_point_attribute("P", core::AttributeType::VEC3F);
    auto* P_out = output->get_point_attribute_typed<Eigen::Vector3f>("P");

    // Random number generator
    std::mt19937 rng(static_cast<unsigned int>(seed));
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    // Generate points based on distribution mode
    if (distribution_mode == 0) {
      // Random distribution in bounding box
      scatter_in_box(rng, dist, count, min_bounds, max_bounds, P_out);

    } else if (distribution_mode == 1) {
      // Uniform grid
      scatter_uniform_grid(count, min_bounds, max_bounds, P_out);

    } else if (distribution_mode == 2) {
      // Poisson disk (simplified version)
      const float min_distance = get_parameter<float>("min_distance", 0.1f);
      scatter_poisson_disk(rng, count, min_bounds, max_bounds, min_distance,
                           P_out);
    }

    // Filter points if "Inside Mesh" mode is enabled
    if (volume_mode == 1) {
      const size_t filtered_count = filter_inside_mesh(input, P_out);
      // Update output geometry to match the filtered point count
      output->set_point_count(filtered_count);
    }

    return output;
  }

private:
  // Helper: Test if a point is inside a mesh using ray casting
  bool is_point_inside_mesh(const Eigen::Vector3f& point,
                            const core::GeometryContainer* geometry) const {
    // Get mesh topology and positions
    const auto& topo = geometry->topology();
    auto* positions = geometry->get_point_attribute_typed<Eigen::Vector3f>("P");
    if (!positions) {
      return false;
    }

    // Cast a ray in +X direction and count intersections
    const Eigen::Vector3f ray_dir(1.0f, 0.0f, 0.0f);
    int intersection_count = 0;

    // Check each triangle in the mesh
    for (size_t prim_idx = 0; prim_idx < topo.primitive_count(); ++prim_idx) {
      const auto& vert_indices = topo.get_primitive_vertices(prim_idx);

      // For each primitive, triangulate and test
      std::vector<int> point_indices;
      for (size_t vi : vert_indices) {
        point_indices.push_back(topo.get_vertex_point(vi));
      }

      // Test triangles (fan triangulation for n-gons)
      for (size_t i = 1; i + 1 < point_indices.size(); ++i) {
        const Eigen::Vector3f& v0 = (*positions)[point_indices[0]];
        const Eigen::Vector3f& v1 = (*positions)[point_indices[i]];
        const Eigen::Vector3f& v2 = (*positions)[point_indices[i + 1]];

        if (ray_intersects_triangle(point, ray_dir, v0, v1, v2)) {
          intersection_count++;
        }
      }
    }

    // Odd number of intersections means inside
    return (intersection_count % 2) == 1;
  }

  // Helper: Ray-triangle intersection test (Möller-Trumbore algorithm)
  bool ray_intersects_triangle(const Eigen::Vector3f& ray_origin,
                               const Eigen::Vector3f& ray_dir,
                               const Eigen::Vector3f& v0,
                               const Eigen::Vector3f& v1,
                               const Eigen::Vector3f& v2) const {
    const float EPSILON = 0.0000001f;

    Eigen::Vector3f edge1 = v1 - v0;
    Eigen::Vector3f edge2 = v2 - v0;
    Eigen::Vector3f h = ray_dir.cross(edge2);
    float a = edge1.dot(h);

    if (a > -EPSILON && a < EPSILON) {
      return false; // Ray is parallel to triangle
    }

    float f = 1.0f / a;
    Eigen::Vector3f s = ray_origin - v0;
    float u = f * s.dot(h);

    if (u < 0.0f || u > 1.0f) {
      return false;
    }

    Eigen::Vector3f q = s.cross(edge1);
    float v = f * ray_dir.dot(q);

    if (v < 0.0f || u + v > 1.0f) {
      return false;
    }

    // Check if intersection is in the positive ray direction
    float t = f * edge2.dot(q);
    return t > EPSILON;
  }

  // Helper: Filter points to keep only those inside the mesh
  // Returns the number of points remaining after filtering
  size_t
  filter_inside_mesh(const std::shared_ptr<core::GeometryContainer>& geometry,
                     core::AttributeStorage<Eigen::Vector3f>* positions) {
    if (!positions || !geometry) {
      return positions ? positions->size() : 0;
    }

    std::vector<Eigen::Vector3f> filtered_points;
    for (size_t i = 0; i < positions->size(); ++i) {
      const Eigen::Vector3f& point = (*positions)[i];
      if (is_point_inside_mesh(point, geometry.get())) {
        filtered_points.push_back(point);
      }
    }

    // Update positions with filtered points
    positions->resize(filtered_points.size());
    for (size_t i = 0; i < filtered_points.size(); ++i) {
      (*positions)[i] = filtered_points[i];
    }

    return filtered_points.size();
  }

  void scatter_in_box(std::mt19937& rng,
                      std::uniform_real_distribution<float>& dist, int count,
                      const Eigen::Vector3f& min_bounds,
                      const Eigen::Vector3f& max_bounds,
                      core::AttributeStorage<Eigen::Vector3f>* P) {
    for (int i = 0; i < count; ++i) {
      Eigen::Vector3f pos;
      pos.x() = min_bounds.x() + dist(rng) * (max_bounds.x() - min_bounds.x());
      pos.y() = min_bounds.y() + dist(rng) * (max_bounds.y() - min_bounds.y());
      pos.z() = min_bounds.z() + dist(rng) * (max_bounds.z() - min_bounds.z());
      P->set(i, pos);
    }
  }

  void scatter_uniform_grid(int count, const Eigen::Vector3f& min_bounds,
                            const Eigen::Vector3f& max_bounds,
                            core::AttributeStorage<Eigen::Vector3f>* P) {
    // Create roughly cubic grid
    int points_per_axis = static_cast<int>(std::cbrt(count)) + 1;
    Eigen::Vector3f size = max_bounds - min_bounds;

    int actual_count = 0;
    for (int z = 0; z < points_per_axis && actual_count < count; ++z) {
      for (int y = 0; y < points_per_axis && actual_count < count; ++y) {
        for (int x = 0; x < points_per_axis && actual_count < count; ++x) {
          float fx =
              static_cast<float>(x) / static_cast<float>(points_per_axis - 1);
          float fy =
              static_cast<float>(y) / static_cast<float>(points_per_axis - 1);
          float fz =
              static_cast<float>(z) / static_cast<float>(points_per_axis - 1);

          Eigen::Vector3f pos =
              min_bounds +
              Eigen::Vector3f(fx * size.x(), fy * size.y(), fz * size.z());
          P->set(actual_count, pos);
          actual_count++;
        }
      }
    }
  }

  void scatter_poisson_disk(std::mt19937& rng, int count,
                            const Eigen::Vector3f& min_bounds,
                            const Eigen::Vector3f& max_bounds,
                            float min_distance,
                            core::AttributeStorage<Eigen::Vector3f>* P) {
    // Simplified Poisson disk sampling
    // Use dart throwing with limited attempts
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::vector<Eigen::Vector3f> points;
    const int max_attempts = count * 30;

    for (int attempt = 0;
         attempt < max_attempts && points.size() < static_cast<size_t>(count);
         ++attempt) {
      Eigen::Vector3f candidate;
      candidate.x() =
          min_bounds.x() + dist(rng) * (max_bounds.x() - min_bounds.x());
      candidate.y() =
          min_bounds.y() + dist(rng) * (max_bounds.y() - min_bounds.y());
      candidate.z() =
          min_bounds.z() + dist(rng) * (max_bounds.z() - min_bounds.z());

      // Check distance to all existing points
      bool valid = true;
      for (const auto& existing : points) {
        if ((candidate - existing).norm() < min_distance) {
          valid = false;
          break;
        }
      }

      if (valid) {
        points.push_back(candidate);
      }
    }

    // Copy points to output
    for (size_t i = 0; i < points.size(); ++i) {
      P->set(i, points[i]);
    }
  }
};

} // namespace nodo::sop
