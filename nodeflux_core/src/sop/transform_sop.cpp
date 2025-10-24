#include "nodeflux/sop/transform_sop.hpp"
#include <cmath>
#include <numbers>

namespace nodeflux::sop {

TransformSOP::TransformSOP(const std::string &name)
    : SOPNode(name, "Transform") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);
}

std::shared_ptr<GeometryData> TransformSOP::execute() {
  // Get input geometry
  auto input_geo = get_input_data(0);
  if (!input_geo) {
    set_error("No input geometry connected");
    return nullptr;
  }

  auto input_mesh = input_geo->get_mesh();
  if (!input_mesh) {
    set_error("Input geometry does not contain a mesh");
    return nullptr;
  }

  // Create output geometry with transformed mesh
  auto output_geo = std::make_shared<GeometryData>(*input_geo);
  auto transformed_mesh = std::make_shared<core::Mesh>(*input_mesh);

  // Build transformation matrix (Scale -> Rotate -> Translate)
  auto &vertices = transformed_mesh->vertices();

  // Convert degrees to radians
  constexpr double DEG_TO_RAD = std::numbers::pi_v<double> / 180.0;
  const double rx = rotate_x_ * DEG_TO_RAD;
  const double ry = rotate_y_ * DEG_TO_RAD;
  const double rz = rotate_z_ * DEG_TO_RAD;

  // Create rotation matrices
  Eigen::Matrix3d rot_x;
  rot_x << 1.0, 0.0, 0.0,
           0.0, std::cos(rx), -std::sin(rx),
           0.0, std::sin(rx), std::cos(rx);

  Eigen::Matrix3d rot_y;
  rot_y << std::cos(ry), 0.0, std::sin(ry),
           0.0, 1.0, 0.0,
           -std::sin(ry), 0.0, std::cos(ry);

  Eigen::Matrix3d rot_z;
  rot_z << std::cos(rz), -std::sin(rz), 0.0,
           std::sin(rz), std::cos(rz), 0.0,
           0.0, 0.0, 1.0;

  // Combined rotation matrix (Z * Y * X order)
  const Eigen::Matrix3d rotation = rot_z * rot_y * rot_x;

  // Scale vector
  const Eigen::Vector3d scale(scale_x_, scale_y_, scale_z_);

  // Translation vector
  const Eigen::Vector3d translation(translate_x_, translate_y_, translate_z_);

  // Apply transformations to vertices (Scale -> Rotate -> Translate)
  for (int i = 0; i < vertices.rows(); ++i) {
    Eigen::Vector3d vertex = vertices.row(i).transpose();
    vertex = vertex.cwiseProduct(scale); // Scale
    vertex = rotation * vertex;          // Rotate
    vertex += translation;               // Translate
    vertices.row(i) = vertex.transpose();
  }

  // Update mesh in output geometry
  output_geo->set_mesh(transformed_mesh);

  return output_geo;
}

Eigen::Matrix4d TransformSOP::build_transform_matrix() const {
  // Convert degrees to radians
  constexpr double DEG_TO_RAD = std::numbers::pi_v<double> / 180.0;
  const double rx = rotate_x_ * DEG_TO_RAD;
  const double ry = rotate_y_ * DEG_TO_RAD;
  const double rz = rotate_z_ * DEG_TO_RAD;

  // Build individual transformation matrices
  Eigen::Matrix4d scale_mat = Eigen::Matrix4d::Identity();
  scale_mat(0, 0) = scale_x_;
  scale_mat(1, 1) = scale_y_;
  scale_mat(2, 2) = scale_z_;

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
  translate_mat(0, 3) = translate_x_;
  translate_mat(1, 3) = translate_y_;
  translate_mat(2, 3) = translate_z_;

  // Combine: Translate * Rotate * Scale
  return translate_mat * rot_z_mat * rot_y_mat * rot_x_mat * scale_mat;
}

} // namespace nodeflux::sop
