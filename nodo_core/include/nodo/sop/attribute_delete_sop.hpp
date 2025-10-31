#pragma once

#include "../core/geometry_container.hpp"
#include "sop_node.hpp"
#include <regex>

namespace nodo::sop {

/**
 * @brief Attribute Delete SOP - Removes attributes from geometry
 *
 * Deletes attributes from points, primitives, vertices, or detail.
 * Supports:
 * - Exact name matching
 * - Pattern matching (wildcards)
 * - Multiple attribute deletion
 */
class AttributeDeleteSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit AttributeDeleteSOP(const std::string &node_name = "attribdelete")
      : SOPNode(node_name, "AttributeDelete") {
    // Single geometry input
    input_ports_.add_port("0", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);

    // Attribute pattern (supports wildcards)
    register_parameter(
        define_string_parameter("pattern", "temp_*")
            .label("Pattern")
            .category("Attribute")
            .description("Attribute name pattern (supports * and ? wildcards)")
            .build());

    // Universal class parameter (from SOPNode base class)
    add_class_parameter();

    // Invert pattern (delete everything EXCEPT matching)
    register_parameter(
        define_int_parameter("invert", 0)
            .label("Invert Pattern")
            .options({"No", "Yes"})
            .category("Options")
            .description("Delete all attributes except those matching pattern")
            .build());
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    auto input = get_input_data(0);
    if (!input) {
      set_error("AttributeDelete requires input geometry");
      return nullptr;
    }

    // Clone input
    auto output = std::make_shared<core::GeometryContainer>(input->clone());

    // Get parameters
    const std::string pattern_str = get_parameter<std::string>("pattern", "");
    const int attr_class = get_parameter<int>("class", 0);
    const bool invert = get_parameter<int>("invert", 0) != 0;

    if (pattern_str.empty()) {
      // No pattern, return unchanged
      return output;
    }

    // Convert wildcard pattern to regex
    std::string regex_pattern = pattern_str;
    // Escape regex special characters except * and ?
    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\."), "\\.");
    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\+"), "\\+");
    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\^"), "\\^");
    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\$"), "\\$");
    // Convert wildcards to regex
    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\*"), ".*");
    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\?"), ".");

    std::regex pattern_regex(regex_pattern);

    // Helper function to delete matching attributes
    auto delete_from_class = [&](auto get_names_func, auto remove_func) {
      auto names = get_names_func();
      for (const auto &name : names) {
        bool matches = std::regex_match(name, pattern_regex);
        // Delete if: (matches && !invert) || (!matches && invert)
        if (matches != invert) {
          remove_func(name);
        }
      }
    };

    // Delete based on class
    switch (attr_class) {
    case 0: // Point
      delete_from_class([&]() { return output->get_point_attribute_names(); },
                        [&](const std::string &name) {
                          output->remove_point_attribute(name);
                        });
      break;

    case 1: // Primitive
      delete_from_class(
          [&]() { return output->get_primitive_attribute_names(); },
          [&](const std::string &name) {
            output->remove_primitive_attribute(name);
          });
      break;

    case 2: // Vertex
      delete_from_class([&]() { return output->get_vertex_attribute_names(); },
                        [&](const std::string &name) {
                          output->remove_vertex_attribute(name);
                        });
      break;

    case 3: // Detail
      delete_from_class([&]() { return output->get_detail_attribute_names(); },
                        [&](const std::string &name) {
                          output->remove_detail_attribute(name);
                        });
      break;

    case 4: // All classes
      delete_from_class([&]() { return output->get_point_attribute_names(); },
                        [&](const std::string &name) {
                          output->remove_point_attribute(name);
                        });
      delete_from_class(
          [&]() { return output->get_primitive_attribute_names(); },
          [&](const std::string &name) {
            output->remove_primitive_attribute(name);
          });
      delete_from_class([&]() { return output->get_vertex_attribute_names(); },
                        [&](const std::string &name) {
                          output->remove_vertex_attribute(name);
                        });
      delete_from_class([&]() { return output->get_detail_attribute_names(); },
                        [&](const std::string &name) {
                          output->remove_detail_attribute(name);
                        });
      break;
    }

    return output;
  }
};

} // namespace nodo::sop
