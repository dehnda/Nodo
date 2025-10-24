#include "nodeflux/sop/boolean_sop.hpp"
#include "nodeflux/geometry/boolean_ops.hpp"
#include <iostream>

namespace nodeflux::sop {

BooleanSOP::BooleanSOP(const std::string &node_name)
    : SOPNode(node_name, "Boolean") {
  // Add input ports
  input_ports_.add_port("mesh_a", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);
  input_ports_.add_port("mesh_b", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_int_parameter("operation", 0)
                         .label("Operation")
                         .options({"Union", "Intersection", "Difference"})
                         .category("Boolean")
                         .build());
}

std::shared_ptr<GeometryData> BooleanSOP::execute() {
  // Read parameters from parameter system
  const auto operation =
      static_cast<OperationType>(get_parameter<int>("operation", 0));

  // Get input meshes from ports
  auto input_a = get_input_data("mesh_a");
  auto input_b = get_input_data("mesh_b");

  if (!input_a || !input_b) {
    std::cout << "BooleanSOP '" << get_name()
              << "': Missing input meshes - A: " << (input_a ? "✓" : "✗")
              << ", B: " << (input_b ? "✓" : "✗") << "\n";
    set_error("Missing input geometry");
    return nullptr;
  }

  auto mesh_a = input_a->get_mesh();
  auto mesh_b = input_b->get_mesh();

  if (!mesh_a || !mesh_b) {
    set_error("Input geometry does not contain valid meshes");
    return nullptr;
  }

  std::cout << "BooleanSOP '" << get_name() << "': Computing "
            << operation_to_string(operation) << " operation with mesh A ("
            << mesh_a->vertex_count() << " verts) and mesh B ("
            << mesh_b->vertex_count() << " verts)\n";

  try {
    std::optional<core::Mesh> result_mesh;

    switch (operation) {
    case OperationType::UNION:
      result_mesh = geometry::BooleanOps::union_meshes(*mesh_a, *mesh_b);
      break;

    case OperationType::INTERSECTION:
      result_mesh = geometry::BooleanOps::intersect_meshes(*mesh_a, *mesh_b);
      break;

    case OperationType::DIFFERENCE:
      result_mesh = geometry::BooleanOps::difference_meshes(*mesh_a, *mesh_b);
      break;

    default:
      set_error("Unknown boolean operation");
      return nullptr;
    }

    if (!result_mesh.has_value()) {
      std::cerr << "BooleanSOP '" << get_name()
                << "': Boolean operation failed: "
                << geometry::BooleanOps::last_error().message << "\n";
      set_error("Boolean operation failed");
      return nullptr;
    }

    return std::make_shared<GeometryData>(
        std::make_shared<core::Mesh>(std::move(result_mesh.value())));

  } catch (const std::exception &exception) {
    std::cout << "BooleanSOP '" << get_name()
              << "': Exception: " << exception.what() << "\n";
    set_error("Boolean operation exception");
    return nullptr;
  }
}

std::string BooleanSOP::operation_to_string(OperationType operation) {
  switch (operation) {
  case OperationType::UNION:
    return "Union";
  case OperationType::INTERSECTION:
    return "Intersection";
  case OperationType::DIFFERENCE:
    return "Difference";
  default:
    return "Unknown";
  }
}

} // namespace nodeflux::sop
