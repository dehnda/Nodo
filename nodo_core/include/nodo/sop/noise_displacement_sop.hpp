#pragma once

#include "nodo/core/geometry_container.hpp"
#include "nodo/sop/sop_node.hpp"
#include <Eigen/Dense>
#include <memory>
#include <string>

namespace nodo::sop {

/**
 * @brief Noise Displacement SOP - Applies procedural noise displacement to mesh
 * vertices
 *
 * Displaces mesh vertices using multi-octave fractal noise patterns for organic
 * effects.
 */
class NoiseDisplacementSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit NoiseDisplacementSOP(const std::string &name = "noise_displacement");

protected:
  /**
   * @brief Execute the noise displacement operation (SOPNode override)
   */
  std::shared_ptr<core::GeometryContainer> execute() override;

private:
  // Noise generation functions
  float fractal_noise(float pos_x, float pos_y, float pos_z, int seed,
                      float frequency, int octaves, float lacunarity,
                      float persistence) const;
  float simple_noise(float pos_x, float pos_y, float pos_z, int seed) const;
};

} // namespace nodo::sop
