#include "nodo/processing/decimation.hpp"

#include "nodo/core/math.hpp"
#include "nodo/core/result.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/processing/pmp_converter.hpp"
#include "nodo/processing/processing_common.hpp"

#include <algorithm>

#include <pmp/algorithms/decimation.h>
#include <pmp/algorithms/triangulation.h>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::processing {

core::Result<std::shared_ptr<core::GeometryContainer>> Decimation::decimate(const core::GeometryContainer& input,
                                                                            const DecimationParams& params) {
  // Basic validation
  if (input.topology().point_count() < 4) {
    return {"Input mesh must have at least 4 vertices for decimation"};
  }

  if (input.topology().primitive_count() < 1) {
    return {"Input mesh must have at least 1 primitive"};
  }

  if (!input.has_point_attribute(attrs::P)) {
    return {"Input mesh missing position attribute 'P'"};
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
    auto result = detail::PMPConverter::from_pmp(pmp_mesh, true);

    return std::make_shared<core::GeometryContainer>(std::move(result));

  } catch (const std::exception& e) {
    return {std::string("Decimation failed: ") + e.what()};
  }
}

} // namespace nodo::processing
