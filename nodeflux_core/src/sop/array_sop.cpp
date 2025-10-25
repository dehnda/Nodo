#include "nodeflux/sop/array_sop.hpp"
#include "nodeflux/core/math.hpp"
#include "nodeflux/core/types.hpp"
#include "nodeflux/sop/geometry_data.hpp"

#include <cmath>
#include <iostream>

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::sop {

ArraySOP::ArraySOP(const std::string &name) : SOPNode(name, "Array") {
  // Add input port
  input_ports_.add_port("mesh", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_int_parameter("array_type", 0)
                         .label("Array Type")
                         .options({"Linear", "Radial", "Grid"})
                         .category("Array")
                         .build());

  register_parameter(define_int_parameter("count", 3)
                         .label("Count")
                         .range(1, 100)
                         .category("Array")
                         .build());

  // Linear array parameters
  register_parameter(define_float_parameter("linear_offset_x", 1.0F)
                         .label("Offset X")
                         .range(-100.0, 100.0)
                         .category("Linear")
                         .build());

  register_parameter(define_float_parameter("linear_offset_y", 0.0F)
                         .label("Offset Y")
                         .range(-100.0, 100.0)
                         .category("Linear")
                         .build());

  register_parameter(define_float_parameter("linear_offset_z", 0.0F)
                         .label("Offset Z")
                         .range(-100.0, 100.0)
                         .category("Linear")
                         .build());

  // Radial array parameters
  register_parameter(define_float_parameter("radial_center_x", 0.0F)
                         .label("Center X")
                         .range(-100.0, 100.0)
                         .category("Radial")
                         .build());

  register_parameter(define_float_parameter("radial_center_y", 0.0F)
                         .label("Center Y")
                         .range(-100.0, 100.0)
                         .category("Radial")
                         .build());

  register_parameter(define_float_parameter("radial_center_z", 0.0F)
                         .label("Center Z")
                         .range(-100.0, 100.0)
                         .category("Radial")
                         .build());

  register_parameter(define_float_parameter("radial_radius", 2.0F)
                         .label("Radius")
                         .range(0.0, 100.0)
                         .category("Radial")
                         .build());

  register_parameter(define_float_parameter("angle_step", 60.0F)
                         .label("Angle Step")
                         .range(0.0, 360.0)
                         .category("Radial")
                         .build());

  // Grid array parameters
  register_parameter(define_int_parameter("grid_width", 3)
                         .label("Grid Width")
                         .range(1, 50)
                         .category("Grid")
                         .build());

  register_parameter(define_int_parameter("grid_height", 3)
                         .label("Grid Height")
                         .range(1, 50)
                         .category("Grid")
                         .build());

  register_parameter(define_float_parameter("grid_spacing_x", 1.0F)
                         .label("Spacing X")
                         .range(0.01, 100.0)
                         .category("Grid")
                         .build());

  register_parameter(define_float_parameter("grid_spacing_y", 1.0F)
                         .label("Spacing Y")
                         .range(0.01, 100.0)
                         .category("Grid")
                         .build());
}

// Helper to convert GeometryData to GeometryContainer (bridge for migration)
static std::unique_ptr<core::GeometryContainer>
convert_to_container(const GeometryData &old_data) {
  auto container = std::make_unique<core::GeometryContainer>();

  auto mesh = old_data.get_mesh();
  if (!mesh || mesh->empty()) {
    return container;
  }

  const auto &vertices = mesh->vertices();
  const auto &faces = mesh->faces();

  // Set up topology
  container->set_point_count(vertices.rows());
  container->set_vertex_count(vertices.rows()); // 1:1 mapping

  auto &topology = container->topology();

  // Set up 1:1 vertex→point mapping
  for (size_t i = 0; i < static_cast<size_t>(vertices.rows()); ++i) {
    topology.set_vertex_point(i, static_cast<int>(i));
  }

  // Add primitives
  for (int i = 0; i < faces.rows(); ++i) {
    std::vector<int> prim_verts(3);
    for (int j = 0; j < 3; ++j) {
      prim_verts[j] = faces(i, j);
    }
    topology.add_primitive(prim_verts);
  }

  // Add position attribute
  container->add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *p_storage = container->get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage) {
    auto p_span = p_storage->values_writable();
    for (size_t i = 0; i < static_cast<size_t>(vertices.rows()); ++i) {
      p_span[i] = vertices.row(i).cast<float>();
    }
  }

  return container;
}

