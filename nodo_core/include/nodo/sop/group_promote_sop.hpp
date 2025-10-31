#pragma once

#include "sop_node.hpp"
#include <memory>

namespace nodo::sop {

/**
 * @brief Promote groups between element classes
 *
 * Converts groups from one element class to another:
 * - Point to Primitive: primitive is in group if ANY of its points are
 * - Primitive to Point: point is in group if ANY of its primitives are
 *
 * This is useful for converting selection types for different operations.
 */
class GroupPromoteSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit GroupPromoteSOP(const std::string &name)
      : SOPNode(name, "GroupPromote") {
    input_ports_.add_port("0", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);

    // Group name to promote
    register_parameter(define_string_parameter("group_name", "group1")
                           .label("Group Name")
                           .category("Group")
                           .description("Name of the group to promote/convert")
                           .build());

    // Source element class
    register_parameter(define_int_parameter("from_class", 0)
                           .label("From Type")
                           .options({"Points", "Primitives"})
                           .category("Group")
                           .description("Source element type of the group")
                           .build());

    // Target element class
    register_parameter(define_int_parameter("to_class", 1)
                           .label("To Type")
                           .options({"Points", "Primitives"})
                           .category("Group")
                           .description("Target element type for the group")
                           .build());

    // Promotion mode
    register_parameter(
        define_int_parameter("mode", 0)
            .label("Mode")
            .options({"Any", "All"})
            .category("Options")
            .description(
                "Include element if any or all connected elements are in group")
            .build());

    // Delete original group
    register_parameter(define_int_parameter("delete_original", 0)
                           .label("Delete Original")
                           .category("Options")
                           .description("Remove original group after promotion")
                           .build());
  }

  ~GroupPromoteSOP() override = default;

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    auto input = get_input_data(0);
    if (!input) {
      set_error("GroupPromoteSOP requires input geometry");
      return nullptr;
    }

    auto result = std::make_shared<core::GeometryContainer>(input->clone());

    std::string group_name = get_parameter<std::string>("group_name", "group1");
    int from_class = get_parameter<int>("from_class", 0);
    int to_class = get_parameter<int>("to_class", 1);
    int mode = get_parameter<int>("mode", 0); // 0=Any, 1=All
    bool delete_original = get_parameter<int>("delete_original", 0) != 0;

    if (group_name.empty()) {
      set_error("Group name cannot be empty");
      return nullptr;
    }

    // Check if source group exists
    core::ElementClass from_ec = (from_class == 0)
                                     ? core::ElementClass::POINT
                                     : core::ElementClass::PRIMITIVE;
    core::ElementClass to_ec = (to_class == 0) ? core::ElementClass::POINT
                                               : core::ElementClass::PRIMITIVE;

    // Get source group attribute
    auto *src_attr =
        (from_class == 0)
            ? result->get_point_attribute_typed<int>(group_name)
            : result->get_primitive_attribute_typed<int>(group_name);

    if (!src_attr) {
      set_error("Source group '" + group_name + "' does not exist");
      return nullptr;
    }

    // Create destination group
    if (to_class == 0) {
      result->add_point_attribute(group_name, core::AttributeType::INT);
    } else {
      result->add_primitive_attribute(group_name, core::AttributeType::INT);
    }

    // Point to Primitive promotion
    if (from_class == 0 && to_class == 1) {
      auto *dst_attr = result->get_primitive_attribute_typed<int>(group_name);

      for (size_t prim_idx = 0; prim_idx < result->primitive_count();
           ++prim_idx) {
        const auto &prim_verts =
            result->topology().get_primitive_vertices(prim_idx);

        int in_group_count = 0;
        for (int vert_idx : prim_verts) {
          int pt_idx = result->topology().get_vertex_point(vert_idx);
          if (pt_idx >= 0 && static_cast<size_t>(pt_idx) < src_attr->size()) {
            if ((*src_attr)[pt_idx] != 0) {
              in_group_count++;
            }
          }
        }

        // Any mode: at least one point in group
        // All mode: all points in group
        bool promote =
            (mode == 0)
                ? (in_group_count > 0)
                : (in_group_count == static_cast<int>(prim_verts.size()));
        (*dst_attr)[prim_idx] = promote ? 1 : 0;
      }

      if (delete_original) {
        result->remove_point_attribute(group_name);
      }
    }
    // Primitive to Point promotion
    else if (from_class == 1 && to_class == 0) {
      auto *dst_attr = result->get_point_attribute_typed<int>(group_name);

      // Initialize all points to 0
      for (size_t i = 0; i < dst_attr->size(); ++i) {
        (*dst_attr)[i] = 0;
      }

      // Mark points that belong to grouped primitives
      for (size_t prim_idx = 0; prim_idx < result->primitive_count();
           ++prim_idx) {
        if ((*src_attr)[prim_idx] != 0) {
          const auto &prim_verts =
              result->topology().get_primitive_vertices(prim_idx);

          for (int vert_idx : prim_verts) {
            int pt_idx = result->topology().get_vertex_point(vert_idx);
            if (pt_idx >= 0 && static_cast<size_t>(pt_idx) < dst_attr->size()) {
              (*dst_attr)[pt_idx] = 1;
            }
          }
        }
      }

      if (delete_original) {
        result->remove_primitive_attribute(group_name);
      }
    }

    return result;
  }
};

} // namespace nodo::sop
