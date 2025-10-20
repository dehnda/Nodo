#pragma once

#include "../core/geometry_attributes.hpp"
#include "sop_node.hpp"

namespace nodeflux::sop {

/**
 * @brief Copy/instance geometry to point locations
 *
 * This SOP takes template geometry and creates instances at each point
 * in the input point cloud, with support for attribute-driven transformations.
 */
class CopyToPointsSOP : public SOPNode {
private:
  static constexpr bool DEFAULT_USE_POINT_NORMALS = true;
  static constexpr bool DEFAULT_USE_POINT_SCALE = true;
  static constexpr float DEFAULT_UNIFORM_SCALE = 1.0F;

public:
  explicit CopyToPointsSOP(const std::string &node_name = "copy_to_points")
      : SOPNode(node_name, "CopyToPointsSOP") {

    // Add input ports
    input_ports_.add_port("points", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);
    input_ports_.add_port("template", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);

    // Set default parameters
    set_parameter("use_point_normals", DEFAULT_USE_POINT_NORMALS);
    set_parameter("use_point_scale", DEFAULT_USE_POINT_SCALE);
    set_parameter("uniform_scale", DEFAULT_UNIFORM_SCALE);
    set_parameter("scale_attribute", std::string("point_index"));
    set_parameter("rotation_attribute", std::string(""));
  }

  /**
   * @brief Copy template geometry to all point locations
   */
  std::shared_ptr<GeometryData> execute() override {
    // Input 0: Points to copy to
    auto points_data = get_input_data("points");
    if (points_data == nullptr) {
      return nullptr;
    }

    // Input 1: Template geometry to copy
    auto template_data = get_input_data("template");
    if (template_data == nullptr) {
      return nullptr;
    }

    // Get parameters
    bool use_point_normals = get_parameter<bool>("use_point_normals");
    bool use_point_scale = get_parameter<bool>("use_point_scale");
    float uniform_scale = get_parameter<float>("uniform_scale");
    std::string scale_attribute = get_parameter<std::string>("scale_attribute");

    // Create output geometry
    auto output_data = std::make_shared<GeometryData>();

    // Copy template to each point
    copy_template_to_points(*points_data, *template_data, *output_data,
                            use_point_normals, use_point_scale, uniform_scale,
                            scale_attribute);

    return output_data;
  }

  /**
   * @brief Copy template geometry to each point location
   */
  void copy_template_to_points(const GeometryData &points_data,
                               const GeometryData &template_data,
                               GeometryData &output_data,
                               bool use_point_normals, bool use_point_scale,
                               float uniform_scale,
                               const std::string &scale_attribute);

private:
  /**
   * @brief Transform template vertices for a specific point instance
   */
  void transform_template_for_point(const core::Mesh &template_mesh,
                                    const core::Vector3 &point_position,
                                    const core::Vector3 &point_normal,
                                    float point_scale,
                                    core::Mesh::Vertices &output_vertices,
                                    size_t vertex_offset);

  /**
   * @brief Calculate scale factor from point attributes
   */
  float calculate_point_scale(const core::GeometryAttributes &point_attrs,
                              size_t point_index,
                              const std::string &scale_attribute,
                              float uniform_scale, bool use_point_scale);
};

} // namespace nodeflux::sop