// Helper to convert GeometryContainer to GeometryData (bridge for migration)
static std::shared_ptr<GeometryData>
convert_from_container(const core::GeometryContainer &container) {
  const auto &topology = container.topology();

  // Extract positions
  auto *p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (!p_storage)
    return std::make_shared<GeometryData>(std::make_shared<core::Mesh>());

  Eigen::MatrixXd vertices(topology.point_count(), 3);
  auto p_span = p_storage->values();
  for (size_t i = 0; i < p_span.size(); ++i) {
    vertices.row(i) = p_span[i].cast<double>();
  }

  // Extract faces - convert vertex indices to point indices
  Eigen::MatrixXi faces(topology.primitive_count(), 3);
  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &vert_indices = topology.get_primitive_vertices(prim_idx);
    for (size_t j = 0; j < 3 && j < vert_indices.size(); ++j) {
      // Convert vertex index to point index
      faces(prim_idx, j) = topology.get_vertex_point(vert_indices[j]);
    }
  }

  auto mesh = std::make_shared<core::Mesh>(vertices, faces);
  return std::make_shared<GeometryData>(mesh);
}

std::shared_ptr<GeometryData> ArraySOP::execute() {
  std::cerr << "DEBUG: ArraySOP::execute() called\n";

  // Get input geometry from port
  auto input_geo = get_input_data("mesh");

  std::cerr << "DEBUG: input_geo is null? " << (input_geo == nullptr) << "\n";

  if (!input_geo) {
    // DEBUG: Print port info
    auto *port = input_ports_.get_port("mesh");
    std::cerr << "ArraySOP::execute() - input_geo is null!\n";
    std::cerr << "  Port exists: " << (port != nullptr) << "\n";
    if (port) {
      std::cerr << "  Port has data: " << (port->get_data() != nullptr) << "\n";
    }
    set_error("No input geometry connected");
    return nullptr;
  }

  // Convert to GeometryContainer
  std::cerr << "DEBUG: About to convert to container\n";

  try {
    auto input_container = convert_to_container(*input_geo);

    std::cerr << "DEBUG: Converted to container, point_count = "
              << input_container->topology().point_count() << "\n";

    if (input_container->topology().point_count() == 0) {
      set_error("Input geometry is empty");
      return nullptr;
    }
  } catch (const std::exception &e) {
    std::cerr << "DEBUG: Exception in convert_to_container: " << e.what()
              << "\n";
    set_error(std::string("Exception: ") + e.what());
    return nullptr;
  }

  // Extract mesh from GeometryData (for array processing)
  auto input_mesh = input_geo->get_mesh();
  if (!input_mesh) {
    set_error("Input geometry does not contain a mesh");
    return nullptr;
  }

  // Store input sizes for attribute tracking
  size_t input_vert_count = input_mesh->vertices().rows();
  size_t input_face_count = input_mesh->faces().rows();

  // Read parameters from parameter system
  const auto array_type =
      static_cast<ArrayType>(get_parameter<int>("array_type", 0));
  const int count = get_parameter<int>("count", 3);

  // Execute the appropriate array operation
  std::optional<core::Mesh> result_mesh;
  int effective_count = count;

  switch (array_type) {
  case ArrayType::LINEAR:
    result_mesh = create_linear_array(*input_mesh, count);
    break;
  case ArrayType::RADIAL:
    result_mesh = create_radial_array(*input_mesh, count);
    break;
  case ArrayType::GRID: {
    const int grid_width = get_parameter<int>("grid_width", 3);
    const int grid_height = get_parameter<int>("grid_height", 3);
    effective_count = grid_width * grid_height;
    result_mesh = create_grid_array(*input_mesh, grid_width, grid_height);
    break;
  }
  default:
    set_error("Unknown array type");
    return nullptr;
  }

  // Check if operation succeeded
  if (!result_mesh) {
    set_error("Array operation failed");
    return nullptr;
  }

  // Convert result to GeometryContainer
  core::GeometryContainer output_container;
  auto &output_topology = output_container.topology();

  const auto &output_verts = result_mesh->vertices();
  const auto &output_faces = result_mesh->faces();

  output_topology.set_point_count(output_verts.rows());

  // Add primitives
  for (int i = 0; i < output_faces.rows(); ++i) {
    std::vector<int> prim_verts(3);
    for (int j = 0; j < 3; ++j) {
      prim_verts[j] = output_faces(i, j);
    }
    output_topology.add_primitive(prim_verts);
  }

  // Add position attribute
  output_container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *p_storage =
      output_container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage) {
    auto p_span = p_storage->values_writable();
    for (size_t i = 0; i < static_cast<size_t>(output_verts.rows()); ++i) {
      p_span[i] = output_verts.row(i).cast<float>();
    }
  }

  // Convert back to GeometryData for compatibility
  auto output_data = convert_from_container(output_container);

  // Add instance tracking attributes
  add_instance_attributes(output_data, input_vert_count, input_face_count,
                          effective_count, array_type);

  return output_data;
}

