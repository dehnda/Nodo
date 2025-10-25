#pragma once

#include "nodeflux/core/geometry_container.hpp"
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

protected:
  /**
   * @brief Execute the noise displacement operation (SOPNode override)
   */
  std::shared_ptr<GeometryData> execute() override;

private:
  // Noise generation functions
  float fractal_noise(float pos_x, float pos_y, float pos_z, int seed,
                      float frequency, int octaves, float lacunarity,
                      float persistence) const;
  float simple_noise(float pos_x, float pos_y, float pos_z, int seed) const;
};

} // namespace nodeflux::sop
