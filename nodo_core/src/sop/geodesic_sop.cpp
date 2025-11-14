#include "nodo/sop/geodesic_sop.hpp"

#include "nodo/processing/geodesic.hpp"

#include <fmt/core.h>

namespace nodo::sop {

std::shared_ptr<core::GeometryContainer> GeodesicSOP::execute() {
  // Get input
  auto input_data = get_input_data(0);
  if (!input_data) {
    fmt::print("GeodesicSOP: No input geometry\n");
    return nullptr;
  }

  // Get parameters
  processing::GeodesicParams params;

  // Method selection
  int method_index = get_parameter<int>("method", 1); // Default to Heat
  switch (method_index) {
    case 0:
      params.method = processing::GeodesicMethod::Dijkstra;
      break;
    case 1:
      params.method = processing::GeodesicMethod::Heat;
      break;
    default:
      params.method = processing::GeodesicMethod::Heat; // Default to Heat
      break;
  }

  // Seed group
  params.seed_group = get_parameter<std::string>("seed_group", "");
  fmt::print("GeodesicSOP: Read seed_group parameter = '{}'\n",
             params.seed_group);

  // Distance and neighbor limits (Dijkstra only)
  params.max_distance = get_parameter<float>("max_distance", 0.0F);
  params.max_neighbors = get_parameter<int>("max_neighbors", 0);

  // Output attribute name
  params.output_attribute =
      get_parameter<std::string>("output_attribute", "geodesic_dist");

  // Compute geodesic distances
  std::string error;
  auto result = processing::Geodesic::compute(*input_data, params, &error);

  if (!result) {
    fmt::print("GeodesicSOP: Failed to compute geodesic distances: {}\n",
               error);
    return nullptr;
  }

  return std::make_shared<core::GeometryContainer>(std::move(*result));
}

} // namespace nodo::sop
