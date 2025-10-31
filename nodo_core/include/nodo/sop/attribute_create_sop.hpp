#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"

namespace nodo::sop {

/**
 * @brief Attribute Create SOP - Creates new attributes on geometry
 *
 * Adds new attributes to points, primitives, or vertices with specified
 * default values. Useful for:
 * - Adding custom data channels
 * - Initializing physics properties
 * - Setting up material IDs
 * - Creating control attributes for downstream nodes
 */
class AttributeCreateSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit AttributeCreateSOP(const std::string &node_name = "attribcreate")
      : SOPNode(node_name, "AttributeCreate") {
    // Single geometry input
    input_ports_.add_port("0", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);

    // Attribute name
    register_parameter(define_string_parameter("name", "myattrib")
                           .label("Name")
                           .category("Attribute")
                           .description("Name of the attribute to create")
                           .build());

    // Universal class parameter (from SOPNode base class)
    add_class_parameter();

    // Attribute type
    register_parameter(
        define_int_parameter("type", 0)
            .label("Type")
            .options({"Float", "Vector", "Integer"})
            .category("Attribute")
            .description(
                "Data type of the attribute (float, vector, or integer)")
            .build());

    // Default value for Float type
    register_parameter(define_float_parameter("value_float", 0.0F)
                           .label("Value")
                           .range(-100.0, 100.0)
                           .category("Value")
                           .visible_when("type", 0)
                           .description("Default value for float attribute")
                           .build());

    // Default value for Vector type
    register_parameter(define_float_parameter("value_x", 0.0F)
                           .label("Value X")
                           .range(-100.0, 100.0)
                           .category("Value")
                           .visible_when("type", 1)
                           .description("X component of default vector value")
                           .build());

    register_parameter(define_float_parameter("value_y", 0.0F)
                           .label("Value Y")
                           .range(-100.0, 100.0)
                           .category("Value")
                           .visible_when("type", 1)
                           .description("Y component of default vector value")
                           .build());

    register_parameter(define_float_parameter("value_z", 0.0F)
                           .label("Value Z")
                           .range(-100.0, 100.0)
                           .category("Value")
                           .visible_when("type", 1)
                           .description("Z component of default vector value")
                           .build());

    // Default value for Integer type
    register_parameter(define_int_parameter("value_int", 0)
                           .label("Value")
                           .range(-1000, 1000)
                           .category("Value")
                           .visible_when("type", 2)
                           .description("Default value for integer attribute")
                           .build());
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    auto input = get_input_data(0);
    if (!input) {
      set_error("AttributeCreate requires input geometry");
      return nullptr;
    }

    // Clone input
    auto output = std::make_shared<core::GeometryContainer>(input->clone());

    // Get parameters
    const std::string attr_name =
        get_parameter<std::string>("name", "myattrib");
    const int attr_class = get_parameter<int>("class", 0);
    const int attr_type = get_parameter<int>("type", 0);

    if (attr_name.empty()) {
      set_error("Attribute name cannot be empty");
      return output;
    }

    // Add attribute based on class and type
    bool success = false;

    switch (attr_class) {
    case 0: {               // Point
      if (attr_type == 0) { // Float
        success =
            output->add_point_attribute(attr_name, core::AttributeType::FLOAT);
        if (success) {
          const float value = get_parameter<float>("value_float", 0.0F);
          auto *attr = output->get_point_attribute_typed<float>(attr_name);
          if (attr) {
            std::fill(attr->values_writable().begin(),
                      attr->values_writable().end(), value);
          }
        }
      } else if (attr_type == 1) { // Vector
        success =
            output->add_point_attribute(attr_name, core::AttributeType::VEC3F);
        if (success) {
          const float vx = get_parameter<float>("value_x", 0.0F);
          const float vy = get_parameter<float>("value_y", 0.0F);
          const float vz = get_parameter<float>("value_z", 0.0F);
          auto *attr =
              output->get_point_attribute_typed<core::Vec3f>(attr_name);
          if (attr) {
            const core::Vec3f vec(vx, vy, vz);
            std::fill(attr->values_writable().begin(),
                      attr->values_writable().end(), vec);
          }
        }
      } else if (attr_type == 2) { // Integer
        success =
            output->add_point_attribute(attr_name, core::AttributeType::INT);
        if (success) {
          const int value = get_parameter<int>("value_int", 0);
          auto *attr = output->get_point_attribute_typed<int>(attr_name);
          if (attr) {
            std::fill(attr->values_writable().begin(),
                      attr->values_writable().end(), value);
          }
        }
      }
      break;
    }

    case 1: {               // Primitive
      if (attr_type == 0) { // Float
        success = output->add_primitive_attribute(attr_name,
                                                  core::AttributeType::FLOAT);
        if (success) {
          const float value = get_parameter<float>("value_float", 0.0F);
          auto *attr = output->get_primitive_attribute_typed<float>(attr_name);
          if (attr) {
            std::fill(attr->values_writable().begin(),
                      attr->values_writable().end(), value);
          }
        }
      } else if (attr_type == 1) { // Vector
        success = output->add_primitive_attribute(attr_name,
                                                  core::AttributeType::VEC3F);
        if (success) {
          const float vx = get_parameter<float>("value_x", 0.0F);
          const float vy = get_parameter<float>("value_y", 0.0F);
          const float vz = get_parameter<float>("value_z", 0.0F);
          auto *attr =
              output->get_primitive_attribute_typed<core::Vec3f>(attr_name);
          if (attr) {
            const core::Vec3f vec(vx, vy, vz);
            std::fill(attr->values_writable().begin(),
                      attr->values_writable().end(), vec);
          }
        }
      } else if (attr_type == 2) { // Integer
        success = output->add_primitive_attribute(attr_name,
                                                  core::AttributeType::INT);
        if (success) {
          const int value = get_parameter<int>("value_int", 0);
          auto *attr = output->get_primitive_attribute_typed<int>(attr_name);
          if (attr) {
            std::fill(attr->values_writable().begin(),
                      attr->values_writable().end(), value);
          }
        }
      }
      break;
    }

    case 2: {               // Vertex
      if (attr_type == 0) { // Float
        success =
            output->add_vertex_attribute(attr_name, core::AttributeType::FLOAT);
        if (success) {
          const float value = get_parameter<float>("value_float", 0.0F);
          auto *attr = output->get_vertex_attribute_typed<float>(attr_name);
          if (attr) {
            std::fill(attr->values_writable().begin(),
                      attr->values_writable().end(), value);
          }
        }
      } else if (attr_type == 1) { // Vector
        success =
            output->add_vertex_attribute(attr_name, core::AttributeType::VEC3F);
        if (success) {
          const float vx = get_parameter<float>("value_x", 0.0F);
          const float vy = get_parameter<float>("value_y", 0.0F);
          const float vz = get_parameter<float>("value_z", 0.0F);
          auto *attr =
              output->get_vertex_attribute_typed<core::Vec3f>(attr_name);
          if (attr) {
            const core::Vec3f vec(vx, vy, vz);
            std::fill(attr->values_writable().begin(),
                      attr->values_writable().end(), vec);
          }
        }
      } else if (attr_type == 2) { // Integer
        success =
            output->add_vertex_attribute(attr_name, core::AttributeType::INT);
        if (success) {
          const int value = get_parameter<int>("value_int", 0);
          auto *attr = output->get_vertex_attribute_typed<int>(attr_name);
          if (attr) {
            std::fill(attr->values_writable().begin(),
                      attr->values_writable().end(), value);
          }
        }
      }
      break;
    }

    case 3: {               // Detail (global attributes)
      if (attr_type == 0) { // Float
        success =
            output->add_detail_attribute(attr_name, core::AttributeType::FLOAT);
        if (success) {
          const float value = get_parameter<float>("value_float", 0.0F);
          auto *attr = output->get_detail_attribute_typed<float>(attr_name);
          if (attr && attr->size() > 0) {
            (*attr)[0] = value;
          }
        }
      } else if (attr_type == 1) { // Vector
        success =
            output->add_detail_attribute(attr_name, core::AttributeType::VEC3F);
        if (success) {
          const float vx = get_parameter<float>("value_x", 0.0F);
          const float vy = get_parameter<float>("value_y", 0.0F);
          const float vz = get_parameter<float>("value_z", 0.0F);
          auto *attr =
              output->get_detail_attribute_typed<core::Vec3f>(attr_name);
          if (attr && attr->size() > 0) {
            (*attr)[0] = core::Vec3f(vx, vy, vz);
          }
        }
      } else if (attr_type == 2) { // Integer
        success =
            output->add_detail_attribute(attr_name, core::AttributeType::INT);
        if (success) {
          const int value = get_parameter<int>("value_int", 0);
          auto *attr = output->get_detail_attribute_typed<int>(attr_name);
          if (attr && attr->size() > 0) {
            (*attr)[0] = value;
          }
        }
      }
      break;
    }
    }

    if (!success) {
      set_error("Failed to create attribute '" + attr_name + "'");
    }

    return output;
  }
};

} // namespace nodo::sop
