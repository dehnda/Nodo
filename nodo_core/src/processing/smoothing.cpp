#include "nodo/processing/smoothing.hpp"

#include "nodo/core/math.hpp"
#include "nodo/processing/pmp_converter.hpp"
#include "nodo/processing/processing_common.hpp"

#include <pmp/algorithms/fairing.h>
#include <pmp/algorithms/smoothing.h>
#include <pmp/algorithms/triangulation.h>

namespace nodo::processing {

std::optional<core::GeometryContainer> Smoothing::smooth(const core::GeometryContainer& container,
                                                         const SmoothingParams& params, std::string* error) {
  // Validate input
  if (container.point_count() < 3) {
    if (error)
      *error = "Smoothing requires at least 3 points";
    return std::nullopt;
  }

  if (container.primitive_count() == 0) {
    if (error)
      *error = "Smoothing requires at least one primitive";
    return std::nullopt;
  }

  try {
    // Convert to PMP format
    auto pmp_mesh = detail::PMPConverter::to_pmp(container);

    if (pmp_mesh.n_vertices() < 3) {
      if (error)
        *error = "Mesh too small for smoothing (minimum 3 vertices required)";
      return std::nullopt;
    }

    // Triangulate the mesh if it has non-triangle faces
    pmp::triangulate(pmp_mesh);

    // Perform smoothing based on method
    if (params.method == 2) {
      // Fairing - minimizes curvature variation (highest quality)
      // Use minimize_curvature which doesn't require boundary constraints
      pmp::minimize_curvature(pmp_mesh);
    } else if (params.method == 1) {
      // Implicit smoothing - higher quality, solves linear system
      pmp::implicit_smoothing(pmp_mesh, params.timestep, params.iterations, params.use_uniform_laplace, params.rescale);
    } else {
      // Explicit smoothing - fast iterative approach (method == 0)
      pmp::explicit_smoothing(pmp_mesh, params.iterations, params.use_uniform_laplace);
    }

    // Convert back to GeometryContainer
    return detail::PMPConverter::from_pmp_container(pmp_mesh);

  } catch (const std::exception& e) {
    if (error)
      *error = std::string("Smoothing failed: ") + e.what();
    return std::nullopt;
  }
}

} // namespace nodo::processing
