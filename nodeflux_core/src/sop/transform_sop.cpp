#include "nodeflux/sop/transform_sop.hpp"
#include "nodeflux/core/standard_attributes.hpp"
#include <cmath>
#include <numbers>

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::sop {

TransformSOP::TransformSOP(const std::string &name)
    : SOPNode(name, "Transform") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_float_parameter("translate_x", 0.0F)
                         .label("Translate X")
                         .range(-100.0, 100.0)
                         .category("Translation")
                         .build());

  register_parameter(define_float_parameter("translate_y", 0.0F)
                         .label("Translate Y")
                         .range(-100.0, 100.0)
                         .category("Translation")
                         .build());

  register_parameter(define_float_parameter("translate_z", 0.0F)
                         .label("Translate Z")
                         .range(-100.0, 100.0)
                         .category("Translation")
                         .build());

  register_parameter(define_float_parameter("rotate_x", 0.0F)
                         .label("Rotate X")
                         .range(-360.0, 360.0)
                         .category("Rotation")
                         .build());

  register_parameter(define_float_parameter("rotate_y", 0.0F)
                         .label("Rotate Y")
                         .range(-360.0, 360.0)
                         .category("Rotation")
                         .build());

  register_parameter(define_float_parameter("rotate_z", 0.0F)
                         .label("Rotate Z")
                         .range(-360.0, 360.0)
                         .category("Rotation")
                         .build());

  register_parameter(define_float_parameter("scale_x", 1.0F)
                         .label("Scale X")
                         .range(0.01, 10.0)
                         .category("Scale")
                         .build());

  register_parameter(define_float_parameter("scale_y", 1.0F)
                         .label("Scale Y")
                         .range(0.01, 10.0)
                         .category("Scale")
                         .build());

  register_parameter(define_float_parameter("scale_z", 1.0F)
                         .label("Scale Z")
                         .range(0.01, 10.0)
                         .category("Scale")
                         .build());
}

std::shared_ptr<GeometryData> TransformSOP::execute() {
  // Get input geometry
  auto input_geo = get_input_data(0);
  if (!input_geo) {
    set_error("No input geometry connected");
    return nullptr;
  }

  // Convert input to GeometryContainer
  core::GeometryContainer container;

  // Try to get mesh and convert to container
  auto input_mesh = input_geo->get_mesh();
  if (!input_mesh) {
    set_error("Input geometry does not contain a mesh");
    return nullptr;
  }

  // Simple conversion: copy vertices and faces to container
  const auto &vertices = input_mesh->vertices();
  const auto &faces = input_mesh->faces();

  auto &topology = container.topology();
  topology.set_point_count(vertices.rows());

  // Add primitives and vertices
  for (int face_idx = 0; face_idx < faces.rows(); ++face_idx) {
    std::vector<int> prim_verts;
    for (int j = 0; j < faces.cols(); ++j) {
      prim_verts.push_back(faces(face_idx, j));
    }
    topology.add_primitive(prim_verts);
  }

  // Copy P attribute
  container.add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *p_storage = container.get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (p_storage != nullptr) {
    auto p_span = p_storage->values_writable();
    for (int i = 0; i < vertices.rows(); ++i) {
      p_span[i] = vertices.row(i).cast<float>();
    }
  }

  // Read transform parameters
  const double translate_x = get_parameter<float>("translate_x", 0.0F);
  const double translate_y = get_parameter<float>("translate_y", 0.0F);
  const double translate_z = get_parameter<float>("translate_z", 0.0F);
  const double rotate_x = get_parameter<float>("rotate_x", 0.0F);
  const double rotate_y = get_parameter<float>("rotate_y", 0.0F);
  const double rotate_z = get_parameter<float>("rotate_z", 0.0F);
  const double scale_x = get_parameter<float>("scale_x", 1.0F);
  const double scale_y = get_parameter<float>("scale_y", 1.0F);
  const double scale_z = get_parameter<float>("scale_z", 1.0F);

  // Convert degrees to radians
  constexpr double DEG_TO_RAD = std::numbers::pi_v<double> / 180.0;
  const double rot_x_rad = rotate_x * DEG_TO_RAD;
  const double rot_y_rad = rotate_y * DEG_TO_RAD;
  const double rot_z_rad = rotate_z * DEG_TO_RAD;

  // Create rotation matrices
  Eigen::Matrix3d rot_x;
  rot_x << 1.0, 0.0, 0.0, 0.0, std::cos(rot_x_rad), -std::sin(rot_x_rad), 0.0,
      std::sin(rot_x_rad), std::cos(rot_x_rad);

  Eigen::Matrix3d rot_y;
  rot_y << std::cos(rot_y_rad), 0.0, std::sin(rot_y_rad), 0.0, 1.0, 0.0,
      -std::sin(rot_y_rad), 0.0, std::cos(rot_y_rad);

  Eigen::Matrix3d rot_z;
  rot_z << std::cos(rot_z_rad), -std::sin(rot_z_rad), 0.0, std::sin(rot_z_rad),
      std::cos(rot_z_rad), 0.0, 0.0, 0.0, 1.0;

  // Combined rotation matrix (Z * Y * X order)
  const Eigen::Matrix3d rotation = rot_z * rot_y * rot_x;

  // Scale vector
  const Eigen::Vector3d scale(scale_x, scale_y, scale_z);

  // Translation vector
  const Eigen::Vector3d translation(translate_x, translate_y, translate_z);

  // Transform P attribute
  if (p_storage != nullptr) {
    auto p_span = p_storage->values_writable();
    for (size_t i = 0; i < p_span.size(); ++i) {
      Eigen::Vector3d vertex = p_span[i].cast<double>();
      vertex = vertex.cwiseProduct(scale); // Scale
      vertex = rotation * vertex;          // Rotate
      vertex += translation;               // Translate
      p_span[i] = vertex.cast<float>();
    }
  }

  // Transform N attribute if present (rotation only, no scale or translation)
  if (container.has_point_attribute(attrs::N)) {
    auto *n_storage =
        container.get_point_attribute_typed<core::Vec3f>(attrs::N);
    if (n_storage != nullptr) {
      auto n_span = n_storage->values_writable();
      for (size_t i = 0; i < n_span.size(); ++i) {
        Eigen::Vector3d normal = n_span[i].cast<double>();
        normal = rotation * normal; // Rotate only
        normal.normalize();         // Renormalize
        n_span[i] = normal.cast<float>();
      }
    }
  }

  // Convert back to mesh for output (temporary until full pipeline migrated)
  auto output_mesh = std::make_shared<core::Mesh>();

  // Copy positions back
  Eigen::MatrixXd out_vertices(container.topology().point_count(), 3);
  if (p_storage != nullptr) {
    auto p_span = p_storage->values();
    for (size_t i = 0; i < p_span.size(); ++i) {
      out_vertices.row(i) = p_span[i].cast<double>();
    }
  }
  output_mesh->vertices() = out_vertices;

  // Copy faces back
  Eigen::MatrixXi out_faces(container.topology().primitive_count(), 3);
  for (size_t prim_idx = 0; prim_idx < container.topology().primitive_count();
       ++prim_idx) {
    const auto &verts = container.topology().get_primitive_vertices(prim_idx);
    for (size_t j = 0; j < 3 && j < verts.size(); ++j) {
      out_faces(prim_idx, j) = verts[j];
    }
  }
  output_mesh->faces() = out_faces;

  auto output_geo = std::make_shared<GeometryData>(output_mesh);

  return output_geo;
}

