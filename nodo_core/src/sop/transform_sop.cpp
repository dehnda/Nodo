#include "nodo/sop/transform_sop.hpp"
#include "nodo/core/standard_attributes.hpp"
#include <cmath>
#include <numbers>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::sop {

TransformSOP::TransformSOP(const std::string &name)
    : SOPNode(name, "Transform") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Use Vector3 parameters for XYZ values (matches HTML design)
  register_parameter(define_vector3_parameter("translate", {0.0F, 0.0F, 0.0F})
                         .label("Translate")
                         .range(-100.0, 100.0)
                         .category("Transform")
                         .description("Translation along X, Y, Z axes")
                         .build());

  register_parameter(define_vector3_parameter("rotate", {0.0F, 0.0F, 0.0F})
                         .label("Rotate")
                         .range(-360.0, 360.0)
                         .category("Transform")
                         .description("Rotation around X, Y, Z axes in degrees")
                         .build());

  register_parameter(define_vector3_parameter("scale", {1.0F, 1.0F, 1.0F})
                         .label("Scale")
                         .range(0.01, 10.0)
                         .category("Transform")
                         .description("Scale factor along X, Y, Z axes")
                         .build());
}

std::shared_ptr<core::GeometryContainer> TransformSOP::execute() {
  // Get input geometry (already a GeometryContainer)
  auto input_container = get_input_data(0);
  if (!input_container) {
    set_error("No input geometry connected");
    return nullptr;
  }

  // Create output container as a deep copy of input
  auto output_container =
      std::make_shared<core::GeometryContainer>(input_container->clone());

  // Get point attribute for positions
  if (!output_container->has_point_attribute("P")) {
    set_error("Input geometry has no position attribute");
    return nullptr;
  }

  auto *p_attr =
      output_container->get_point_attribute_typed<Eigen::Vector3f>("P");
  if (!p_attr) {
    set_error("Position attribute has wrong type");
    return nullptr;
  }

  // Read transform parameters (as Vector3)
  const auto translate = get_parameter<Eigen::Vector3f>(
      "translate", Eigen::Vector3f(0.0F, 0.0F, 0.0F));
  const auto rotate = get_parameter<Eigen::Vector3f>(
      "rotate", Eigen::Vector3f(0.0F, 0.0F, 0.0F));
  const auto scale_vec = get_parameter<Eigen::Vector3f>(
      "scale", Eigen::Vector3f(1.0F, 1.0F, 1.0F));

  const double translate_x = translate.x();
  const double translate_y = translate.y();
  const double translate_z = translate.z();
  const double rotate_x = rotate.x();
  const double rotate_y = rotate.y();
  const double rotate_z = rotate.z();
  const double scale_x = scale_vec.x();
  const double scale_y = scale_vec.y();
  const double scale_z = scale_vec.z();

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
  auto positions_span = p_attr->values_writable();
  for (size_t i = 0; i < positions_span.size(); ++i) {
    // Skip points not in group filter
    if (!is_in_group(output_container, 0, i))
      continue;

    Eigen::Vector3d vertex = positions_span[i].cast<double>();
    vertex = vertex.cwiseProduct(scale); // Scale
    vertex = rotation * vertex;          // Rotate
    vertex += translation;               // Translate
    positions_span[i] = vertex.cast<float>();
  }

  // Transform N attribute if present (rotation only, no scale or translation)
  if (output_container->has_point_attribute("N")) {
    auto *n_attr =
        output_container->get_point_attribute_typed<Eigen::Vector3f>("N");
    if (n_attr) {
      auto normals_span = n_attr->values_writable();
      for (size_t i = 0; i < normals_span.size(); ++i) {
        // Skip normals for points not in group filter
        if (!is_in_group(output_container, 0, i))
          continue;

        Eigen::Vector3d normal = normals_span[i].cast<double>();
        normal = rotation * normal; // Rotate only
        normal.normalize();         // Renormalize
        normals_span[i] = normal.cast<float>();
      }
    }
  }

  return output_container;
}

Eigen::Matrix4d TransformSOP::build_transform_matrix() const {
  // Read parameters from parameter system (as Vector3)
  const auto translate = get_parameter<Eigen::Vector3f>(
      "translate", Eigen::Vector3f(0.0F, 0.0F, 0.0F));
  const auto rotate = get_parameter<Eigen::Vector3f>(
      "rotate", Eigen::Vector3f(0.0F, 0.0F, 0.0F));
  const auto scale_vec = get_parameter<Eigen::Vector3f>(
      "scale", Eigen::Vector3f(1.0F, 1.0F, 1.0F));

  const double translate_x = translate.x();
  const double translate_y = translate.y();
  const double translate_z = translate.z();
  const double rotate_x = rotate.x();
  const double rotate_y = rotate.y();
  const double rotate_z = rotate.z();
  const double scale_x = scale_vec.x();
  const double scale_y = scale_vec.y();
  const double scale_z = scale_vec.z();

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

} // namespace nodo::sop
