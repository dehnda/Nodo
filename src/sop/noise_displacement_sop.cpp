#include "nodeflux/sop/noise_displacement_sop.hpp"
#include <algorithm>
#include <cmath>

namespace nodeflux::sop {

// Noise seed variation constants
constexpr float SEED_OFFSET_X = 0.1F;
constexpr float SEED_OFFSET_Y = 0.2F;
constexpr float SEED_OFFSET_Z = 0.3F;

// Smoothstep constants
constexpr float SMOOTHSTEP_COEFF_A = 3.0F;
constexpr float SMOOTHSTEP_COEFF_B = 2.0F;

// Vertex normal threshold
constexpr double VERTEX_NORMAL_THRESHOLD = 0.1;

std::optional<core::Mesh>
NoiseDisplacementSOP::process(const core::Mesh &input_mesh) {
  // Create a copy of the input mesh
  core::Mesh result = input_mesh;

  // Apply noise displacement to vertices
  auto &vertices = result.vertices();

  for (int i = 0; i < vertices.rows(); ++i) {
    Eigen::Vector3d vertex = vertices.row(i);

    // Generate noise value at vertex position
    float noise_value =
        fractal_noise(vertex.x() * frequency_, vertex.y() * frequency_,
                      vertex.z() * frequency_);

    // Calculate displacement direction (outward from origin for sphere-like
    // shapes)
    Eigen::Vector3d displacement_direction =
        Eigen::Vector3d(0, 0, 1); // Default upward

    // Use position-based normal for sphere-like shapes
    if (vertex.norm() > VERTEX_NORMAL_THRESHOLD) {
      displacement_direction = vertex.normalized();
    }

    // Apply displacement
    Eigen::Vector3d displacement =
        displacement_direction * (noise_value * amplitude_);
    vertices.row(i) = vertex + displacement;
  }

  return result;
}

float NoiseDisplacementSOP::fractal_noise(float pos_x, float pos_y,
                                          float pos_z) const {
  float total = 0.0F;
  float max_value = 0.0F;
  float amplitude = 1.0F;
  float frequency = 1.0F;

  for (int i = 0; i < octaves_; ++i) {
    total +=
        simple_noise(pos_x * frequency, pos_y * frequency, pos_z * frequency) *
        amplitude;
    max_value += amplitude;
    amplitude *= persistence_;
    frequency *= lacunarity_;
  }

  return total / max_value;
}

float NoiseDisplacementSOP::simple_noise(float pos_x, float pos_y,
                                         float pos_z) const {
  // Use seed to vary the noise pattern
  pos_x += seed_ * SEED_OFFSET_X;
  pos_y += seed_ * SEED_OFFSET_Y;
  pos_z += seed_ * SEED_OFFSET_Z;

  // Simple hash-based noise
  int grid_x = static_cast<int>(std::floor(pos_x));
  int grid_y = static_cast<int>(std::floor(pos_y));
  int grid_z = static_cast<int>(std::floor(pos_z));

  float frac_x = pos_x - grid_x;
  float frac_y = pos_y - grid_y;
  float frac_z = pos_z - grid_z;

  // Smooth interpolation
  frac_x = frac_x * frac_x * (SMOOTHSTEP_COEFF_A - SMOOTHSTEP_COEFF_B * frac_x);
  frac_y = frac_y * frac_y * (SMOOTHSTEP_COEFF_A - SMOOTHSTEP_COEFF_B * frac_y);
  frac_z = frac_z * frac_z * (SMOOTHSTEP_COEFF_A - SMOOTHSTEP_COEFF_B * frac_z);

  // Hash function for pseudo-random values
  auto hash = [](int x_coord, int y_coord, int z_coord) -> float {
    int n = x_coord + y_coord * 57 + z_coord * 113;
    n = (n << 13) ^ n;
    return (1.0F - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) /
                       1073741824.0F);
  };

  // Get corner values
  float c000 = hash(grid_x, grid_y, grid_z);
  float c001 = hash(grid_x, grid_y, grid_z + 1);
  float c010 = hash(grid_x, grid_y + 1, grid_z);
  float c011 = hash(grid_x, grid_y + 1, grid_z + 1);
  float c100 = hash(grid_x + 1, grid_y, grid_z);
  float c101 = hash(grid_x + 1, grid_y, grid_z + 1);
  float c110 = hash(grid_x + 1, grid_y + 1, grid_z);
  float c111 = hash(grid_x + 1, grid_y + 1, grid_z + 1);

  // Trilinear interpolation
  float c00 = c000 * (1.0F - frac_x) + c100 * frac_x;
  float c01 = c001 * (1.0F - frac_x) + c101 * frac_x;
  float c10 = c010 * (1.0F - frac_x) + c110 * frac_x;
  float c11 = c011 * (1.0F - frac_x) + c111 * frac_x;

  float c0 = c00 * (1.0F - frac_y) + c10 * frac_y;
  float c1 = c01 * (1.0F - frac_y) + c11 * frac_y;

  return c0 * (1.0F - frac_z) + c1 * frac_z;
}

} // namespace nodeflux::sop
