#include "nodo/processing/remeshing.hpp"
#include "nodo/processing/pmp_converter.hpp"
#include <pmp/algorithms/remeshing.h>
#include <pmp/algorithms/triangulation.h>

namespace nodo::processing {

std::optional<core::GeometryContainer>
Remeshing::remesh(const core::GeometryContainer &container,
                  const RemeshingParams &params, std::string *error) {

  // Validate input
  if (container.point_count() < 3) {
    if (error)
      *error = "Remeshing requires at least 3 points";
    return std::nullopt;
  }

  if (container.primitive_count() == 0) {
    if (error)
      *error = "Remeshing requires at least one primitive";
    return std::nullopt;
  }

  try {
    // Convert to PMP format
    auto pmp_mesh = detail::PMPConverter::to_pmp(container);

    if (pmp_mesh.n_vertices() < 4) {
      if (error)
        *error = "Mesh too small for remeshing (minimum 4 vertices required)";
      return std::nullopt;
    }

    // Triangulate the mesh if it has non-triangle faces
    pmp::triangulate(pmp_mesh);

    // Perform remeshing
    if (params.use_adaptive) {
      // Adaptive remeshing - adjusts to curvature
      pmp::adaptive_remeshing(pmp_mesh, params.min_edge_length,
                              params.max_edge_length, params.approx_error,
                              params.iterations, true);
    } else {
      // Uniform remeshing - constant edge length
      pmp::uniform_remeshing(pmp_mesh, params.target_edge_length,
                             params.iterations, params.preserve_boundaries);
    }

    // Convert back to GeometryContainer
    return detail::PMPConverter::from_pmp_container(pmp_mesh);

  } catch (const std::exception &e) {
    if (error)
      *error = std::string("Remeshing failed: ") + e.what();
    return std::nullopt;
  }
}

} // namespace nodo::processing
