#include "nodeflux/sop/noise_displacement_sop.hpp"
#include "nodeflux/core/math.hpp"
#include "nodeflux/core/standard_attributes.hpp"
#include "nodeflux/core/types.hpp"
#include <cmath>

namespace attrs = nodeflux::core::standard_attrs;

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

NoiseDisplacementSOP::NoiseDisplacementSOP(const std::string &name)
    : SOPNode(name, "NoiseDisplacement") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_float_parameter("amplitude", 0.1F)
                         .label("Amplitude")
                         .range(0.0, 10.0)
                         .category("Noise")
                         .build());

  register_parameter(define_float_parameter("frequency", 1.0F)
                         .label("Frequency")
                         .range(0.01, 10.0)
                         .category("Noise")
                         .build());

  register_parameter(define_int_parameter("octaves", 4)
                         .label("Octaves")
                         .range(1, 8)
                         .category("Noise")
                         .build());

  register_parameter(define_float_parameter("lacunarity", 2.0F)
                         .label("Lacunarity")
                         .range(1.0, 4.0)
                         .category("Noise")
                         .build());

  register_parameter(define_float_parameter("persistence", 0.5F)
                         .label("Persistence")
                         .range(0.0, 1.0)
                         .category("Noise")
                         .build());

  register_parameter(define_int_parameter("seed", 42)
                         .label("Seed")
                         .range(0, 10000)
                         .category("Noise")
                         .build());
}

std::shared_ptr<GeometryData> NoiseDisplacementSOP::execute() {
  // Get input geometry
  auto input_geo = get_input_data(0);
  if (!input_geo) {
    set_error("No input geometry connected");
    return nullptr;
  }

  // Convert input to GeometryContainer
  core::GeometryContainer container;

  // Try to get mesh and convert to container
  auto input_mesh = input_geo->get_mesh();
  if (!input_mesh) {
    set_error("Input geometry does not contain a mesh");
    return nullptr;
  }

  // Simple conversion: copy vertices and faces to container
  const auto &vertices = input_mesh->vertices();
  const auto &faces = input_mesh->faces();

  auto &topology = container.topology();
  topology.set_point_count(vertices.rows());

  // Add primitives
  for (int face_idx = 0; face_idx < faces.rows(); ++face_idx) {
    std::vector<int> prim_verts;
    for (int j = 0; j < faces.cols(); ++j) {
      prim_verts.push_back(faces(face_idx, j));
    }
    topology.add_primitive(prim_verts);
  }

  // Copy P attribute
  container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage != nullptr) {
    auto p_span = p_storage->values_writable();
    for (int i = 0; i < vertices.rows(); ++i) {
      p_span[i] = vertices.row(i).cast<float>();
    }
  }

  // Read parameters from parameter system
  const float amplitude = get_parameter<float>("amplitude", 0.1F);
  const float frequency = get_parameter<float>("frequency", 1.0F);
  const int octaves = get_parameter<int>("octaves", 4);
  const float lacunarity = get_parameter<float>("lacunarity", 2.0F);
  const float persistence = get_parameter<float>("persistence", 0.5F);
  const int seed = get_parameter<int>("seed", 42);

  // Apply noise displacement to P attribute
  if (p_storage != nullptr) {
    auto p_span = p_storage->values_writable();

    for (size_t i = 0; i < p_span.size(); ++i) {
      const core::Vec3f &vertex = p_span[i];

      // Generate noise value at vertex position
      float noise_value =
          fractal_noise(vertex.x() * frequency, vertex.y() * frequency,
                        vertex.z() * frequency, seed, frequency, octaves,
                        lacunarity, persistence);

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
  }

  // Convert back to mesh for output (temporary until full pipeline migrated)
  auto output_mesh = std::make_shared<core::Mesh>();

  // Copy positions back
  Eigen::MatrixXd out_vertices(container.topology().point_count(), 3);
  if (p_storage != nullptr) {
    auto p_span = p_storage->values();
    for (size_t i = 0; i < p_span.size(); ++i) {
      out_vertices.row(i) = p_span[i].cast<double>();
    }
  }
  output_mesh->vertices() = out_vertices;

  // Copy faces back
  Eigen::MatrixXi out_faces(container.topology().primitive_count(), 3);
  for (size_t prim_idx = 0; prim_idx < container.topology().primitive_count();
       ++prim_idx) {
    const auto &verts = container.topology().get_primitive_vertices(prim_idx);
    for (size_t j = 0; j < 3 && j < verts.size(); ++j) {
      out_faces(prim_idx, j) = verts[j];
    }
  }
  output_mesh->faces() = out_faces;

  auto output_geo = std::make_shared<GeometryData>(output_mesh);

  return output_geo;
}

float NoiseDisplacementSOP::fractal_noise(float pos_x, float pos_y, float pos_z,
                                          int seed, float base_frequency,
                                          int octaves, float lacunarity,
                                          float persistence) const {
  float total = 0.0F;
  float max_value = 0.0F;
  float amplitude = 1.0F;
  float frequency = 1.0F;

  for (int i = 0; i < octaves; ++i) {
    total += simple_noise(pos_x * frequency, pos_y * frequency,
                          pos_z * frequency, seed) *
             amplitude;
    max_value += amplitude;
    amplitude *= persistence;
    frequency *= lacunarity;
  }

  return total / max_value;
}

float NoiseDisplacementSOP::simple_noise(float pos_x, float pos_y, float pos_z,
                                         int seed) const {
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
