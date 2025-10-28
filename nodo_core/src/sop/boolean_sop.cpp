#include "nodo/sop/boolean_sop.hpp"
#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/geometry/boolean_ops.hpp"

namespace attrs = nodo::core::standard_attrs;

namespace nodo::sop {

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

  // Triangulate: Convert quads/polygons to triangles
  std::vector<Eigen::Vector3i> triangle_list;
  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &vert_indices = topology.get_primitive_vertices(prim_idx);

    // Get point indices for this primitive
    std::vector<int> point_indices;
    for (size_t vi : vert_indices) {
      point_indices.push_back(topology.get_vertex_point(vi));
    }

    // Triangulate the polygon (simple fan triangulation)
    if (point_indices.size() >= 3) {
      for (size_t i = 1; i + 1 < point_indices.size(); ++i) {
        triangle_list.emplace_back(point_indices[0], point_indices[i], point_indices[i + 1]);
      }
    }
  }

  // Convert triangle list to Eigen matrix
  Eigen::MatrixXi faces(triangle_list.size(), 3);
  for (size_t i = 0; i < triangle_list.size(); ++i) {
    faces.row(i) = triangle_list[i];
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
  size_t total_verts = faces.rows() * faces.cols();
  container->topology().set_vertex_count(total_verts);

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
  if (positions != nullptr) {
    for (int i = 0; i < vertices.rows(); ++i) {
      (*positions)[i] = vertices.row(i).cast<float>();
    }
  }

  return container;
}

BooleanSOP::BooleanSOP(const std::string &node_name)
    : SOPNode(node_name, "Boolean") {
  // Add input ports (using numeric names for execution engine compatibility)
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);
  input_ports_.add_port("1", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_int_parameter("operation", 0)
                         .label("Operation")
                         .options({"Union", "Intersection", "Difference"})
                         .category("Boolean")
                         .build());
}

std::shared_ptr<core::GeometryContainer> BooleanSOP::execute() {
  // Get input geometries (using numeric port names)
  auto input_a = get_input_data(0);  // Port "0"
  auto input_b = get_input_data(1);  // Port "1"

  if (!input_a) {
    set_error("Missing input geometry A");
    return nullptr;
  }

  if (!input_b) {
    set_error("Missing input geometry B");
    return nullptr;
  }

  // Get operation type parameter
  const int operation = get_parameter<int>("operation", 0);

  // Convert GeometryContainers to Meshes
  auto mesh_a = container_to_mesh(*input_a);
  auto mesh_b = container_to_mesh(*input_b);

  if (!mesh_a) {
    set_error("Failed to convert input A to mesh");
    return nullptr;
  }

  if (!mesh_b) {
    set_error("Failed to convert input B to mesh");
    return nullptr;
  }

  // Perform the boolean operation based on parameter
  std::optional<core::Mesh> result;

  switch (operation) {
  case 0: // UNION
    result = geometry::BooleanOps::union_meshes(*mesh_a, *mesh_b);
    break;
  case 1: // INTERSECTION
    result = geometry::BooleanOps::intersect_meshes(*mesh_a, *mesh_b);
    break;
  case 2: // DIFFERENCE
    result = geometry::BooleanOps::difference_meshes(*mesh_a, *mesh_b);
    break;
  default:
    set_error("Invalid operation type: " + std::to_string(operation));
    return nullptr;
  }

  // Check if operation succeeded
  if (!result.has_value()) {
    const auto &error = geometry::BooleanOps::last_error();
    set_error("Boolean operation failed: " + error.message);
    return nullptr;
  }

  // Convert result mesh back to GeometryContainer
  auto output = mesh_to_container(*result);

  if (!output) {
    set_error("Failed to convert result mesh to container");
    return nullptr;
  }

  return output;
}
} // namespace nodo::sop
