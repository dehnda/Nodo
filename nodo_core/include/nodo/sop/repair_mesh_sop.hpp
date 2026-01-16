#pragma once

#include "../core/geometry_container.hpp"
#include "../processing/hole_filling.hpp"
#include "sop_node.hpp"

#include <fmt/core.h>

namespace nodo::sop {

/**
 * @brief RepairMesh SOP - Repairs meshes by filling holes and fixing issues
 *
 * Automatically detects and fills holes in geometry.
 * Uses PMP library's hole filling algorithm.
 *
 * Useful for:
 * - Repairing scanned meshes
 * - Fixing incomplete models
 * - Preparing meshes for 3D printing
 * - Closing gaps in imported geometry
 */
class RepairMeshSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit RepairMeshSOP(const std::string& node_name = "repair_mesh") : SOPNode(node_name, "RepairMesh") {
    // Add input port
    input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

    // Minimum hole size
    register_parameter(define_int_parameter("min_hole_size", 0)
                           .label("Min Hole Size")
                           .range(0, 1000)
                           .category("Hole Filling")
                           .description("Minimum hole size to fill (number of "
                                        "boundary edges). 0 = fill all holes")
                           .build());

    // Maximum hole size
    register_parameter(define_int_parameter("max_hole_size", 0)
                           .label("Max Hole Size")
                           .range(0, 10000)
                           .category("Hole Filling")
                           .description("Maximum hole size to fill (number of "
                                        "boundary edges). 0 = no limit")
                           .build());

    // Refine fill
    register_parameter(define_bool_parameter("refine_fill", true)
                           .label("Refine Fill")
                           .category("Hole Filling")
                           .description("Refine filled regions for better mesh quality")
                           .build());
  }

  InputConfig get_input_config() const override { return InputConfig(InputType::SINGLE, 1, 1, 0); }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    // Get input
    auto input_data = get_input_data(0);
    if (!input_data) {
      fmt::print("RepairMeshSOP: No input geometry\n");
      return nullptr;
    }

    // Get parameters
    processing::HoleFillingParams params;
    params.min_hole_size = get_parameter<int>("min_hole_size", 0);
    params.max_hole_size = get_parameter<int>("max_hole_size", 0);
    params.refine_fill = get_parameter<bool>("refine_fill", true);

    fmt::print("RepairMeshSOP: Processing geometry (min_hole_size={}, "
               "max_hole_size={})\n",
               params.min_hole_size, params.max_hole_size);

    // Fill holes
    auto result = processing::HoleFilling::fill_holes(*input_data, params);

    if (result) {
      fmt::print("RepairMeshSOP: Repair complete\n");
      return std::make_shared<core::GeometryContainer>(std::move(*result));
    } else {
      fmt::print("RepairMeshSOP: Repair failed\n");
      return nullptr;
    }
  }
};

} // namespace nodo::sop