std::optional<core::Mesh>
ArraySOP::create_linear_array(const core::Mesh &input_mesh, int count) {
  const auto &input_vertices = input_mesh.vertices();
  const auto &input_faces = input_mesh.faces();

  // Read linear offset parameters
  const core::Vector3 linear_offset(
      get_parameter<float>("linear_offset_x", 1.0F),
      get_parameter<float>("linear_offset_y", 0.0F),
      get_parameter<float>("linear_offset_z", 0.0F));

  // Calculate total size
  int total_vertices = input_vertices.rows() * count;
  int total_faces = input_faces.rows() * count;

  // Prepare output matrices
  core::Mesh::Vertices output_vertices(total_vertices, 3);
  core::Mesh::Faces output_faces(total_faces, 3);

  // Copy geometry for each array element
  for (int i = 0; i < count; ++i) {
    // Calculate offset for this copy
    core::Vector3 current_offset =
        (linear_offset * static_cast<float>(i)).cast<double>();

    // Copy vertices with offset
    int vertex_start = i * input_vertices.rows();
    for (int v = 0; v < input_vertices.rows(); ++v) {
      output_vertices.row(vertex_start + v) =
          input_vertices.row(v) + current_offset.transpose();
    }

    // Copy faces with vertex index offset
    int face_start = i * input_faces.rows();
    int vertex_offset = i * input_vertices.rows();
    for (int f = 0; f < input_faces.rows(); ++f) {
      output_faces.row(face_start + f) =
          input_faces.row(f).array() + vertex_offset;
    }
  }

  return core::Mesh(std::move(output_vertices), std::move(output_faces));
}

std::optional<core::Mesh>
ArraySOP::create_radial_array(const core::Mesh &input_mesh, int count) {
  const auto &input_vertices = input_mesh.vertices();
  const auto &input_faces = input_mesh.faces();

  // Read radial parameters
  const core::Vector3 radial_center(
      get_parameter<float>("radial_center_x", 0.0F),
      get_parameter<float>("radial_center_y", 0.0F),
      get_parameter<float>("radial_center_z", 0.0F));
  const float radial_radius = get_parameter<float>("radial_radius", 2.0F);
  const float angle_step = get_parameter<float>("angle_step", 60.0F);

  // Calculate total size
  int total_vertices = input_vertices.rows() * count;
  int total_faces = input_faces.rows() * count;

  // Prepare output matrices
  core::Mesh::Vertices output_vertices(total_vertices, 3);
  core::Mesh::Faces output_faces(total_faces, 3);

  // Copy geometry for each array element
  for (int i = 0; i < count; ++i) {
    // Calculate rotation angle
    double angle_rad = core::math::degrees_to_radians(angle_step * i);
    auto rotation = core::math::rotation_z(angle_rad);

    // Calculate position offset (radial positioning)
    core::Vector3 position_offset = radial_center.cast<double>();
    if (radial_radius > 0.0F) {
      position_offset +=
          core::math::circular_offset_2d(radial_radius, angle_rad);
    }

    // Copy vertices with rotation and translation
    int vertex_start = i * input_vertices.rows();
    for (int v = 0; v < input_vertices.rows(); ++v) {
      // Get the original vertex and transform it
      core::Vector3 original_vertex = input_vertices.row(v).transpose();
      core::Vector3 transformed_vertex = core::math::apply_transform(
          original_vertex, rotation, position_offset);
      output_vertices.row(vertex_start + v) = transformed_vertex.transpose();
    }

    // Copy faces with vertex index offset
    int face_start = i * input_faces.rows();
    int vertex_offset = i * input_vertices.rows();
    for (int f = 0; f < input_faces.rows(); ++f) {
      output_faces.row(face_start + f) =
          input_faces.row(f).array() + vertex_offset;
    }
  }

  return core::Mesh(std::move(output_vertices), std::move(output_faces));
}

