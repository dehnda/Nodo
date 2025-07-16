#pragma once

#include "nodeflux/core/mesh.hpp"
#include <Eigen/Dense>
#include <optional>
#include <string>

namespace nodeflux::sop {

// Noise displacement constants
constexpr float DEFAULT_NOISE_AMPLITUDE = 0.1F;
constexpr float DEFAULT_NOISE_LACUNARITY = 2.0F;
constexpr float DEFAULT_NOISE_PERSISTENCE = 0.5F;
constexpr int DEFAULT_NOISE_SEED = 42;
constexpr int MAX_NOISE_OCTAVES = 8;

/**
 * @brief Noise Displacement SOP - Applies procedural noise displacement to mesh
 * vertices
 *
 * Displaces mesh vertices using multi-octave fractal noise patterns for organic
 * effects.
 */
class NoiseDisplacementSOP {
private:
  std::string name_;
  float amplitude_ = DEFAULT_NOISE_AMPLITUDE;
  float frequency_ = 1.0F;
  int octaves_ = 4;
  float lacunarity_ = DEFAULT_NOISE_LACUNARITY;
  float persistence_ = DEFAULT_NOISE_PERSISTENCE;
  int seed_ = DEFAULT_NOISE_SEED;

public:
  explicit NoiseDisplacementSOP(std::string name = "noise_displacement")
      : name_(std::move(name)) {}

  const std::string &get_name() const { return name_; }

  // Configuration methods
  void set_amplitude(float amplitude) { amplitude_ = amplitude; }
  void set_frequency(float frequency) { frequency_ = frequency; }
  void set_octaves(int octaves) {
    octaves_ = std::max(1, std::min(octaves, MAX_NOISE_OCTAVES));
  }
  void set_lacunarity(float lacunarity) { lacunarity_ = lacunarity; }
  void set_persistence(float persistence) { persistence_ = persistence; }
  void set_seed(int seed) { seed_ = seed; }

  // Getters
  float get_amplitude() const { return amplitude_; }
  float get_frequency() const { return frequency_; }
  int get_octaves() const { return octaves_; }
  float get_lacunarity() const { return lacunarity_; }
  float get_persistence() const { return persistence_; }
  int get_seed() const { return seed_; }

  /**
   * @brief Apply noise displacement to input mesh
   */
  std::optional<core::Mesh> process(const core::Mesh &input_mesh);

private:
  // Noise generation functions
  float fractal_noise(float pos_x, float pos_y, float pos_z) const;
  float simple_noise(float pos_x, float pos_y, float pos_z) const;
};

} // namespace nodeflux::sop
