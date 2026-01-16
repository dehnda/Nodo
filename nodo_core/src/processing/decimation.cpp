#include "nodo/processing/decimation.hpp"

#include "nodo/core/math.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/processing/pmp_converter.hpp"
#include "nodo/processing/processing_common.hpp"

#include <algorithm>

#include <pmp/algorithms/decimation.h>
#include <pmp/algorithms/triangulation.h>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::processing {

std::string Decimation::last_error_;

std::optional<core::GeometryContainer> Decimation::decimate(const core::GeometryContainer& input,
                                                            const DecimationParams& params) {
  // Basic validation
  if (input.topology().point_count() < 4) {
    set_error("Input mesh must have at least 4 vertices for decimation");
    return std::nullopt;
  }

  if (input.topology().primitive_count() < 1) {
    set_error("Input mesh must have at least 1 primitive");
    return std::nullopt;
  }

  if (!input.has_point_attribute(attrs::P)) {
    set_error("Input mesh missing position attribute 'P'");
    return std::nullopt;
  }

  try {
    // Convert to PMP format (handles the conversion)
    auto pmp_mesh = detail::PMPConverter::to_pmp(input);

    // Triangulate the mesh if it has non-triangle faces
    // PMP's triangulate() handles this efficiently
    pmp::triangulate(pmp_mesh);

    // Calculate target vertex count
    unsigned int target_vertices;
    if (params.use_vertex_count) {
      target_vertices = static_cast<unsigned int>(params.target_vertex_count);
    } else {
      const unsigned int original_count = static_cast<unsigned int>(pmp_mesh.n_vertices());
      target_vertices = static_cast<unsigned int>(static_cast<float>(original_count) * params.target_percentage);
    }

    // Ensure minimum vertex count
    target_vertices = std::max(target_vertices, 4U);

    // Perform decimation using PMP
    pmp::decimate(pmp_mesh, target_vertices, params.aspect_ratio, params.edge_length, params.max_valence,
                  params.preserve_topology, params.preserve_boundaries);

    // Convert back to Nodo format
    auto result = detail::PMPConverter::from_pmp_container(pmp_mesh, true);

    // Clear any previous error
    last_error_.clear();

    return result;

  } catch (const std::exception& e) {
    set_error(std::string("Decimation failed: ") + e.what());
    return std::nullopt;
  }
}

const std::string& Decimation::get_last_error() {
  return last_error_;
}

void Decimation::set_error(std::string error) {
  last_error_ = std::move(error);
}

} // namespace nodo::processing
