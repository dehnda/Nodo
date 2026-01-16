#include "nodo/processing/curvature.hpp"

#include "nodo/core/math.hpp"
#include "nodo/processing/pmp_converter.hpp"
#include "nodo/processing/processing_common.hpp"

#include <cmath>

#include <fmt/core.h>
#include <pmp/algorithms/curvature.h>
#include <pmp/algorithms/normals.h>

namespace nodo::processing {

std::optional<core::GeometryContainer> Curvature::compute(const core::GeometryContainer& input,
                                                          const CurvatureParams& params) {
  // Validate input
  auto validation_error = detail::PMPConverter::validate_for_pmp(input);
  if (!validation_error.empty()) {
    fmt::print("Curvature: Validation failed: {}\n", validation_error);
    return std::nullopt;
  }

  // Convert to PMP
  auto pmp_mesh = detail::PMPConverter::to_pmp(input);

  fmt::print("Curvature: Computing curvature for mesh with {} vertices, {} faces\n", pmp_mesh.n_vertices(),
             pmp_mesh.n_faces());

  // Ensure normals are computed
  if (!pmp_mesh.has_vertex_property("v:normal")) {
    pmp::vertex_normals(pmp_mesh);
  }

  // Compute curvature
  try {
    // Determine which curvature types to compute
    bool compute_mean = (params.type == CurvatureType::MEAN || params.type == CurvatureType::ALL);
    bool compute_gaussian = (params.type == CurvatureType::GAUSSIAN || params.type == CurvatureType::ALL);
    bool compute_min = (params.type == CurvatureType::MIN || params.type == CurvatureType::ALL);
    bool compute_max = (params.type == CurvatureType::MAX || params.type == CurvatureType::ALL);

    int smoothing = params.smooth ? params.smoothing_iterations : 0;

    // Note: PMP stores curvature in "v:curv" property, overwriting it each time
    // We need to extract values immediately after each computation

    std::vector<float> mean_values, gauss_values, min_values, max_values;
    size_t n_verts = pmp_mesh.n_vertices();

    if (compute_mean) {
      fmt::print("  Computing mean curvature...\n");
      pmp::curvature(pmp_mesh, pmp::Curvature::mean, smoothing);
      auto curv = pmp_mesh.get_vertex_property<pmp::Scalar>("v:curv");
      if (!curv) {
        fmt::print("  ERROR: v:curv property not found after mean curvature "
                   "computation!\n");
        return std::nullopt;
      }
      fmt::print("  Extracting {} mean curvature values...\n", n_verts);
      mean_values.resize(n_verts);
      for (size_t i = 0; i < n_verts; ++i) {
        mean_values[i] = curv[pmp::Vertex(i)];
      }
      fmt::print("  Computed mean curvature\n");
    }

    if (compute_gaussian) {
      pmp::curvature(pmp_mesh, pmp::Curvature::gauss, smoothing);
      auto curv = pmp_mesh.get_vertex_property<pmp::Scalar>("v:curv");
      gauss_values.resize(n_verts);
      for (size_t i = 0; i < n_verts; ++i) {
        gauss_values[i] = curv[pmp::Vertex(i)];
      }
    }

    if (compute_min) {
      pmp::curvature(pmp_mesh, pmp::Curvature::min, smoothing);
      auto curv = pmp_mesh.get_vertex_property<pmp::Scalar>("v:curv");
      min_values.resize(n_verts);
      for (size_t i = 0; i < n_verts; ++i) {
        min_values[i] = curv[pmp::Vertex(i)];
      }
    }

    if (compute_max) {
      pmp::curvature(pmp_mesh, pmp::Curvature::max, smoothing);
      auto curv = pmp_mesh.get_vertex_property<pmp::Scalar>("v:curv");
      max_values.resize(n_verts);
      for (size_t i = 0; i < n_verts; ++i) {
        max_values[i] = curv[pmp::Vertex(i)];
      }
    }
    fmt::print("Curvature: Computation complete\n");

    // Convert back to Nodo format
    fmt::print("Curvature: Converting to Nodo format...\n");
    auto result = detail::PMPConverter::from_pmp(pmp_mesh);
    fmt::print("Curvature: Converted, result has {} points\n", result.point_count());

    // Add extracted curvature attributes
    if (compute_mean) {
      fmt::print("  Adding mean_curvature attribute...\n");
      result.add_point_attribute("mean_curvature", core::AttributeType::FLOAT);
      auto* attr = result.get_point_attribute_typed<float>("mean_curvature");
      if (!attr) {
        fmt::print("  ERROR: Failed to get mean_curvature attribute!\n");
        return std::nullopt;
      }
      attr->resize(n_verts); // CRITICAL: Explicitly resize the storage!
      fmt::print("  Attribute resized to {} elements\n", n_verts);

      auto attr_span = attr->values_writable();
      fmt::print("  Copying {} mean curvature values...\n", n_verts);
      for (size_t i = 0; i < n_verts; ++i) {
        float value = mean_values[i];
        if (params.use_absolute)
          value = std::abs(value);
        attr_span[i] = value;
      }
      fmt::print("  Added mean_curvature attribute\n");
    }

    if (compute_gaussian) {
      result.add_point_attribute("gaussian_curvature", core::AttributeType::FLOAT);
      auto* attr = result.get_point_attribute_typed<float>("gaussian_curvature");
      attr->resize(n_verts);

      auto attr_span = attr->values_writable();
      for (size_t i = 0; i < n_verts; ++i) {
        float value = gauss_values[i];
        if (params.use_absolute)
          value = std::abs(value);
        attr_span[i] = value;
      }
      fmt::print("  Added gaussian_curvature attribute\n");
    }

    if (compute_min) {
      result.add_point_attribute("min_curvature", core::AttributeType::FLOAT);
      auto* attr = result.get_point_attribute_typed<float>("min_curvature");
      attr->resize(n_verts);

      auto attr_span = attr->values_writable();
      for (size_t i = 0; i < n_verts; ++i) {
        float value = min_values[i];
        if (params.use_absolute)
          value = std::abs(value);
        attr_span[i] = value;
      }
      fmt::print("  Added min_curvature attribute\n");
    }

    if (compute_max) {
      result.add_point_attribute("max_curvature", core::AttributeType::FLOAT);
      auto* attr = result.get_point_attribute_typed<float>("max_curvature");
      attr->resize(n_verts);

      auto attr_span = attr->values_writable();
      for (size_t i = 0; i < n_verts; ++i) {
        float value = max_values[i];
        if (params.use_absolute)
          value = std::abs(value);
        attr_span[i] = value;
      }
      fmt::print("  Added max_curvature attribute\n");
    }

    fmt::print("Curvature: Result has {} points, {} point attributes\n", result.point_count(),
               result.get_point_attribute_names().size());

    // Debug: Print all attribute names and verify they're accessible
    auto attr_names = result.get_point_attribute_names();
    fmt::print("Curvature: Point attribute names ({}): ", attr_names.size());
    for (const auto& name : attr_names) {
      fmt::print("'{}', ", name);
    }
    fmt::print("\n");

    // Verify attributes can be retrieved
    if (compute_mean) {
      auto* attr = result.get_point_attribute_typed<float>("mean_curvature");
      if (attr) {
        fmt::print("  mean_curvature: size={}, first value={}\n", attr->size(), attr->size() > 0 ? (*attr)[0] : 0.0f);
      } else {
        fmt::print("  mean_curvature: FAILED TO RETRIEVE!\n");
      }
    }

    return result;
  } catch (const std::exception& e) {
    fmt::print("Curvature: Failed to compute: {}\n", e.what());
    return std::nullopt;
  }
}

} // namespace nodo::processing
