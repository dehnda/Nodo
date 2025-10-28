#include "nodo/sop/copy_to_points_sop.hpp"

namespace attrs = nodo::core::standard_attrs;

namespace nodo::sop {

// Constants for copy operations
constexpr float DEFAULT_SCALE_MULTIPLIER = 0.1F;
constexpr double UP_VECTOR_Y = 1.0;

CopyToPointsSOP::CopyToPointsSOP(const std::string &node_name)
    : SOPNode(node_name, "CopyToPoints") {
  // Add input ports (use "0" and "1" to match execution engine's numeric
  // indexing)
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);
  input_ports_.add_port("1", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_int_parameter("use_point_normals", 1)
                         .label("Use Point Normals")
                         .range(0, 1)
                         .category("Copy")
                         .build());

  register_parameter(define_int_parameter("use_point_scale", 1)
                         .label("Use Point Scale")
                         .range(0, 1)
                         .category("Copy")
                         .build());

  register_parameter(define_float_parameter("uniform_scale", 1.0F)
                         .label("Uniform Scale")
                         .range(0.01, 10.0)
                         .category("Copy")
                         .build());

  register_parameter(define_string_parameter("scale_attribute", "point_index")
                         .label("Scale Attribute")
                         .category("Copy")
                         .build());

  register_parameter(define_string_parameter("rotation_attribute", "")
                         .label("Rotation Attribute")
                         .category("Copy")
                         .build());
}

