#include "nodo/sop/delete_sop.hpp"
#include "nodo/core/attribute_group.hpp"
#include "nodo/core/geometry_container.hpp"
#include <algorithm>
#include <iostream>
#include <unordered_set>

namespace nodo::sop {

DeleteSOP::DeleteSOP(const std::string &name) : SOPNode(name, "Delete") {
  // Single geometry input
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Element type
  register_parameter(define_int_parameter("entity_type", 0)
                         .label("Entity")
                         .options({"Points", "Primitives"})
                         .category("Delete")
                         .build());

  // Delete mode
  register_parameter(define_int_parameter("delete_mode", 0)
                         .label("Mode")
                         .options({"Selected (in group)",
                                   "Non-selected (not in group)", "Pattern"})
                         .category("Delete")
                         .build());

  // Group name (for group-based deletion) - visible when delete_mode is 0 or 1
  // Note: visible_when doesn't support "OR" conditions, so we'll show for mode
  // 0 Users can still use it for mode 1 (it just won't hide)
  register_parameter(define_string_parameter("group_name", "group1")
                         .label("Group")
                         .category("Group")
                         .build());

  // Pattern selection mode - visible when delete_mode is 2
  register_parameter(define_int_parameter("pattern_type", 0)
                         .label("Pattern Type")
                         .options({"Range", "Every Nth"})
                         .category("Pattern")
                         .visible_when("delete_mode", 2)
                         .build());

  // Range parameters - visible when pattern_type is 0
  register_parameter(define_int_parameter("range_start", 0)
                         .label("Start")
                         .category("Pattern")
                         .visible_when("pattern_type", 0)
                         .build());

  register_parameter(define_int_parameter("range_end", 10)
                         .label("End")
                         .category("Pattern")
                         .visible_when("pattern_type", 0)
                         .build());

  // Every Nth parameters - visible when pattern_type is 1
  register_parameter(define_int_parameter("pattern_step", 2)
                         .label("Step")
                         .category("Pattern")
                         .visible_when("pattern_type", 1)
                         .build());

  register_parameter(define_int_parameter("pattern_offset", 0)
                         .label("Offset")
                         .category("Pattern")
                         .visible_when("pattern_type", 1)
                         .build());

  // Cleanup option
  register_parameter(define_int_parameter("delete_unused_points", 1)
                         .label("Delete Unused Points")
                         .options({"No", "Yes"})
                         .category("Delete")
                         .build());
}

std::shared_ptr<core::GeometryContainer> DeleteSOP::execute() {
  auto input = get_input_data(0);
  if (!input) {
    throw std::runtime_error("DeleteSOP requires input geometry");
  }

  auto result = std::make_shared<core::GeometryContainer>(input->clone());

  int entity_type =
      get_parameter<int>("entity_type", 0); // 0=Points, 1=Primitives
  int delete_mode = get_parameter<int>(
      "delete_mode", 0); // 0=Selected, 1=Non-selected, 2=Pattern
  bool is_primitive = (entity_type == 1);
  auto elem_class =
      is_primitive ? core::ElementClass::PRIMITIVE : core::ElementClass::POINT;

  std::string group_name;
  bool created_temp_group = false;

  // Determine which group to use for deletion
  if (delete_mode == 0 || delete_mode == 1) {
    // Group-based deletion - use existing group
    group_name = get_parameter<std::string>("group_name", "group1");

    std::cerr << "DeleteSOP: Group-based deletion\n";
    std::cerr << "  Entity type: " << (is_primitive ? "Primitives" : "Points")
              << "\n";
    std::cerr << "  Group name: '" << group_name << "'\n";

    if (group_name.empty()) {
      std::cerr << "DeleteSOP: No group name specified, skipping deletion\n";
      return result;
    }

    if (!core::has_group(*result, group_name, elem_class)) {
      std::cerr << "DeleteSOP: Group '" << group_name << "' does not exist for "
                << (is_primitive ? "primitives" : "points") << "\n";
      return result;
    }

    // For "Non-selected" mode, invert the group
    if (delete_mode == 1) {
      std::string temp_group = "__delete_temp_inverse__";
      auto group_elements =
          core::get_group_elements(*result, group_name, elem_class);
      std::unordered_set<size_t> in_group(group_elements.begin(),
                                          group_elements.end());

      size_t elem_count =
          is_primitive ? result->primitive_count() : result->point_count();
      std::vector<size_t> inverse_elements;

      for (size_t i = 0; i < elem_count; ++i) {
        if (in_group.find(i) == in_group.end()) {
          inverse_elements.push_back(i);
        }
      }

      core::create_group(*result, temp_group, elem_class);
      core::add_to_group(*result, temp_group, elem_class, inverse_elements);
      group_name = temp_group;
      created_temp_group = true;
    }
  } else {
    // Pattern-based deletion - create temporary group
    group_name = "__delete_temp_pattern__";
    created_temp_group = true;

    std::vector<size_t> to_delete;
    size_t elem_count =
        is_primitive ? result->primitive_count() : result->point_count();
    int pattern_type = get_parameter<int>("pattern_type", 0);

    if (pattern_type == 0) {
      // Range
      int start = get_parameter<int>("range_start", 0);
      int end = get_parameter<int>("range_end", 10);

      start = std::max(0, std::min(start, static_cast<int>(elem_count)));
      end = std::max(0, std::min(end, static_cast<int>(elem_count)));

      for (int i = start; i < end; ++i) {
        to_delete.push_back(static_cast<size_t>(i));
      }
    } else {
      // Every Nth
      int step = get_parameter<int>("pattern_step", 2);
      int offset = get_parameter<int>("pattern_offset", 0);

      step = std::max(1, step);
      offset = std::max(0, offset);

      for (size_t i = offset; i < elem_count; i += step) {
        to_delete.push_back(i);
      }
    }

    if (to_delete.empty()) {
      std::cerr << "DeleteSOP: No elements to delete\n";
      return result;
    }

    // Create temporary group with pattern selection
    core::create_group(*result, group_name, elem_class);
    core::add_to_group(*result, group_name, elem_class, to_delete);

    std::cerr << "DeleteSOP: Pattern-based deletion - created temp group with "
              << to_delete.size() << " elements\n";
  }

  // Use the delete_elements API
  bool cleanup = (get_parameter<int>("delete_unused_points", 1) == 1);

  auto new_geometry = result->delete_elements(group_name, elem_class, cleanup);

  // Clean up temporary group if we created one
  if (created_temp_group && core::has_group(*result, group_name, elem_class)) {
    core::delete_group(*result, group_name, elem_class);
  }

  if (!new_geometry) {
    std::cerr << "DeleteSOP: Failed to delete elements\n";
    set_error("Failed to delete group '" + group_name + "'");
    return nullptr;
  }

  // Move the result into a shared_ptr
  auto final_result =
      std::make_shared<core::GeometryContainer>(std::move(*new_geometry));

  std::cerr << "DeleteSOP: Result has " << final_result->point_count()
            << " points, " << final_result->primitive_count()
            << " primitives\n";

  return final_result;
}

} // namespace nodo::sop
