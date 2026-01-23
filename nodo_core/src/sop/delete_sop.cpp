#include "nodo/sop/delete_sop.hpp"

#include "nodo/core/attribute_group.hpp"

#include <unordered_set>

namespace nodo::sop {

// Constants for parameter ranges
constexpr int MAX_RANGE_VALUE = 10000;
constexpr int DEFAULT_RANGE_END = 10;
constexpr int MAX_NTH_STEP = 100;
constexpr int MAX_NTH_OFFSET = 100;

DeleteSOP::DeleteSOP(const std::string& name) : SOPNode(name, "Delete") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

  // Element class parameter (Point or Primitive)
  register_parameter(define_int_parameter("class", 0)
                         .label("Class")
                         .category("Delete")
                         .description("Element type to delete")
                         .options({"Point", "Primitive"})
                         .build());

  // Delete operation mode
  register_parameter(define_int_parameter("operation", 0)
                         .label("Operation")
                         .category("Delete")
                         .description("Deletion mode")
                         .options({"Delete Selected", "Delete Non-Selected", "Delete All"})
                         .build());

  // Group pattern parameter
  register_parameter(define_string_parameter("group", "")
                         .label("Group")
                         .category("Delete")
                         .description("Group name or pattern to delete")
                         .build());

  // Pattern mode (by range or every Nth)
  register_parameter(define_int_parameter("pattern_mode", 0)
                         .label("Pattern Mode")
                         .category("Pattern")
                         .description("Pattern selection mode")
                         .options({"Off", "Range", "Every Nth"})
                         .build());

  // Range start
  register_parameter(define_int_parameter("range_start", 0)
                         .label("Range Start")
                         .category("Pattern")
                         .description("Start index for range pattern")
                         .range(0, MAX_RANGE_VALUE)
                         .visible_when("pattern_mode", 1)
                         .build());

  // Range end
  register_parameter(define_int_parameter("range_end", DEFAULT_RANGE_END)
                         .label("Range End")
                         .category("Pattern")
                         .description("End index for range pattern")
                         .range(0, MAX_RANGE_VALUE)
                         .visible_when("pattern_mode", 1)
                         .build());

  // Every Nth - step size
  register_parameter(define_int_parameter("nth_step", 2)
                         .label("Step")
                         .category("Pattern")
                         .description("Delete every Nth element")
                         .range(1, MAX_NTH_STEP)
                         .visible_when("pattern_mode", 2)
                         .build());

  // Every Nth - offset
  register_parameter(define_int_parameter("nth_offset", 0)
                         .label("Offset")
                         .category("Pattern")
                         .description("Starting offset for Nth pattern")
                         .range(0, MAX_NTH_OFFSET)
                         .visible_when("pattern_mode", 2)
                         .build());

  // Cleanup unused points when deleting primitives
  register_parameter(define_bool_parameter("cleanup", true)
                         .label("Delete Unused Points")
                         .category("Options")
                         .description("Remove points not referenced by any primitive after deletion")
                         .build());
}

core::Result<std::shared_ptr<core::GeometryContainer>> DeleteSOP::execute() {
  // Get input geometry
  auto filter_result = apply_group_filter(0, core::ElementClass::POINT, false);
  if (!filter_result.is_success()) {
    return {"DeleteSOP requires input geometry"};
  }

  const auto& input = filter_result.get_value();
  // Read parameters
  int class_type = get_parameter<int>("class", 0);
  int operation = get_parameter<int>("operation", 0);
  std::string group_name = get_parameter<std::string>("group", "");
  int pattern_mode = get_parameter<int>("pattern_mode", 0);
  bool cleanup = get_parameter<bool>("cleanup", true);

  // Determine element class
  core::ElementClass element_class = (class_type == 0) ? core::ElementClass::POINT : core::ElementClass::PRIMITIVE;

  // Determine total element count
  size_t total_count = (element_class == core::ElementClass::POINT) ? input->point_count() : input->primitive_count();

  // Build set of elements to delete
  std::unordered_set<int> elements_to_delete;

  // Handle operation modes
  if (operation == 2) {
    // Delete All - delete all elements of this class
    for (size_t i = 0; i < total_count; ++i) {
      elements_to_delete.insert(static_cast<int>(i));
    }
  } else {
    // Build selection based on group and/or pattern
    std::unordered_set<int> selection;

    // 1. Apply group selection if group name is provided
    if (!group_name.empty()) {
      if (core::has_group(*input, group_name, element_class)) {
        auto group_elements = core::get_group_elements(*input, group_name, element_class);
        selection.insert(group_elements.begin(), group_elements.end());
      }
    }

    // 2. Apply pattern selection if enabled
    if (pattern_mode == 1) {
      // Range pattern
      int range_start = get_parameter<int>("range_start", 0);
      int range_end = get_parameter<int>("range_end", DEFAULT_RANGE_END);

      // Clamp to valid range
      range_start = std::max(0, std::min(range_start, static_cast<int>(total_count) - 1));
      range_end = std::max(0, std::min(range_end, static_cast<int>(total_count) - 1));

      if (range_start <= range_end) {
        for (int i = range_start; i <= range_end; ++i) {
          selection.insert(i);
        }
      }
    } else if (pattern_mode == 2) {
      // Every Nth pattern
      int nth_step = get_parameter<int>("nth_step", 2);
      int nth_offset = get_parameter<int>("nth_offset", 0);

      if (nth_step > 0) {
        for (size_t i = nth_offset; i < total_count; i += nth_step) {
          selection.insert(static_cast<int>(i));
        }
      }
    }

    // 3. Apply operation mode
    if (operation == 0) {
      // Delete Selected
      elements_to_delete = selection;
    } else if (operation == 1) {
      // Delete Non-Selected - invert selection
      for (size_t i = 0; i < total_count; ++i) {
        if (!selection.contains(static_cast<int>(i))) {
          elements_to_delete.insert(static_cast<int>(i));
        }
      }
    }
  }

  // If nothing to delete, return clone
  if (elements_to_delete.empty()) {
    return std::make_shared<core::GeometryContainer>(input->clone());
  }

  // If deleting all elements, return appropriate empty result
  if (elements_to_delete.size() == total_count) {
    if (element_class == core::ElementClass::POINT) {
      // Delete all points - return empty geometry
      return std::make_shared<core::GeometryContainer>();
    }
    // Delete all primitives - clone with no primitives
    auto result = std::make_shared<core::GeometryContainer>(input->clone());
    result->set_primitive_count(0);
    return result;
  }

  // Create a temporary group with elements to delete
  auto result = std::make_shared<core::GeometryContainer>(input->clone());
  std::string temp_group = "__delete_temp__";

  core::create_group(*result, temp_group, element_class);
  for (int elem_idx : elements_to_delete) {
    core::add_to_group(*result, temp_group, element_class, elem_idx);
  }

  // Use delete_elements which is the public API
  auto result_opt = result->delete_elements(temp_group, element_class, cleanup);

  if (result_opt.is_error()) {
    return {"Failed to delete elements"};
  }

  return result_opt;
}

} // namespace nodo::sop
