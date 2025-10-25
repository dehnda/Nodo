#pragma once

#include "../core/geometry_container.hpp"
#include "../geometry/sphere_generator.hpp"
#include "../gpu/gpu_mesh_generator.hpp"
#include "sop_node.hpp"

namespace nodeflux {
namespace sop {

/**
 * @brief GPU-accelerated sphere generator SOP node
 *
 * This node generates spheres using the existing GPU mesh generation system
 * and integrates them into the SOP data flow architecture.
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
    set_parameter("use_gpu", gpu::GPUMeshGenerator::is_available());
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

  /**
   * @brief Enable/disable GPU acceleration
   */
  void set_gpu_acceleration(bool enabled) { set_parameter("use_gpu", enabled); }

protected:
  /**
   * @brief Execute sphere generation
   */
  std::shared_ptr<core::GeometryContainer> execute() override {
    const float radius = get_parameter<float>("radius", DEFAULT_RADIUS);
    const int segments = get_parameter<int>("segments", DEFAULT_SEGMENTS);
    const int rings = get_parameter<int>("rings", DEFAULT_RINGS);
    const bool use_gpu = get_parameter<bool>("use_gpu", false);

    try {
      std::shared_ptr<core::Mesh> mesh;

      if (use_gpu && gpu::GPUMeshGenerator::is_available()) {
        // Use GPU generation (static method)
        auto gpu_result =
            gpu::GPUMeshGenerator::generate_sphere(radius, segments, rings);
        if (gpu_result.has_value()) {
          mesh = std::make_shared<core::Mesh>(std::move(gpu_result.value()));
        } else {
          set_error("GPU sphere generation failed");
          return nullptr;
        }
      } else {
        // Fall back to CPU generation using SphereGenerator
        auto cpu_result = geometry::SphereGenerator::generate_uv_sphere(
            static_cast<double>(radius), segments, rings);
        if (cpu_result.has_value()) {
          mesh = std::make_shared<core::Mesh>(std::move(cpu_result.value()));
        } else {
          set_error("CPU sphere generation failed");
          return nullptr;
        }
      }

      if (!mesh) {
        set_error("Failed to generate sphere mesh");
        return nullptr;
      }

      // Convert Mesh to GeometryContainer
      auto container = std::make_shared<core::GeometryContainer>();
      const auto &vertices = mesh->vertices();
      const auto &faces = mesh->faces();

      // Set topology
      container->topology().set_point_count(vertices.rows());
      for (int i = 0; i < faces.rows(); ++i) {
        std::vector<int> prim_verts = {faces(i, 0), faces(i, 1), faces(i, 2)};
        container->topology().add_primitive(prim_verts);
      }

      // Add positions
      container->add_point_attribute("P", core::AttributeType::VEC3F);
      auto *positions =
          container->get_point_attribute_typed<Eigen::Vector3f>("P");
      if (positions) {
        auto pos_span = positions->values_writable();
        for (size_t i = 0; i < static_cast<size_t>(vertices.rows()); ++i) {
          pos_span[i] = vertices.row(i).cast<float>();
        }
      }

      // Add normals
      const auto &normals = mesh->vertex_normals();
      container->add_point_attribute("N", core::AttributeType::VEC3F);
      auto *normal_attr =
          container->get_point_attribute_typed<Eigen::Vector3f>("N");
      if (normal_attr) {
        auto norm_span = normal_attr->values_writable();
        for (size_t i = 0; i < static_cast<size_t>(normals.rows()); ++i) {
          norm_span[i] = normals.row(i).cast<float>();
        }
      }

      return container;

    } catch (const std::exception &exception) {
      set_error("Exception during sphere generation: " +
                std::string(exception.what()));
      return nullptr;
    }
  }
};

} // namespace sop
} // namespace nodeflux
