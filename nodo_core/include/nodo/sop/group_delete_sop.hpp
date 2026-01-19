#pragma once

#include "sop_node.hpp"

#include <memory>
#include <regex>

namespace nodo::sop {

/**
 * @brief Delete groups from geometry
 *
 * Removes one or more group attributes from the geometry. Groups are stored
 * as integer attributes, and this node deletes those attributes.
 * Supports wildcard patterns for deleting multiple groups at once.
 */
class GroupDeleteSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit GroupDeleteSOP(const std::string& name = "group_delete") : SOPNode(name, "GroupDelete") {
    input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

    // Group name pattern (supports wildcards)
    register_parameter(define_string_parameter("pattern", "*")
                           .label("Pattern")
                           .category("Group")
                           .description("Group name pattern to delete (supports * and ? wildcards)")
                           .build());

    // Custom group type parameter (GroupDeleteSOP needs Edges and All options)
    register_parameter(define_int_parameter("element_class", 0)
                           .label("Group Type")
                           .options({"Points", "Primitives", "Edges", "All"})
                           .category("Group")
                           .description("Type of groups to delete (point, primitive, edge, or all)")
                           .build());
  }

  ~GroupDeleteSOP() override = default;

protected:
  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override {
    auto input = get_input_data(0);
    if (!input) {
      return {"GroupDeleteSOP requires input geometry"};
    }

    auto result = std::make_shared<core::GeometryContainer>(input->clone());

    std::string pattern = get_parameter<std::string>("pattern", "*");
    int elem_class = get_parameter<int>("element_class", 0);

    // Convert wildcard pattern to regex
    std::string regex_pattern = pattern;
    // Escape special regex chars except * and ?
    std::string escaped;
    for (char c : regex_pattern) {
      if (c == '*') {
        escaped += ".*";
      } else if (c == '?') {
        escaped += ".";
      } else if (c == '.' || c == '[' || c == ']' || c == '(' || c == ')' || c == '+' || c == '^' || c == '$' ||
                 c == '\\') {
        escaped += '\\';
        escaped += c;
      } else {
        escaped += c;
      }
    }

    std::regex pattern_regex(escaped);

    // Get all group names (groups are stored as attributes)
    auto delete_groups_from_class = [&](core::ElementClass ec) {
      std::vector<std::string> groups_to_delete;

      // Get attribute names based on element class
      if (ec == core::ElementClass::POINT) {
        auto attr_names = result->get_point_attribute_names();
        for (const auto& name : attr_names) {
          if (std::regex_match(name, pattern_regex)) {
            // Check if it's actually a group (int attribute)
            auto attr_typed = result->get_point_attribute_typed<int>(name);
            if (attr_typed) {
              groups_to_delete.push_back(name);
            }
          }
        }
      } else if (ec == core::ElementClass::PRIMITIVE) {
        auto attr_names = result->get_primitive_attribute_names();
        for (const auto& name : attr_names) {
          if (std::regex_match(name, pattern_regex)) {
            auto attr_typed = result->get_primitive_attribute_typed<int>(name);
            if (attr_typed) {
              groups_to_delete.push_back(name);
            }
          }
        }
      }

      // Delete matched groups
      for (const auto& group_name : groups_to_delete) {
        if (ec == core::ElementClass::POINT) {
          result->remove_point_attribute(group_name);
        } else if (ec == core::ElementClass::PRIMITIVE) {
          result->remove_primitive_attribute(group_name);
        }
      }
    };

    // Delete from selected element class(es)
    if (elem_class == 3) { // All
      delete_groups_from_class(core::ElementClass::POINT);
      delete_groups_from_class(core::ElementClass::PRIMITIVE);
    } else if (elem_class == 0) {
      delete_groups_from_class(core::ElementClass::POINT);
    } else if (elem_class == 1) {
      delete_groups_from_class(core::ElementClass::PRIMITIVE);
    }
    // elem_class == 2 (Edges) not yet supported

    return result;
  }
};

} // namespace nodo::sop
