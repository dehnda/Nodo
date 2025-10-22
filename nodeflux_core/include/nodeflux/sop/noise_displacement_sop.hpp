#pragma once

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/sop/geometry_data.hpp"
#include "nodeflux/sop/sop_node.hpp"
#include <Eigen/Dense>
#include <memory>
#include <string>

namespace nodeflux::sop {

/**
 * @brief Noise Displacement SOP - Applies procedural noise displacement to mesh
 * vertices
 *
 * Displaces mesh vertices using multi-octave fractal noise patterns for organic
 * effects.
 */
class NoiseDisplacementSOP : public SOPNode {
public:
  explicit NoiseDisplacementSOP(const std::string &name = "noise_displacement");

  // Configuration methods
  void set_amplitude(float amplitude) {
    if (amplitude_ != amplitude) {
      amplitude_ = amplitude;
      mark_dirty();
    }
  }

  void set_frequency(float frequency) {
    if (frequency_ != frequency) {
      frequency_ = frequency;
      mark_dirty();
    }
  }

  void set_octaves(int octaves) {
    int clamped = std::max(1, std::min(octaves, 8));
    if (octaves_ != clamped) {
      octaves_ = clamped;
      mark_dirty();
    }
  }

  void set_lacunarity(float lacunarity) {
    if (lacunarity_ != lacunarity) {
      lacunarity_ = lacunarity;
      mark_dirty();
    }
  }

  void set_persistence(float persistence) {
    if (persistence_ != persistence) {
      persistence_ = persistence;
      mark_dirty();
    }
  }

  void set_seed(int seed) {
    if (seed_ != seed) {
      seed_ = seed;
      mark_dirty();
    }
  }

  // Getters
  float get_amplitude() const { return amplitude_; }
  float get_frequency() const { return frequency_; }
  int get_octaves() const { return octaves_; }
  float get_lacunarity() const { return lacunarity_; }
  float get_persistence() const { return persistence_; }
  int get_seed() const { return seed_; }

protected:
  /**
   * @brief Execute the noise displacement operation (SOPNode override)
   */
  std::shared_ptr<GeometryData> execute() override;

private:
  // Noise generation functions
  float fractal_noise(float pos_x, float pos_y, float pos_z) const;
  float simple_noise(float pos_x, float pos_y, float pos_z) const;

  float amplitude_ = 0.1F;
  float frequency_ = 1.0F;
  int octaves_ = 4;
  float lacunarity_ = 2.0F;
  float persistence_ = 0.5F;
  int seed_ = 42;
};

} // namespace nodeflux::sop
