#include "nodo/sop/merge_sop.hpp"

#include <iostream>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::sop {

MergeSOP::MergeSOP(const std::string& name) : SOPNode(name, "Merge") {
  // Add multiple input ports (up to 4 for now)
  for (int i = 0; i < 4; ++i) {
    input_ports_.add_port(std::to_string(i), NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);
  }

  // Optional parameter to control how many inputs are visible
  register_parameter(
      define_int_parameter("num_inputs", 2)
          .label("Number of Inputs")
          .range(1, 4)
          .category("Merge")
          .description("Number of visible input ports for merging geometry")
          .build());
}

std::shared_ptr<core::GeometryContainer> MergeSOP::execute() {
  std::cerr << "MergeSOP::execute() called\n";

  // Collect all connected inputs
  std::vector<std::shared_ptr<core::GeometryContainer>> inputs;

  for (int i = 0; i < 4; ++i) {
    auto input = get_input_data(i);
    if (input != nullptr) {
      std::cerr << "  Input " << i << ": " << input->topology().point_count()
                << " points, " << input->topology().primitive_count()
                << " prims\n";
      inputs.push_back(input);
    }
  }

  std::cerr << "  Total inputs collected: " << inputs.size() << "\n";

  if (inputs.empty()) {
    std::cerr << "  No inputs - returning empty geometry\n";
    // Return empty geometry instead of error for merge with no inputs
    return std::make_shared<core::GeometryContainer>();
  }

  // If only one input, just clone and return it
  if (inputs.size() == 1) {
    std::cerr << "  Single input - cloning\n";
    return std::make_shared<core::GeometryContainer>(inputs[0]->clone());
  }

  // Calculate total points and vertices
  size_t total_points = 0;
  size_t total_vertices = 0;

  for (const auto& input : inputs) {
    total_points += input->topology().point_count();
    total_vertices += input->topology().vertex_count();
  }

  std::cerr << "  Total points: " << total_points
            << ", vertices: " << total_vertices << "\n";

  // If total points is zero, return empty geometry
  if (total_points == 0) {
    std::cerr << "  Zero total points - returning empty\n";
    return std::make_shared<core::GeometryContainer>();
  }

  // Create result container
  auto result = std::make_shared<core::GeometryContainer>();
  std::cerr << "  Created result container\n";

  result->set_point_count(total_points);
  result->set_vertex_count(total_vertices);
  std::cerr << "  Set point count to " << total_points << ", vertex count to "
            << total_vertices << "\n";

  // Add position attribute
  result->add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  std::cerr << "  Added position attribute\n";

  auto* result_positions =
      result->get_point_attribute_typed<core::Vec3f>(attrs::P);

  if (result_positions == nullptr) {
    std::cerr << "  ERROR: Failed to create position attribute\n";
    set_error("Failed to create position attribute in result");
    return nullptr;
  }

  std::cerr << "  Got position attribute pointer\n";

  // Merge all inputs
  size_t point_offset = 0;
  size_t vertex_offset = 0;

  for (const auto& input : inputs) {
    std::cerr << "  Processing input at offset " << point_offset << "\n";

    const auto* input_positions =
        input->get_point_attribute_typed<core::Vec3f>(attrs::P);

    if (input_positions == nullptr) {
      std::cerr << "  WARNING: Input has no position attribute, skipping\n";
      continue; // Skip inputs without positions
    }

    const auto& input_topology = input->topology();
    const size_t input_point_count = input_topology.point_count();
    const size_t input_vertex_count = input_topology.vertex_count();
    const size_t input_prim_count = input_topology.primitive_count();

    std::cerr << "    Copying " << input_point_count << " points, "
              << input_vertex_count << " vertices, and " << input_prim_count
              << " prims\n";

    // Copy points
    for (size_t pt_idx = 0; pt_idx < input_point_count; ++pt_idx) {
      (*result_positions)[point_offset + pt_idx] = (*input_positions)[pt_idx];
    }

    std::cerr << "    Points copied\n";

    // Copy primitives with adjusted vertex indices
    std::cerr << "    Starting primitive copy loop, count=" << input_prim_count
              << "\n";

    for (size_t prim_idx = 0; prim_idx < input_prim_count; ++prim_idx) {
      if (prim_idx % 200 == 0) {
        std::cerr << "      Processing prim " << prim_idx << "/"
                  << input_prim_count << "\n";
      }

      const auto& input_verts = input_topology.get_primitive_vertices(prim_idx);
      std::vector<int> result_prim_verts;
      result_prim_verts.reserve(input_verts.size());

      for (size_t vert_idx : input_verts) {
        const int input_point_idx = input_topology.get_vertex_point(vert_idx);
        const int result_point_idx =
            static_cast<int>(point_offset) + input_point_idx;
        const int result_vert_idx =
            static_cast<int>(vertex_offset) + static_cast<int>(vert_idx);

        // Set up vertex-to-point mapping
        result->topology().set_vertex_point(result_vert_idx, result_point_idx);
        result_prim_verts.push_back(result_vert_idx);
      }

      try {
        result->add_primitive(result_prim_verts);
      } catch (const std::exception& e) {
        std::cerr << "      ERROR adding primitive " << prim_idx << ": "
                  << e.what() << "\n";
        set_error(std::string("Failed to add primitive: ") + e.what());
        return nullptr;
      }
    }

    std::cerr << "    Primitives copied (" << input_prim_count << " total)\n";

    point_offset += input_point_count;
    vertex_offset += input_vertex_count;
  }

  std::cerr << "  Merge complete - returning "
            << result->topology().point_count() << " points, "
            << result->topology().primitive_count() << " prims\n";

  return result;
}

} // namespace nodo::sop
