#pragma once

#include "../sop_node.hpp"
#include "../../gpu/gpu_mesh_generator.hpp"

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
    // GPU mesh generator for actual computation
    mutable std::unique_ptr<gpu::GPUMeshGenerator> gpu_generator_;

public:
    explicit SphereSOP(const std::string& node_name = "sphere")
        : SOPNode(node_name, "SphereSOP") {
        
        // Set default parameters
        set_parameter("radius", 1.0f);
        set_parameter("segments", 32);
        set_parameter("rings", 16);
        set_parameter("use_gpu", true);
        
        // Initialize GPU generator if available
        try {
            gpu_generator_ = std::make_unique<gpu::GPUMeshGenerator>();
        } catch (const std::exception& exception) {
            // Fall back to CPU generation if GPU not available
            set_parameter("use_gpu", false);
        }
    }

    /**
     * @brief Set sphere radius
     */
    void set_radius(float radius) {
        set_parameter("radius", radius);
    }

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
    void set_gpu_acceleration(bool enabled) {
        set_parameter("use_gpu", enabled);
    }

protected:
    /**
     * @brief Execute sphere generation
     */
    std::shared_ptr<GeometryData> execute() override {
        const float radius = get_parameter<float>("radius", 1.0f);
        const int segments = get_parameter<int>("segments", 32);
        const int rings = get_parameter<int>("rings", 16);
        const bool use_gpu = get_parameter<bool>("use_gpu", false);

        try {
            std::shared_ptr<core::Mesh> mesh;

            if (use_gpu && gpu_generator_) {
                // Use GPU generation
                mesh = gpu_generator_->generate_sphere(radius, segments, rings);
            } else {
                // Fall back to CPU generation
                // TODO: Implement CPU sphere generation or use existing generator
                set_error("CPU sphere generation not yet implemented");
                return nullptr;
            }

            if (!mesh) {
                set_error("Failed to generate sphere mesh");
                return nullptr;
            }

            // Create GeometryData container
            auto geometry_data = std::make_shared<GeometryData>(mesh);
            
            // Set some basic attributes
            geometry_data->set_global_attribute("primitive_type", std::string("sphere"));
            geometry_data->set_global_attribute("radius", radius);
            geometry_data->set_global_attribute("segments", segments);
            geometry_data->set_global_attribute("rings", rings);

            return geometry_data;

        } catch (const std::exception& exception) {
            set_error("Exception during sphere generation: " + std::string(exception.what()));
            return nullptr;
        }
    }
};

} // namespace sop
} // namespace nodeflux
