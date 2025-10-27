#include "nodeflux/sop/delete_sop.hpp"
#include "nodeflux/core/attribute_group.hpp"
#include "nodeflux/core/geometry_container.hpp"
#include <algorithm>
#include <iostream>
#include <unordered_set>

namespace nodeflux::sop {

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

  // Group name (for group-based deletion)
  register_parameter(define_string_parameter("group_name", "group1")
                         .label("Group")
                         .category("Delete")
                         .build());

  // Pattern selection mode
  register_parameter(define_int_parameter("pattern_type", 0)
                         .label("Pattern")
                         .options({"Range", "Every Nth"})
                         .category("Pattern")
                         .build());

  // Range parameters
  register_parameter(define_int_parameter("range_start", 0)
                         .label("Start")
                         .category("Pattern")
                         .build());

  register_parameter(define_int_parameter("range_end", 10)
                         .label("End")
                         .category("Pattern")
                         .build());

  // Every Nth parameters
  register_parameter(define_int_parameter("pattern_step", 2)
                         .label("Step")
                         .category("Pattern")
                         .build());

  register_parameter(define_int_parameter("pattern_offset", 0)
                         .label("Offset")
                         .category("Pattern")
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
      get_parameter<int>("entity_type", 1); // 0=Points, 1=Primitives
  int delete_mode = get_parameter<int>(
      "delete_mode", 0); // 0=Selected, 1=Non-selected, 2=Pattern
  bool is_primitive = (entity_type == 1);
  auto elem_class =
      is_primitive ? core::ElementClass::PRIMITIVE : core::ElementClass::POINT;

  std::vector<size_t> to_delete;

  // Build deletion list based on mode
  if (delete_mode == 0 || delete_mode == 1) {
    // Group-based deletion
    std::string group_name = get_parameter<std::string>("group_name", "");

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

    auto group_elements =
        core::get_group_elements(*result, group_name, elem_class);

    if (delete_mode == 0) {
      // Delete selected (elements IN group)
      to_delete = group_elements;
    } else {
      // Delete non-selected (elements NOT in group)
      std::unordered_set<size_t> in_group(group_elements.begin(),
                                          group_elements.end());
      size_t elem_count =
          is_primitive ? result->primitive_count() : result->point_count();

      for (size_t i = 0; i < elem_count; ++i) {
        if (in_group.find(i) == in_group.end()) {
          to_delete.push_back(i);
        }
      }
    }
  } else {
    // Pattern-based deletion
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
  }

  if (to_delete.empty()) {
    std::cerr << "DeleteSOP: No elements to delete\n";
    return result;
  }

  // Sort and remove duplicates
  std::sort(to_delete.begin(), to_delete.end());
  to_delete.erase(std::unique(to_delete.begin(), to_delete.end()),
                  to_delete.end());

  std::cerr << "DeleteSOP: Deleting " << to_delete.size() << " "
            << (is_primitive ? "primitives" : "points") << "\n";

  // TODO: Implement actual geometry deletion
  // This requires working with ElementTopology API and attribute copying
  // For now, return input unchanged to demonstrate the selection logic

  std::cerr << "DeleteSOP: WARNING - deletion not yet implemented\n";
  std::cerr << "DeleteSOP: Result has " << result->point_count() << " points, "
            << result->primitive_count() << " primitives\n";

  return result;
}

} // namespace nodeflux::sop
