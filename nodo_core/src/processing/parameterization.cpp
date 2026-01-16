#include "nodo/processing/parameterization.hpp"

#include "nodo/core/attribute_types.hpp"
#include "nodo/core/math.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/processing/pmp_converter.hpp"
#include "nodo/processing/processing_common.hpp"

#include <pmp/algorithms/normals.h>
#include <pmp/algorithms/parameterization.h>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::processing {

std::optional<core::GeometryContainer> Parameterization::parameterize(const core::GeometryContainer& input,
                                                                      const ParameterizationParams& params,
                                                                      std::string* error) {
  try {
    // Validate input
    auto validation_error = detail::PMPConverter::validate_for_pmp(input);
    if (!validation_error.empty()) {
      if (error)
        *error = validation_error;
      return std::nullopt;
    }

    // Convert to PMP
    auto pmp_mesh = detail::PMPConverter::to_pmp(input);

    // Check if mesh has boundary
    bool has_boundary = false;
    for (auto h : pmp_mesh.halfedges()) {
      if (pmp_mesh.is_boundary(h)) {
        has_boundary = true;
        break;
      }
    }

    if (!has_boundary) {
      if (error)
        *error = "Parameterization requires mesh with at least one boundary. "
                 "Closed meshes must be cut open first (use Blast/Split to "
                 "delete faces).";
      return std::nullopt;
    }

    // Apply parameterization based on method
    if (params.method == ParameterizationMethod::Harmonic) {
      // Harmonic parameterization works on general polygon meshes
      pmp::harmonic_parameterization(pmp_mesh, params.use_uniform_weights);
    } else if (params.method == ParameterizationMethod::LSCM) {
      // LSCM requires triangle mesh
      // Check if triangulated
      for (auto f : pmp_mesh.faces()) {
        if (pmp_mesh.valence(f) != 3) {
          if (error)
            *error = "LSCM parameterization requires triangle mesh";
          return std::nullopt;
        }
      }

      pmp::lscm_parameterization(pmp_mesh);
    }

    // Convert back to GeometryContainer
    auto result = detail::PMPConverter::from_pmp(pmp_mesh, true);

    // Extract UV coordinates from PMP's "v:tex" property
    auto tex_coords = pmp_mesh.get_vertex_property<pmp::TexCoord>("v:tex");
    if (tex_coords) {
      const size_t n_vertices = pmp_mesh.n_vertices();

      // Create UV attribute
      result.add_point_attribute(params.uv_attribute_name, core::AttributeType::VEC2F);
      auto* uv_attr = result.get_point_attribute_typed<core::Vec2f>(params.uv_attribute_name);
      uv_attr->resize(n_vertices);
      auto uv_writable = uv_attr->values_writable();

      // Copy UV coordinates
      size_t idx = 0;
      for (auto v : pmp_mesh.vertices()) {
        const auto& tex = tex_coords[v];
        uv_writable[idx] = core::Vec2f(tex[0], tex[1]);
        ++idx;
      }
    } else {
      if (error)
        *error = "Failed to retrieve UV coordinates from parameterization";
      return std::nullopt;
    }

    return std::move(result);

  } catch (const std::exception& e) {
    if (error)
      *error = std::string("Parameterization failed: ") + e.what();
    return std::nullopt;
  }
}

} // namespace nodo::processing
