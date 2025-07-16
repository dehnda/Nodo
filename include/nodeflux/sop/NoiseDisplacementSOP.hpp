#pragma once

#include "nodeflux/core/mesh.hpp"
#include <Eigen/Dense>
#include <optional>
#include <string>

namespace nodeflux::sop {

/**
 * @brief Noise Displacement SOP - Applies procedural noise displacement to mesh vertices
 * 
 * Displaces mesh vertices using multi-octave fractal noise patterns for organic effects.
 */
class NoiseDisplacementSOP {
private:
    std::string name_;
    float amplitude_ = 0.1F;
    float frequency_ = 1.0F;
    int octaves_ = 4;
    float lacunarity_ = 2.0F;
    float persistence_ = 0.5F;
    int seed_ = 42;

public:
    explicit NoiseDisplacementSOP(std::string name = "noise_displacement") : name_(std::move(name)) {}
    
    const std::string& get_name() const { return name_; }

    // Configuration methods
    void set_amplitude(float amplitude) { amplitude_ = amplitude; }
    void set_frequency(float frequency) { frequency_ = frequency; }
    void set_octaves(int octaves) { octaves_ = std::max(1, std::min(octaves, 8)); }
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
    std::optional<core::Mesh> process(const core::Mesh& input_mesh);

private:
    // Noise generation functions
    float fractal_noise(float x, float y, float z) const;
    float simple_noise(float x, float y, float z) const;
};

} // namespace nodeflux::sop