Eigen::Matrix4d TransformSOP::build_transform_matrix() const {
  // Read parameters from parameter system
  const double translate_x = get_parameter<float>("translate_x", 0.0F);
  const double translate_y = get_parameter<float>("translate_y", 0.0F);
  const double translate_z = get_parameter<float>("translate_z", 0.0F);
  const double rotate_x = get_parameter<float>("rotate_x", 0.0F);
  const double rotate_y = get_parameter<float>("rotate_y", 0.0F);
  const double rotate_z = get_parameter<float>("rotate_z", 0.0F);
  const double scale_x = get_parameter<float>("scale_x", 1.0F);
  const double scale_y = get_parameter<float>("scale_y", 1.0F);
  const double scale_z = get_parameter<float>("scale_z", 1.0F);

  // Convert degrees to radians
  constexpr double DEG_TO_RAD = std::numbers::pi_v<double> / 180.0;
  const double rx = rotate_x * DEG_TO_RAD;
  const double ry = rotate_y * DEG_TO_RAD;
  const double rz = rotate_z * DEG_TO_RAD;

  // Build individual transformation matrices
  Eigen::Matrix4d scale_mat = Eigen::Matrix4d::Identity();
  scale_mat(0, 0) = scale_x;
  scale_mat(1, 1) = scale_y;
  scale_mat(2, 2) = scale_z;

  Eigen::Matrix4d rot_x_mat = Eigen::Matrix4d::Identity();
  rot_x_mat(1, 1) = std::cos(rx);
  rot_x_mat(1, 2) = -std::sin(rx);
  rot_x_mat(2, 1) = std::sin(rx);
  rot_x_mat(2, 2) = std::cos(rx);

  Eigen::Matrix4d rot_y_mat = Eigen::Matrix4d::Identity();
  rot_y_mat(0, 0) = std::cos(ry);
  rot_y_mat(0, 2) = std::sin(ry);
  rot_y_mat(2, 0) = -std::sin(ry);
  rot_y_mat(2, 2) = std::cos(ry);

  Eigen::Matrix4d rot_z_mat = Eigen::Matrix4d::Identity();
  rot_z_mat(0, 0) = std::cos(rz);
  rot_z_mat(0, 1) = -std::sin(rz);
  rot_z_mat(1, 0) = std::sin(rz);
  rot_z_mat(1, 1) = std::cos(rz);

  Eigen::Matrix4d translate_mat = Eigen::Matrix4d::Identity();
  translate_mat(0, 3) = translate_x;
  translate_mat(1, 3) = translate_y;
  translate_mat(2, 3) = translate_z;

  // Combine: Translate * Rotate * Scale
  return translate_mat * rot_z_mat * rot_y_mat * rot_x_mat * scale_mat;
}

} // namespace nodeflux::sop
