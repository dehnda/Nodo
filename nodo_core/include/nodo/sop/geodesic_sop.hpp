#pragma once

#include "../core/geometry_container.hpp"
#include "../processing/geodesic.hpp"
#include "sop_node.hpp"
#include <fmt/core.h>

namespace nodo::sop {

/**
 * @brief Geodesic SOP - Computes geodesic (surface-following) distances from
 * seed points
 *
 * Provides two methods for computing geodesic distances:
 * - Dijkstra: Fast approximation using breadth-first search, requires triangle
 * mesh
 * - Heat: High-quality distances using heat diffusion, works on any polygon
 * mesh
 *
 * Use cases:
 * - Distance-based procedural effects that follow surface topology
 * - Creating falloff patterns that respect surface features
 * - Path finding and distance queries
 * - Heat diffusion simulation
 */
class GeodesicSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit GeodesicSOP(const std::string &node_name = "geodesic")
      : SOPNode(node_name, "Geodesic") {

    // Add input port
    input_ports_.add_port("0", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);

    // Method selection
    register_parameter(define_int_parameter("method", 1) // Default to Heat
                           .label("Method")
                           .options({"Dijkstra", "Heat"})
                           .category("Geodesic")
                           .description("Dijkstra: Fast, requires triangles. "
                                        "Heat: Quality, works on polygons")
                           .build());

    // Seed point group
    register_parameter(define_string_parameter("seed_group", "")
                           .label("Seed Group")
                           .category("Geodesic")
                           .description("Point group to use as seeds (empty = "
                                        "all points)")
                           .build());

    // Max distance (Dijkstra only)
    register_parameter(define_float_parameter("max_distance", 0.0F)
                           .label("Max Distance")
                           .range(0.0F, 1000.0F)
                           .category("Geodesic")
                           .description("Maximum distance to compute (0 = "
                                        "unlimited, Dijkstra only)")
                           .build());

    // Max neighbors (Dijkstra only)
    register_parameter(define_int_parameter("max_neighbors", 0)
                           .label("Max Neighbors")
                           .range(0, 100000)
                           .category("Geodesic")
                           .description("Maximum neighbors to process (0 = "
                                        "unlimited, Dijkstra only)")
                           .build());

    // Output attribute name
    register_parameter(
        define_string_parameter("output_attribute", "geodesic_dist")
            .label("Output Attribute")
            .category("Output")
            .description("Name of the output distance attribute")
            .build());
  }

  InputConfig get_input_config() const override {
    return InputConfig(InputType::SINGLE, 1, 1, 0);
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override;
};

} // namespace nodo::sop
