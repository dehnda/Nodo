#include "nodo/processing/geodesic.hpp"
#include "nodo/core/attribute_group.hpp"
#include "nodo/core/attribute_types.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/processing/pmp_converter.hpp"
#include <fmt/core.h>
#include <limits>
#include <pmp/algorithms/geodesics.h>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::processing {

std::optional<core::GeometryContainer>
Geodesic::compute(const core::GeometryContainer &input,
                  const GeodesicParams &params, std::string *error) {
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

    // Collect seed vertices
    std::vector<pmp::Vertex> seeds;

    fmt::print("Geodesic: seed_group parameter = '{}'\n", params.seed_group);

    if (params.seed_group.empty()) {
      // No group specified - use all vertices as seeds
      fmt::print("Geodesic: seed_group is empty, using all {} vertices\n",
                 pmp_mesh.n_vertices());
      for (auto v : pmp_mesh.vertices()) {
        seeds.push_back(v);
      }
    } else {
      // Get seed vertices from group
      fmt::print("Geodesic: Looking for group '{}'\n", params.seed_group);

      if (!core::has_group(input, params.seed_group,
                           core::ElementClass::POINT)) {
        if (error)
          *error = "Seed group '" + params.seed_group + "' not found";
        return std::nullopt;
      }

      auto members = core::get_group_elements(input, params.seed_group,
                                              core::ElementClass::POINT);

      fmt::print("Geodesic: seed_group='{}' has {} members\n",
                 params.seed_group, members.size());

      if (members.empty()) {
        if (error)
          *error = "Seed group '" + params.seed_group +
                   "' is empty (0 points selected). Select at least one point "
                   "in the Group node";
        return std::nullopt;
      }

      // Convert point indices to PMP vertices
      for (size_t idx : members) {
        if (idx < pmp_mesh.n_vertices()) {
          seeds.push_back(pmp::Vertex(static_cast<int>(idx)));
        }
      }

      fmt::print("Geodesic: converted {} members to {} seeds\n", members.size(),
                 seeds.size());

      if (seeds.empty()) {
        if (error)
          *error = "No valid seed vertices found in group (all indices out of "
                   "range). Group has " +
                   std::to_string(members.size()) +
                   " members but mesh only has " +
                   std::to_string(pmp_mesh.n_vertices()) + " vertices";
        return std::nullopt;
      }
    }

    // Compute geodesic distances based on method
    if (params.method == GeodesicMethod::Dijkstra) {
      // Dijkstra method - fast, can limit distance/count
      // Check if mesh is triangulated
      for (auto f : pmp_mesh.faces()) {
        if (pmp_mesh.valence(f) != 3) {
          if (error)
            *error = "Dijkstra geodesic method requires triangle mesh. Use "
                     "Heat method for quad/polygon meshes, or triangulate "
                     "first with Subdivide/Remesh";
          return std::nullopt;
        }
      }

      float max_dist = params.max_distance > 0.0F
                           ? params.max_distance
                           : std::numeric_limits<float>::max();
      unsigned int max_num = params.max_neighbors > 0
                                 ? params.max_neighbors
                                 : std::numeric_limits<unsigned int>::max();

      fmt::print(
          "Calling pmp::geodesics with {} seeds, max_dist={}, max_num={}\n",
          seeds.size(), max_dist, max_num);

      pmp::geodesics(pmp_mesh, seeds, max_dist, max_num);

      fmt::print("pmp::geodesics completed\n");

    } else if (params.method == GeodesicMethod::Heat) {
      // Heat method - higher quality, works on general meshes
      fmt::print("Calling pmp::geodesics_heat with {} seeds\n", seeds.size());

      // Debug: Check properties before
      fmt::print("Properties BEFORE geodesics_heat:\n");
      auto vprops_before = pmp_mesh.vertex_properties();
      for (const auto &name : vprops_before) {
        fmt::print("  - {}\n", name);
      }

      pmp::geodesics_heat(pmp_mesh, seeds);

      fmt::print("pmp::geodesics_heat completed\n");

      // Debug: Check properties after
      fmt::print("Properties AFTER geodesics_heat:\n");
      auto vprops_after = pmp_mesh.vertex_properties();
      for (const auto &name : vprops_after) {
        fmt::print("  - {}\n", name);
      }
    }

    // Convert back to GeometryContainer
    auto result = detail::PMPConverter::from_pmp_container(pmp_mesh, true);

    // Extract geodesic distances from PMP's "geodesic:distance" property
    auto geodesic_prop =
        pmp_mesh.get_vertex_property<float>("geodesic:distance");

    fmt::print("Checking for 'geodesic:distance' property: {}\n",
               geodesic_prop ? "found" : "NOT FOUND");

    if (geodesic_prop) {
      const size_t n_vertices = pmp_mesh.n_vertices();

      // Create distance attribute
      result.add_point_attribute(params.output_attribute,
                                 core::AttributeType::FLOAT);
      auto *dist_attr =
          result.get_point_attribute_typed<float>(params.output_attribute);
      dist_attr->resize(n_vertices);
      auto dist_writable = dist_attr->values_writable();

      // Copy distance values
      size_t idx = 0;
      for (auto v : pmp_mesh.vertices()) {
        dist_writable[idx] = geodesic_prop[v];
        ++idx;
      }

      fmt::print("Copied {} distance values to '{}' attribute\n", n_vertices,
                 params.output_attribute);
    } else {
      if (error)
        *error = "Failed to retrieve geodesic distances from computation. "
                 "PMP geodesic algorithm did not create 'geodesic:distance' "
                 "property.";
      return std::nullopt;
    }

    return std::move(result);

  } catch (const std::exception &e) {
    if (error)
      *error = std::string("Geodesic computation failed: ") + e.what();
    return std::nullopt;
  }
}

} // namespace nodo::processing