std::shared_ptr<core::GeometryContainer> CopyToPointsSOP::execute() {
  // Get inputs (by index: 0 = points, 1 = template)
  auto points_input = get_input_data(0);
  auto template_input = get_input_data(1);

  if (!points_input) {
    set_error("Missing 'points' input (port 0)");
    return nullptr;
  }

  if (!template_input) {
    set_error("Missing 'template' input (port 1)");
    return nullptr;
  }

  // Get parameters
  const bool use_point_normals =
      get_parameter<int>("use_point_normals", 1) != 0;
  const bool use_point_scale = get_parameter<int>("use_point_scale", 1) != 0;
  const float uniform_scale = get_parameter<float>("uniform_scale", 1.0F);
  const std::string scale_attr =
      get_parameter<std::string>("scale_attribute", "point_index");

  // Get point positions from points input
  auto *points_P =
      points_input->get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (points_P == nullptr) {
    set_error("Points input has no position attribute");
    return nullptr;
  }

  // Get optional point normals for rotation
  auto *points_N =
      points_input->get_point_attribute_typed<core::Vec3f>(attrs::N);

  // Get template positions
  auto *template_P =
      template_input->get_point_attribute_typed<core::Vec3f>(attrs::P);
  if (template_P == nullptr) {
    set_error("Template input has no position attribute");
    return nullptr;
  }

  const size_t num_points = points_input->topology().point_count();
  const size_t template_point_count = template_input->topology().point_count();
  const size_t template_prim_count =
      template_input->topology().primitive_count();

  if (num_points == 0 || template_point_count == 0) {
    return std::make_shared<core::GeometryContainer>();
  }

  // Create output container
  auto result = std::make_shared<core::GeometryContainer>();

  const size_t total_points = num_points * template_point_count;
  result->set_point_count(total_points);

  // Add position attribute
  result->add_point_attribute(attrs::P, core::AttributeType::VEC3F);
  auto *result_P = result->get_point_attribute_typed<core::Vec3f>(attrs::P);

  // Copy other point attributes from template if they exist
  if (template_input->has_point_attribute(attrs::N)) {
    result->add_point_attribute(attrs::N, core::AttributeType::VEC3F);
  }
  if (template_input->has_point_attribute(attrs::Cd)) {
    result->add_point_attribute(attrs::Cd, core::AttributeType::VEC3F);
  }

  auto *result_N = result->get_point_attribute_typed<core::Vec3f>(attrs::N);
  auto *result_Cd = result->get_point_attribute_typed<core::Vec3f>(attrs::Cd);
  auto *template_N =
      template_input->get_point_attribute_typed<core::Vec3f>(attrs::N);
  auto *template_Cd =
      template_input->get_point_attribute_typed<core::Vec3f>(attrs::Cd);

  // For each point in the points input
  for (size_t point_idx = 0; point_idx < num_points; ++point_idx) {
    const core::Vec3f copy_position = (*points_P)[point_idx];

    // Calculate scale for this instance
    float instance_scale = uniform_scale;
    if (use_point_scale) {
      // Try to get scale from point attribute
      auto *pscale_attr =
          points_input->get_point_attribute_typed<float>(attrs::pscale);
      if (pscale_attr != nullptr) {
        instance_scale *= (*pscale_attr)[point_idx];
      }
    }

    // Calculate rotation matrix if using normals
    Eigen::Matrix3f rotation = Eigen::Matrix3f::Identity();
    if (use_point_normals && (points_N != nullptr)) {
      core::Vec3f normal = (*points_N)[point_idx];
      constexpr float MIN_NORMAL_LENGTH = 1e-6F;
      if (normal.squaredNorm() > MIN_NORMAL_LENGTH) {
        normal.normalize();

        // Build rotation from +Z to normal direction
        const core::Vec3f ref_up(0.0F, 1.0F, 0.0F);
        const core::Vec3f z_axis = normal;

        // Create orthogonal basis
        core::Vec3f x_axis = ref_up.cross(z_axis);
        if (x_axis.squaredNorm() < MIN_NORMAL_LENGTH) {
          // Normal is parallel to up, use different reference
          x_axis = core::Vec3f(1.0F, 0.0F, 0.0F).cross(z_axis);
        }
        x_axis.normalize();

        const core::Vec3f y_axis = z_axis.cross(x_axis).normalized();

        rotation.col(0) = x_axis;
        rotation.col(1) = y_axis;
        rotation.col(2) = z_axis;
      }
    }

    // Copy template points with transformation
    const size_t output_offset = point_idx * template_point_count;
    for (size_t tpl_pt = 0; tpl_pt < template_point_count; ++tpl_pt) {
      const size_t output_idx = output_offset + tpl_pt;

      // Transform template point: rotate, scale, translate
      core::Vec3f pos = (*template_P)[tpl_pt];
      pos = rotation * (pos * instance_scale) + copy_position;
      (*result_P)[output_idx] = pos;

      // Copy normals with rotation
      if ((result_N != nullptr) && (template_N != nullptr)) {
        core::Vec3f normal = (*template_N)[tpl_pt];
        if (use_point_normals) {
          normal = rotation * normal;
        }
        (*result_N)[output_idx] = normal.normalized();
      }

      // Copy color
      if ((result_Cd != nullptr) && (template_Cd != nullptr)) {
        (*result_Cd)[output_idx] = (*template_Cd)[tpl_pt];
      }
    }
  }

  // Copy topology for each instance
  // Calculate total vertex count needed
  size_t total_vertices = 0;
  for (size_t prim_idx = 0; prim_idx < template_prim_count; ++prim_idx) {
    total_vertices +=
        template_input->topology().get_primitive_vertices(prim_idx).size();
  }
  total_vertices *= num_points;

  result->set_vertex_count(total_vertices);

  size_t vertex_counter = 0;
  for (size_t point_idx = 0; point_idx < num_points; ++point_idx) {
    const int point_offset = static_cast<int>(point_idx * template_point_count);

    for (size_t prim_idx = 0; prim_idx < template_prim_count; ++prim_idx) {
      const auto &template_verts =
          template_input->topology().get_primitive_vertices(prim_idx);
      std::vector<int> new_prim_verts;
      new_prim_verts.reserve(template_verts.size());

      for (int vert_idx : template_verts) {
        const int template_point =
            template_input->topology().get_vertex_point(vert_idx);
        const int new_point = template_point + point_offset;

        result->topology().set_vertex_point(vertex_counter, new_point);
        new_prim_verts.push_back(static_cast<int>(vertex_counter));
        ++vertex_counter;
      }

      result->add_primitive(new_prim_verts);
    }
  }

  return result;
}

} // namespace nodo::sop
