#include "nodeflux/sop/mirror_sop.hpp"
#include "nodeflux/core/math.hpp"

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::sop {

MirrorSOP::MirrorSOP(const std::string &name)
    : SOPNode(name, "Mirror") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(
      define_int_parameter("plane", 2)
          .label("Plane")
          .options({"XY", "XZ", "YZ", "Custom"})
          .category("Mirror")
          .build());

  register_parameter(
      define_int_parameter("keep_original", 1)
          .label("Keep Original")
          .range(0, 1)
          .category("Mirror")
          .build());

  // Custom plane point
  register_parameter(
      define_float_parameter("custom_point_x", 0.0F)
          .label("Custom Point X")
          .range(-100.0, 100.0)
          .category("Custom Plane")
          .build());

  register_parameter(
      define_float_parameter("custom_point_y", 0.0F)
          .label("Custom Point Y")
          .range(-100.0, 100.0)
          .category("Custom Plane")
          .build());

  register_parameter(
      define_float_parameter("custom_point_z", 0.0F)
          .label("Custom Point Z")
          .range(-100.0, 100.0)
          .category("Custom Plane")
          .build());

  // Custom plane normal
  register_parameter(
      define_float_parameter("custom_normal_x", 0.0F)
          .label("Custom Normal X")
          .range(-1.0, 1.0)
          .category("Custom Plane")
          .build());

  register_parameter(
      define_float_parameter("custom_normal_y", 1.0F)
          .label("Custom Normal Y")
          .range(-1.0, 1.0)
          .category("Custom Plane")
          .build());

  register_parameter(
      define_float_parameter("custom_normal_z", 0.0F)
          .label("Custom Normal Z")
          .range(-1.0, 1.0)
          .category("Custom Plane")
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
  auto &topology = container->topology();
  topology.set_point_count(vertices.rows());

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

  // Add normals if available
  try {
    const auto &normals = mesh->vertex_normals();
    container->add_point_attribute(attrs::N, core::AttributeType::VEC3F);
    auto *n_storage = container->get_point_attribute_typed<core::Vec3f>(attrs::N);
    if (n_storage) {
      auto n_span = n_storage->values_writable();
      for (size_t i = 0; i < static_cast<size_t>(normals.rows()); ++i) {
        n_span[i] = normals.row(i).cast<float>();
      }
    }
  } catch (...) {
    // No normals available
  }

  return container;
}

// Helper to convert GeometryContainer to GeometryData (bridge for migration)
static std::shared_ptr<GeometryData>
convert_from_container(const core::GeometryContainer &container) {
  const auto &topology = container.topology();

  // Extract positions
  auto *p_storage =
      container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (!p_storage)
    return std::make_shared<GeometryData>(std::make_shared<core::Mesh>());

  Eigen::MatrixXd vertices(topology.point_count(), 3);
  auto p_span = p_storage->values();
  for (size_t i = 0; i < p_span.size(); ++i) {
    vertices.row(i) = p_span[i].cast<double>();
  }

  // Extract faces
  Eigen::MatrixXi faces(topology.primitive_count(), 3);
  for (size_t prim_idx = 0; prim_idx < topology.primitive_count(); ++prim_idx) {
    const auto &verts = topology.get_primitive_vertices(prim_idx);
    for (size_t j = 0; j < 3 && j < verts.size(); ++j) {
      faces(prim_idx, j) = verts[j];
    }
  }

  auto mesh = std::make_shared<core::Mesh>(vertices, faces);
  return std::make_shared<GeometryData>(mesh);
}

std::shared_ptr<GeometryData> MirrorSOP::execute() {
  // Sync member variables from parameter system
  plane_ = static_cast<MirrorPlane>(get_parameter<int>("plane", 2));
  keep_original_ = (get_parameter<int>("keep_original", 1) != 0);
  custom_point_ = core::Vector3(
      get_parameter<float>("custom_point_x", 0.0F),
      get_parameter<float>("custom_point_y", 0.0F),
      get_parameter<float>("custom_point_z", 0.0F)
  );
  custom_normal_ = core::Vector3(
      get_parameter<float>("custom_normal_x", 0.0F),
      get_parameter<float>("custom_normal_y", 1.0F),
      get_parameter<float>("custom_normal_z", 0.0F)
  );

  // Get input geometry
  auto input_geo = get_input_data(0);
  if (!input_geo) {
    set_error("No input geometry connected");
    return nullptr;
  }

  // Convert to GeometryContainer
  auto input_container = convert_to_container(*input_geo);
  if (input_container->topology().point_count() == 0) {
    set_error("Input geometry is empty");
    return nullptr;
  }

  // Determine mirror plane
  core::Vector3 plane_point;
  core::Vector3 plane_normal;

  switch (plane_) {
  case MirrorPlane::XY:
    plane_point = core::Vector3::Zero();
    plane_normal = core::Vector3::UnitZ();
    break;
  case MirrorPlane::XZ:
    plane_point = core::Vector3::Zero();
    plane_normal = core::Vector3::UnitY();
    break;
  case MirrorPlane::YZ:
    plane_point = core::Vector3::Zero();
    plane_normal = core::Vector3::UnitX();
    break;
  case MirrorPlane::CUSTOM:
    plane_point = custom_point_;
    plane_normal = custom_normal_.normalized();
    break;
  }

  // Get input positions and topology
  auto *input_positions = input_container->get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (!input_positions) {
    set_error("Input geometry missing position attribute");
    return nullptr;
  }

  const auto &input_topology = input_container->topology();
  auto input_pos_span = input_positions->values();

  // Create output container
  core::GeometryContainer output_container;
  auto &output_topology = output_container.topology();

  // Calculate mirrored positions
  std::vector<core::Vec3f> mirrored_positions;
  mirrored_positions.reserve(input_pos_span.size());

  for (const auto &pos : input_pos_span) {
    core::Vector3 vertex(pos[0], pos[1], pos[2]);
    core::Vector3 mirrored = core::math::mirror_point_across_plane(
        vertex, plane_point, plane_normal);
    mirrored_positions.push_back({static_cast<float>(mirrored.x()),
                                  static_cast<float>(mirrored.y()),
                                  static_cast<float>(mirrored.z())});
  }

  // Mirror normals if present
  std::vector<core::Vec3f> mirrored_normals;
  auto *input_normals = input_container->get_point_attribute_typed<core::Vec3f>(attrs::N);
  if (input_normals) {
    auto input_norm_span = input_normals->values();
    mirrored_normals.reserve(input_norm_span.size());

    for (const auto &norm : input_norm_span) {
      core::Vector3 normal(norm[0], norm[1], norm[2]);
      // Mirror normals (reflection across plane, but don't translate)
      core::Vector3 mirrored_n = normal - 2.0 * normal.dot(plane_normal) * plane_normal;
      mirrored_normals.push_back({static_cast<float>(mirrored_n.x()),
                                  static_cast<float>(mirrored_n.y()),
                                  static_cast<float>(mirrored_n.z())});
    }
  }

  // Build output based on keep_original flag
  if (keep_original_) {
    // Combine original + mirrored
    const size_t total_points = input_pos_span.size() * 2;
    const size_t total_prims = input_topology.primitive_count() * 2;

    output_topology.set_point_count(total_points);

    // Add original primitives
    for (size_t i = 0; i < input_topology.primitive_count(); ++i) {
      output_topology.add_primitive(input_topology.get_primitive_vertices(i));
    }

    // Add mirrored primitives (with flipped winding)
    const int vertex_offset = input_pos_span.size();
    for (size_t i = 0; i < input_topology.primitive_count(); ++i) {
      const auto &orig_verts = input_topology.get_primitive_vertices(i);
      std::vector<int> mirrored_verts(orig_verts.size());
      // Flip winding order for correct normals
      mirrored_verts[0] = orig_verts[0] + vertex_offset;
      mirrored_verts[1] = orig_verts[2] + vertex_offset; // Swap 1 and 2
      mirrored_verts[2] = orig_verts[1] + vertex_offset;
      output_topology.add_primitive(mirrored_verts);
    }

    // Add combined positions
    output_container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
    auto *out_positions = output_container.get_point_attribute_typed<core::Vec3f>(attrs::P);
    if (out_positions) {
      auto out_pos_span = out_positions->values_writable();
      // Copy original
      std::copy(input_pos_span.begin(), input_pos_span.end(), out_pos_span.begin());
      // Copy mirrored
      std::copy(mirrored_positions.begin(), mirrored_positions.end(),
                out_pos_span.begin() + input_pos_span.size());
    }

    // Add combined normals if available
    if (!mirrored_normals.empty()) {
      output_container.add_point_attribute(attrs::N, core::AttributeType::VEC3F);
      auto *out_normals = output_container.get_point_attribute_typed<core::Vec3f>(attrs::N);
      if (out_normals) {
        auto out_norm_span = out_normals->values_writable();
        auto input_norm_span = input_normals->values();
        // Copy original normals
        std::copy(input_norm_span.begin(), input_norm_span.end(), out_norm_span.begin());
        // Copy mirrored normals
        std::copy(mirrored_normals.begin(), mirrored_normals.end(),
                  out_norm_span.begin() + input_norm_span.size());
      }
    }

  } else {
    // Only mirrored version
    output_topology.set_point_count(mirrored_positions.size());

    // Add mirrored primitives with flipped winding
    for (size_t i = 0; i < input_topology.primitive_count(); ++i) {
      const auto &orig_verts = input_topology.get_primitive_vertices(i);
      std::vector<int> mirrored_verts(orig_verts.size());
      // Flip winding order
      mirrored_verts[0] = orig_verts[0];
      mirrored_verts[1] = orig_verts[2]; // Swap 1 and 2
      mirrored_verts[2] = orig_verts[1];
      output_topology.add_primitive(mirrored_verts);
    }

    // Add mirrored positions
    output_container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
    auto *out_positions = output_container.get_point_attribute_typed<core::Vec3f>(attrs::P);
    if (out_positions) {
      auto out_pos_span = out_positions->values_writable();
      std::copy(mirrored_positions.begin(), mirrored_positions.end(), out_pos_span.begin());
    }

    // Add mirrored normals if available
    if (!mirrored_normals.empty()) {
      output_container.add_point_attribute(attrs::N, core::AttributeType::VEC3F);
      auto *out_normals = output_container.get_point_attribute_typed<core::Vec3f>(attrs::N);
      if (out_normals) {
        auto out_norm_span = out_normals->values_writable();
        std::copy(mirrored_normals.begin(), mirrored_normals.end(), out_norm_span.begin());
      }
    }
  }

  // Convert back to GeometryData for compatibility
  return convert_from_container(output_container);
}

std::string MirrorSOP::plane_to_string(MirrorPlane plane) {
  switch (plane) {
  case MirrorPlane::XY:
    return "XY";
  case MirrorPlane::XZ:
    return "XZ";
  case MirrorPlane::YZ:
    return "YZ";
  case MirrorPlane::CUSTOM:
    return "Custom";
  default:
    return "Unknown";
  }
}

std::vector<core::Vector3>
MirrorSOP::mirror_vertices(const std::vector<core::Vector3> &vertices,
                           const core::Vector3 &plane_point,
                           const core::Vector3 &plane_normal) const {

  std::vector<core::Vector3> mirrored_vertices;
  mirrored_vertices.reserve(vertices.size());

  for (const auto &vertex : vertices) {
    // Use the new math utility function
    core::Vector3 mirrored = core::math::mirror_point_across_plane(
        vertex, plane_point, plane_normal);
    mirrored_vertices.push_back(mirrored);
  }

  return mirrored_vertices;
}

} // namespace nodeflux::sop
