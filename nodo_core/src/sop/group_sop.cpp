#include "nodo/sop/group_sop.hpp"

#include "nodo/core/attribute_group.hpp"

#include <iostream>

namespace nodo::sop {

GroupSOP::GroupSOP(const std::string& name) : SOPNode(name, "Group") {
  // Single input (using standard "0" port name)
  input_ports_.add_port("0", NodePort::Type::INPUT, NodePort::DataType::GEOMETRY, this);

  // Group name parameter
  register_parameter(define_string_parameter("group_name", "group1")
                         .label("Group Name")
                         .category("Group")
                         .description("Name of the group to create or modify")
                         .build());

  // Universal group type parameter (from SOPNode base class)
  add_group_type_parameter();

  // Operation mode
  register_parameter(define_int_parameter("operation", 0)
                         .label("Operation")
                         .options({"Create/Replace", "Add to Existing", "Remove from Existing"})
                         .category("Group")
                         .description("How to modify existing group (create, add, or remove)")
                         .build());

  // Selection method - dropdown
  register_parameter(define_int_parameter("selection_mode", 0)
                         .label("Selection Mode")
                         .options({"Range", "Every Nth", "Random", "All"})
                         .category("Selection")
                         .description("Method for selecting elements (range, pattern, random, or all)")
                         .build());

  // Range parameters
  register_parameter(define_int_parameter("range_start", 0)
                         .label("Start")
                         .range(0, 10000)
                         .category("Range")
                         .description("First element index in range selection")
                         .build());

  register_parameter(define_int_parameter("range_end", 10)
                         .label("End")
                         .range(0, 10000)
                         .category("Range")
                         .description("Last element index in range selection")
                         .build());

  // Pattern parameters (Every Nth)
  register_parameter(define_int_parameter("pattern_step", 2)
                         .label("Step")
                         .range(1, 100)
                         .category("Pattern")
                         .description("Select every Nth element (e.g., 2 = every other)")
                         .build());

  register_parameter(define_int_parameter("pattern_offset", 0)
                         .label("Offset")
                         .range(0, 100)
                         .category("Pattern")
                         .description("Starting offset for pattern selection")
                         .build());

  // Random parameters
  register_parameter(define_int_parameter("random_count", 10)
                         .label("Count")
                         .range(1, 10000)
                         .category("Random")
                         .description("Number of random elements to select")
                         .build());

  register_parameter(define_int_parameter("random_seed", 0)
                         .label("Seed")
                         .range(0, 10000)
                         .category("Random")
                         .description("Random seed for reproducible selection")
                         .build());
}

core::Result<std::shared_ptr<core::GeometryContainer>> GroupSOP::execute() {
  std::cerr << "GroupSOP::execute() called\n";

  // Get input
  auto input = get_input_data(0);
  if (input == nullptr) {
    std::cerr << "  ERROR: No input geometry\n";
    set_error("GroupSOP requires input geometry - connect a node to input 0");
    return {(std::string) "No input geometry"};
  }

  std::cerr << "  Input has " << input->point_count() << " points, " << input->primitive_count() << " primitives\n";
  std::cerr << "  Input point_attributes().size() = " << input->point_attributes().size() << "\n";
  std::cerr << "  Input point_attributes().attribute_count() = " << input->point_attributes().attribute_count() << "\n";

  // Get writable handle (COW)
  auto handle = get_input_handle(0);
  auto& result = handle.write();

  std::cerr << "  Result has " << result.point_count() << " points, " << result.primitive_count() << " primitives\n";
  std::cerr << "  Point attributes size: " << result.point_attributes().size() << "\n";

  // Get parameters
  std::string group_name = get_parameter<std::string>("group_name", "group1");
  std::cerr << "  Group name: " << group_name << "\n";

  if (group_name.empty()) {
    std::cerr << "  ERROR: Empty group name\n";
    set_error("Group name cannot be empty");
    return {(std::string) "Group name cannot be empty"};
  }

  int elem_class_int = get_parameter<int>("element_class", 0);
  core::ElementClass elem_class = (elem_class_int == 0) ? core::ElementClass::POINT : core::ElementClass::PRIMITIVE;

  std::cerr << "  Element class: " << (elem_class_int == 0 ? "Points" : "Primitives") << "\n";

  int selection_mode = get_parameter<int>("selection_mode", 0);
  int operation = get_parameter<int>("operation", 0);

  std::cerr << "  Selection mode: " << selection_mode << ", Operation: " << operation << "\n";

  // Get element count
  size_t elem_count = (elem_class == core::ElementClass::POINT) ? result.point_count() : result.primitive_count();

  // Create group if it doesn't exist (or if operation is Create/Replace)
  bool group_exists = core::has_group(result, group_name, elem_class);

  if (operation == 0) { // Create/Replace
    if (group_exists) {
      core::clear_group(result, group_name, elem_class);
    } else {
      core::create_group(result, group_name, elem_class);
    }
  } else if (!group_exists) {
    // For Add/Remove operations, create group if it doesn't exist
    core::create_group(result, group_name, elem_class);
  }

  // Build selection based on mode
  std::vector<size_t> selection;

  switch (selection_mode) {
    case 0: { // Range
      int start = get_parameter<int>("range_start", 0);
      int end = get_parameter<int>("range_end", 10);

      // Clamp to valid range
      start = std::max(0, std::min(start, static_cast<int>(elem_count)));
      end = std::max(0, std::min(end, static_cast<int>(elem_count)));

      if (start < end) {
        for (int i = start; i < end; ++i) {
          selection.push_back(static_cast<size_t>(i));
        }
      }
      break;
    }

    case 1: { // Every Nth (Pattern)
      int step = get_parameter<int>("pattern_step", 2);
      int offset = get_parameter<int>("pattern_offset", 0);

      step = std::max(1, step);
      offset = std::max(0, offset);

      for (size_t i = offset; i < elem_count; i += step) {
        selection.push_back(i);
      }
      break;
    }

    case 2: { // Random
      int count = get_parameter<int>("random_count", 10);
      int seed = get_parameter<int>("random_seed", 0);

      count = std::max(0, std::min(count, static_cast<int>(elem_count)));

      // Use the existing random selection function
      // Note: We need to create a temp group, get elements, then apply
      // operation
      std::string temp_group = "__temp_random__";
      core::create_group(result, temp_group, elem_class);
      core::select_random(result, temp_group, elem_class, count, seed);
      selection = core::get_group_elements(result, temp_group, elem_class);
      core::delete_group(result, temp_group, elem_class);
      break;
    }

    case 3: { // All
      for (size_t i = 0; i < elem_count; ++i) {
        selection.push_back(i);
      }
      break;
    }
  }

  // Apply operation
  std::cerr << "  Applying operation with " << selection.size() << " elements selected\n";

  if (operation == 0 || operation == 1) {
    // Create/Replace or Add
    bool success = core::add_to_group(result, group_name, elem_class, selection);
    std::cerr << "  add_to_group returned: " << (success ? "true" : "false") << "\n";
  } else if (operation == 2) {
    // Remove
    core::remove_from_group(result, group_name, elem_class, selection);
  }

  // Verify group contents
  auto group_elements = core::get_group_elements(result, group_name, elem_class);
  std::cerr << "  Group '" << group_name << "' now contains " << group_elements.size() << " elements\n";

  std::cerr << "  Group created successfully, returning result\n";
  std::cerr << "  Result has " << result.point_count() << " points, " << result.primitive_count() << " primitives\n";

  return std::make_shared<core::GeometryContainer>(std::move(result));
}

} // namespace nodo::sop
