#pragma once

#include "sop_node.hpp"
#include <memory>

namespace nodo::sop {

/**
 * @brief Combine groups using boolean operations
 *
 * Performs set operations on groups:
 * - Union: A | B (elements in either group)
 * - Intersect: A & B (elements in both groups)
 * - Subtract: A - B (elements in A but not B)
 * - Symmetric Difference: A ^ B (elements in A or B but not both)
 *
 * Creates a new group from the combination of two existing groups.
 */
class GroupCombineSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit GroupCombineSOP(const std::string &name)
      : SOPNode(name, "GroupCombine") {
    input_ports_.add_port("0", NodePort::Type::INPUT,
                          NodePort::DataType::GEOMETRY, this);

    // First group
    register_parameter(define_string_parameter("group_a", "group1")
                           .label("Group A")
                           .category("Groups")
                           .description("Name of first group to combine")
                           .build());

    // Second group
    register_parameter(define_string_parameter("group_b", "group2")
                           .label("Group B")
                           .category("Groups")
                           .description("Name of second group to combine")
                           .build());

    // Operation
    register_parameter(
        define_int_parameter("operation", 0)
            .label("Operation")
            .options({"Union (A | B)", "Intersect (A & B)", "Subtract (A - B)",
                      "Symmetric Diff (A ^ B)"})
            .category("Operation")
            .description("Boolean operation to combine groups (union, "
                         "intersect, subtract, or XOR)")
            .build());

    // Output group name
    register_parameter(define_string_parameter("output_group", "combined")
                           .label("Output Group")
                           .category("Output")
                           .description("Name for the resulting combined group")
                           .build());

    // Universal group type parameter (from SOPNode base class)
    add_group_type_parameter("element_class", "Group Type", "Groups");
  }

  ~GroupCombineSOP() override = default;

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    auto input = get_input_data(0);
    if (!input) {
      set_error("GroupCombineSOP requires input geometry");
      return nullptr;
    }

    auto result = std::make_shared<core::GeometryContainer>(input->clone());

    std::string group_a_name = get_parameter<std::string>("group_a", "group1");
    std::string group_b_name = get_parameter<std::string>("group_b", "group2");
    std::string output_name =
        get_parameter<std::string>("output_group", "combined");
    int operation = get_parameter<int>("operation", 0);
    int elem_class = get_parameter<int>("element_class", 0);

    if (group_a_name.empty() || group_b_name.empty() || output_name.empty()) {
      set_error("Group names cannot be empty");
      return nullptr;
    }

    // Get source groups
    const int *group_a = nullptr;
    const int *group_b = nullptr;
    size_t elem_count = 0;

    if (elem_class == 0) { // Points
      auto *attr_a = result->get_point_attribute_typed<int>(group_a_name);
      auto *attr_b = result->get_point_attribute_typed<int>(group_b_name);

      if (!attr_a || !attr_b) {
        set_error("One or both groups do not exist");
        return nullptr;
      }

      group_a = &(*attr_a)[0];
      group_b = &(*attr_b)[0];
      elem_count = result->point_count();

      // Create output group
      result->add_point_attribute(output_name, core::AttributeType::INT);
      auto *output_attr = result->get_point_attribute_typed<int>(output_name);

      // Perform operation
      for (size_t i = 0; i < elem_count; ++i) {
        bool in_a = group_a[i] != 0;
        bool in_b = group_b[i] != 0;
        bool result_val = false;

        switch (operation) {
        case 0: // Union
          result_val = in_a || in_b;
          break;
        case 1: // Intersect
          result_val = in_a && in_b;
          break;
        case 2: // Subtract
          result_val = in_a && !in_b;
          break;
        case 3: // Symmetric Difference
          result_val = in_a != in_b;
          break;
        }

        (*output_attr)[i] = result_val ? 1 : 0;
      }
    } else { // Primitives
      auto *attr_a = result->get_primitive_attribute_typed<int>(group_a_name);
      auto *attr_b = result->get_primitive_attribute_typed<int>(group_b_name);

      if (!attr_a || !attr_b) {
        set_error("One or both groups do not exist");
        return nullptr;
      }

      group_a = &(*attr_a)[0];
      group_b = &(*attr_b)[0];
      elem_count = result->primitive_count();

      // Create output group
      result->add_primitive_attribute(output_name, core::AttributeType::INT);
      auto *output_attr =
          result->get_primitive_attribute_typed<int>(output_name);

      // Perform operation
      for (size_t i = 0; i < elem_count; ++i) {
        bool in_a = group_a[i] != 0;
        bool in_b = group_b[i] != 0;
        bool result_val = false;

        switch (operation) {
        case 0: // Union
          result_val = in_a || in_b;
          break;
        case 1: // Intersect
          result_val = in_a && in_b;
          break;
        case 2: // Subtract
          result_val = in_a && !in_b;
          break;
        case 3: // Symmetric Difference
          result_val = in_a != in_b;
          break;
        }

        (*output_attr)[i] = result_val ? 1 : 0;
      }
    }

    return result;
  }
};

} // namespace nodo::sop