std::optional<core::Mesh>
ArraySOP::create_grid_array(const core::Mesh &input_mesh, int grid_width,
                            int grid_height) {
  const auto &input_vertices = input_mesh.vertices();
  const auto &input_faces = input_mesh.faces();

  // Read grid spacing parameters
  const float grid_spacing_x = get_parameter<float>("grid_spacing_x", 1.0F);
  const float grid_spacing_y = get_parameter<float>("grid_spacing_y", 1.0F);

  int grid_count = grid_width * grid_height;

  // Calculate total size
  int total_vertices = input_vertices.rows() * grid_count;
  int total_faces = input_faces.rows() * grid_count;

  // Prepare output matrices
  core::Mesh::Vertices output_vertices(total_vertices, 3);
  core::Mesh::Faces output_faces(total_faces, 3);

  int copy_index = 0;

  // Copy geometry for each grid position
  for (int y = 0; y < grid_height; ++y) {
    for (int x = 0; x < grid_width; ++x) {
      // Calculate grid position offset
      core::Vector3 grid_offset(x * grid_spacing_x, y * grid_spacing_y, 0.0);

      // Copy vertices with offset
      int vertex_start = copy_index * input_vertices.rows();
      for (int v = 0; v < input_vertices.rows(); ++v) {
        output_vertices.row(vertex_start + v) =
            input_vertices.row(v) + grid_offset.transpose();
      }

      // Copy faces with vertex index offset
      int face_start = copy_index * input_faces.rows();
      int vertex_offset = copy_index * input_vertices.rows();
      for (int f = 0; f < input_faces.rows(); ++f) {
        output_faces.row(face_start + f) =
            input_faces.row(f).array() + vertex_offset;
      }

      copy_index++;
    }
  }

  return core::Mesh(std::move(output_vertices), std::move(output_faces));
}

void ArraySOP::add_instance_attributes(std::shared_ptr<GeometryData> geo_data,
                                       size_t verts_per_instance,
                                       size_t faces_per_instance,
                                       int instance_count,
                                       ArrayType array_type) {
  // Add per-vertex instance ID attribute
  GeometryData::AttributeArray vertex_instance_ids;
  vertex_instance_ids.reserve(verts_per_instance * instance_count);

  for (int copy = 0; copy < instance_count; ++copy) {
    for (size_t v = 0; v < verts_per_instance; ++v) {
      vertex_instance_ids.push_back(copy);
    }
  }
  geo_data->set_vertex_attribute("instance_id", vertex_instance_ids);

  // Add per-face instance ID attribute
  GeometryData::AttributeArray face_instance_ids;
  face_instance_ids.reserve(faces_per_instance * instance_count);

  for (int copy = 0; copy < instance_count; ++copy) {
    for (size_t f = 0; f < faces_per_instance; ++f) {
      face_instance_ids.push_back(copy);
    }
  }
  geo_data->set_face_attribute("instance_id", face_instance_ids);

  // Add global metadata
  geo_data->set_global_attribute("instance_count", instance_count);
  geo_data->set_global_attribute("array_type", static_cast<int>(array_type));
}

} // namespace nodeflux::sop
