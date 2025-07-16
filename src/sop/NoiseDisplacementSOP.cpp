#include "nodeflux/sop/NoiseDisplacementSOP.hpp"
#include <cmath>
#include <algorithm>

namespace nodeflux::sop {

std::optional<core::Mesh> NoiseDisplacementSOP::process(const core::Mesh& input_mesh) {
    // Create a copy of the input mesh
    core::Mesh result = input_mesh;
    
    // Apply noise displacement to vertices
    auto& vertices = result.vertices();
    
    for (int i = 0; i < vertices.rows(); ++i) {
        Eigen::Vector3d vertex = vertices.row(i);
        
        // Generate noise value at vertex position
        float noise_value = fractal_noise(
            vertex.x() * frequency_,
            vertex.y() * frequency_,
            vertex.z() * frequency_
        );
        
        // Calculate displacement direction (outward from origin for sphere-like shapes)
        Eigen::Vector3d displacement_direction = Eigen::Vector3d(0, 0, 1); // Default upward
        
        // Use position-based normal for sphere-like shapes
        if (vertex.norm() > 0.1) {
            displacement_direction = vertex.normalized();
        }
        
        // Apply displacement
        Eigen::Vector3d displacement = displacement_direction * (noise_value * amplitude_);
        vertices.row(i) = vertex + displacement;
    }

    return result;
}

float NoiseDisplacementSOP::fractal_noise(float x, float y, float z) const {
    float total = 0.0F;
    float max_value = 0.0F;
    float amplitude = 1.0F;
    float frequency = 1.0F;

    for (int i = 0; i < octaves_; ++i) {
        total += simple_noise(x * frequency, y * frequency, z * frequency) * amplitude;
        max_value += amplitude;
        amplitude *= persistence_;
        frequency *= lacunarity_;
    }

    return total / max_value;
}

float NoiseDisplacementSOP::simple_noise(float x, float y, float z) const {
    // Use seed to vary the noise pattern
    x += seed_ * 0.1F;
    y += seed_ * 0.2F;
    z += seed_ * 0.3F;
    
    // Simple hash-based noise
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    int iz = static_cast<int>(std::floor(z));
    
    float fx = x - ix;
    float fy = y - iy;
    float fz = z - iz;
    
    // Smooth interpolation
    fx = fx * fx * (3.0F - 2.0F * fx);
    fy = fy * fy * (3.0F - 2.0F * fy);
    fz = fz * fz * (3.0F - 2.0F * fz);
    
    // Hash function for pseudo-random values
    auto hash = [](int x_coord, int y_coord, int z_coord) -> float {
        int n = x_coord + y_coord * 57 + z_coord * 113;
        n = (n << 13) ^ n;
        return (1.0F - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0F);
    };
    
    // Get corner values
    float c000 = hash(ix, iy, iz);
    float c001 = hash(ix, iy, iz + 1);
    float c010 = hash(ix, iy + 1, iz);
    float c011 = hash(ix, iy + 1, iz + 1);
    float c100 = hash(ix + 1, iy, iz);
    float c101 = hash(ix + 1, iy, iz + 1);
    float c110 = hash(ix + 1, iy + 1, iz);
    float c111 = hash(ix + 1, iy + 1, iz + 1);
    
    // Trilinear interpolation
    float c00 = c000 * (1.0F - fx) + c100 * fx;
    float c01 = c001 * (1.0F - fx) + c101 * fx;
    float c10 = c010 * (1.0F - fx) + c110 * fx;
    float c11 = c011 * (1.0F - fx) + c111 * fx;
    
    float c0 = c00 * (1.0F - fy) + c10 * fy;
    float c1 = c01 * (1.0F - fy) + c11 * fy;
    
    return c0 * (1.0F - fz) + c1 * fz;
}

} // namespace nodeflux::sop
