#include "nodo/processing/subdivision.hpp"

#include "nodo/core/math.hpp"
#include "nodo/processing/pmp_converter.hpp"
#include "nodo/processing/processing_common.hpp"

#include <fmt/core.h>
#include <pmp/algorithms/subdivision.h>
#include <pmp/algorithms/triangulation.h>

namespace nodo::processing {

std::optional<core::GeometryContainer>
Subdivision::subdivide(const core::GeometryContainer& container,
                       const SubdivisionParams& params, std::string* error) {
  // Validate input
  if (container.point_count() < 3) {
    if (error)
      *error = "Subdivision requires at least 3 points";
    return std::nullopt;
  }

  if (container.primitive_count() == 0) {
    if (error)
      *error = "Subdivision requires at least one primitive";
    return std::nullopt;
  }

  if (params.levels < 1 || params.levels > 10) {
    if (error)
      *error = "Subdivision levels must be between 1 and 10";
    return std::nullopt;
  }

  try {
    // Convert to PMP format
    auto pmp_mesh = detail::PMPConverter::to_pmp(container);

    if (pmp_mesh.n_vertices() < 3) {
      if (error)
        *error = "Mesh too small for subdivision (minimum 3 vertices required)";
      return std::nullopt;
    }

    fmt::print("Before subdivision: {} vertices, {} faces\n",
               pmp_mesh.n_vertices(), pmp_mesh.n_faces());

    // Loop subdivision requires triangles - triangulate first
    // Catmull-Clark and Quad-Tri work on quads/mixed, so DON'T triangulate
    if (params.type == SubdivisionType::LOOP) {
      pmp::triangulate(pmp_mesh);
      fmt::print("After triangulation for Loop: {} vertices, {} faces\n",
                 pmp_mesh.n_vertices(), pmp_mesh.n_faces());
    }

    // Perform subdivision for the specified number of levels
    for (unsigned int i = 0; i < params.levels; ++i) {
      switch (params.type) {
        case SubdivisionType::CATMULL_CLARK:
          pmp::catmull_clark_subdivision(pmp_mesh);
          break;

        case SubdivisionType::LOOP:
          pmp::loop_subdivision(pmp_mesh);
          break;

        case SubdivisionType::QUAD_TRI:
          pmp::quad_tri_subdivision(pmp_mesh);
          break;
      }
      fmt::print("After subdivision level {}: {} vertices, {} faces\n", i + 1,
                 pmp_mesh.n_vertices(), pmp_mesh.n_faces());
    }

    // Convert back to GeometryContainer
    return detail::PMPConverter::from_pmp_container(pmp_mesh);

  } catch (const std::exception& e) {
    if (error)
      *error = std::string("Subdivision failed: ") + e.what();
    return std::nullopt;
  }
}

} // namespace nodo::processing
