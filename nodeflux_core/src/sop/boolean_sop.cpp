#include "nodeflux/sop/boolean_sop.hpp"
#include "nodeflux/geometry/boolean_ops.hpp"
#include "nodeflux/core/standard_attributes.hpp"
#include "nodeflux/core/geometry_container.hpp"
#include <iostream>

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::sop {

// Helper to convert GeometryData to Mesh for BooleanOps (temporary bridge)
static std::shared_ptr<core::Mesh>
container_to_mesh(const core::GeometryContainer &container) {
  const auto &topology = container.topology();

  auto *p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (!p_storage)
    return nullptr;

  Eigen::MatrixXd vertices(topology.point_count(), 3);
  auto p_span = p_storage->values();
  for (size_t i = 0; i < p_span.size(); ++i) {
    vertices.row(i) = p_span[i].cast<double>();
  }

  Eigen::MatrixXi faces(topology.primitive_count(), 3);
  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &vert_indices = topology.get_primitive_vertices(prim_idx);
    for (size_t j = 0; j < 3 && j < vert_indices.size(); ++j) {
      faces(prim_idx, j) = topology.get_vertex_point(vert_indices[j]);
    }
  }

  return std::make_shared<core::Mesh>(vertices, faces);
}

// Helper to convert Mesh back to GeometryContainer
static std::shared_ptr<core::GeometryContainer>
mesh_to_container(const core::Mesh &mesh) {
  auto container = std::make_shared<core::GeometryContainer>();
  const auto &vertices = mesh.vertices();
  const auto &faces = mesh.faces();

  container->set_point_count(vertices.rows());
  
  // Build topology
  size_t vert_idx = 0;
  for (int face_idx = 0; face_idx < faces.rows(); ++face_idx) {
    std::vector<int> prim_verts;
    for (int j = 0; j < faces.cols(); ++j) {
      int point_idx = faces(face_idx, j);
      container->topology().set_vertex_point(vert_idx, point_idx);
      prim_verts.push_back(static_cast<int>(vert_idx));
      ++vert_idx;
    }
    container->add_primitive(prim_verts);
  }

  // Copy positions
  container->add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *positions = container->get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (positions) {
    for (int i = 0; i < vertices.rows(); ++i) {
      (*positions)[i] = vertices.row(i).cast<float>();
    }
  }

  return container;
}

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

  // Get input geometry from ports
  auto input_a = get_input_data("mesh_a");
  auto input_b = get_input_data("mesh_b");

  if (!input_a || !input_b) {
    std::cout << "BooleanSOP '" << get_name()
              << "': Missing input meshes - A: " << (input_a ? "✓" : "✗")
              << ", B: " << (input_b ? "✓" : "✗") << "\n";
    set_error("Missing input geometry");
    return nullptr;
  }

  // Convert inputs to Mesh for BooleanOps (which requires Mesh)
  // TODO: When BooleanOps supports GeometryContainer, remove these conversions
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

    // Convert result Mesh back to GeometryContainer and then to GeometryData
    auto result_container = mesh_to_container(result_mesh.value());
    
    // Convert container to old GeometryData format
    auto result_mesh_ptr = container_to_mesh(*result_container);
    return std::make_shared<GeometryData>(result_mesh_ptr);

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
