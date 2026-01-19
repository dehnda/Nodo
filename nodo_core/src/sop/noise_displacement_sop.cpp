#include "nodo/sop/noise_displacement_sop.hpp"

#include "nodo/core/math.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/core/types.hpp"

#include <cmath>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::sop {

// Noise seed variation constants
constexpr float SEED_OFFSET_X = 0.1F;
constexpr float SEED_OFFSET_Y = 0.2F;
constexpr float SEED_OFFSET_Z = 0.3F;

// Smoothstep constants
constexpr float SMOOTHSTEP_COEFF_A = 3.0F;
constexpr float SMOOTHSTEP_COEFF_B = 2.0F;

// Vertex normal threshold
constexpr double VERTEX_NORMAL_THRESHOLD = 0.1;

NoiseDisplacementSOP::NoiseDisplacementSOP(const std::string& name) : SOPNode(name, "NoiseDisplacement") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_float_parameter("amplitude", 0.1F)
                         .label("Amplitude")
                         .range(0.0, 10.0)
                         .category("Noise")
                         .description("Displacement strength (height of noise)")
                         .build());

  register_parameter(define_float_parameter("frequency", 1.0F)
                         .label("Frequency")
                         .range(0.01, 10.0)
                         .category("Noise")
                         .description("Noise pattern frequency (scale)")
                         .build());

  register_parameter(define_int_parameter("octaves", 4)
                         .label("Octaves")
                         .range(1, 8)
                         .category("Noise")
                         .description("Number of noise layers to combine")
                         .build());

  register_parameter(define_float_parameter("lacunarity", 2.0F)
                         .label("Lacunarity")
                         .range(1.0, 4.0)
                         .category("Noise")
                         .description("Frequency multiplier for each octave")
                         .build());

  register_parameter(define_float_parameter("persistence", 0.5F)
                         .label("Persistence")
                         .range(0.0, 1.0)
                         .category("Noise")
                         .description("Amplitude multiplier for each octave")
                         .build());

  register_parameter(define_int_parameter("seed", 42)
                         .label("Seed")
                         .range(0, 10000)
                         .category("Noise")
                         .description("Random seed for noise pattern")
                         .build());
}

core::Result<std::shared_ptr<core::GeometryContainer>> NoiseDisplacementSOP::execute() {
  // Get input geometry using COW handle
  auto handle = get_input_handle(0);
  if (!handle.is_valid()) {
    set_error("No input geometry connected");
    return {(std::string) "No input geometry connected"};
  }

  // Get writable access (triggers COW if shared)
  auto& output = handle.write();

  // Read parameters from parameter system
  const float amplitude = get_parameter<float>("amplitude", 0.1F);
  const float frequency = get_parameter<float>("frequency", 1.0F);
  const int octaves = get_parameter<int>("octaves", 4);
  const float lacunarity = get_parameter<float>("lacunarity", 2.0F);
  const float persistence = get_parameter<float>("persistence", 0.5F);
  const int seed = get_parameter<int>("seed", 42);

  // Apply noise displacement to P attribute
  auto* p_storage = output.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (!p_storage) {
    set_error("Input geometry missing position attribute");
    return {(std::string) "Input geometry missing position attribute"};
  }

  auto p_span = p_storage->values_writable();

  // Create temporary shared_ptr for group checking (no-op deleter)
  std::shared_ptr<core::GeometryContainer> geo_ptr(&output, [](core::GeometryContainer*) {});

  for (size_t i = 0; i < p_span.size(); ++i) {
    // Skip points not in group filter
    if (!is_in_group(geo_ptr, 0, i))
      continue;

    const core::Vec3f& vertex = p_span[i];

    // Generate noise value at vertex position
    float noise_value = fractal_noise(vertex.x() * frequency, vertex.y() * frequency, vertex.z() * frequency, seed,
                                      frequency, octaves, lacunarity, persistence);

    // Calculate displacement direction (outward from origin for sphere-like
    // shapes)
    core::Vec3f displacement_direction(0.0F, 0.0F, 1.0F); // Default upward

    // Use position-based normal for sphere-like shapes
    if (vertex.norm() > VERTEX_NORMAL_THRESHOLD) {
      displacement_direction = vertex.normalized();
    }

    // Apply displacement
    p_span[i] = vertex + displacement_direction * (noise_value * amplitude);
  }

  return std::make_shared<core::GeometryContainer>(std::move(output));
}

float NoiseDisplacementSOP::fractal_noise(float pos_x, float pos_y, float pos_z, int seed,
                                          [[maybe_unused]] float base_frequency, int octaves, float lacunarity,
                                          float persistence) const {
  float total = 0.0F;
  float max_value = 0.0F;
  float amplitude = 1.0F;
  float frequency = 1.0F;

  for (int i = 0; i < octaves; ++i) {
    total += simple_noise(pos_x * frequency, pos_y * frequency, pos_z * frequency, seed) * amplitude;
    max_value += amplitude;
    amplitude *= persistence;
    frequency *= lacunarity;
  }

  return total / max_value;
}

float NoiseDisplacementSOP::simple_noise(float pos_x, float pos_y, float pos_z, int seed) const {
  // Use seed to vary the noise pattern
  pos_x += seed * SEED_OFFSET_X;
  pos_y += seed * SEED_OFFSET_Y;
  pos_z += seed * SEED_OFFSET_Z;

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
    return (1.0F - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0F);
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

} // namespace nodo::sop
