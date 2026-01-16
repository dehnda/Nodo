#include "nodo/sop/blast_sop.hpp"

#include "nodo/core/attribute_group.hpp"

#include <iostream>
#include <unordered_set>

namespace nodo::sop {

std::shared_ptr<core::GeometryContainer> BlastSOP::execute() {
  auto input = get_input_data(0);
  if (!input) {
    set_error("BlastSOP requires input geometry");
    return nullptr;
  }

  std::cerr << "\n=== BlastSOP::execute() ===\n";
  std::cerr << "Input: " << input->point_count() << " points, " << input->primitive_count() << " primitives\n";

  // Get parameters - use universal input_group parameter
  std::string group_name = get_parameter<std::string>("input_group", "");
  int class_type = get_parameter<int>("class", 0);
  bool negate = get_parameter<int>("negate", 0) != 0;

  std::cerr << "Group: '" << group_name << "'\n";
  std::cerr << "Class: " << class_type << " (" << (class_type == 0 ? "Points" : "Primitives") << ")\n";
  std::cerr << "Negate (delete non-selected): " << (negate ? "YES" : "NO") << "\n";

  // Determine element class
  core::ElementClass element_class = (class_type == 0) ? core::ElementClass::POINT : core::ElementClass::PRIMITIVE;

  // Handle empty group name
  if (group_name.empty()) {
    if (negate) {
      // Empty group + negate = keep everything (delete nothing)
      return std::make_shared<core::GeometryContainer>(input->clone());
    }
    // Empty group + no negate = delete all elements of that class
    if (element_class == core::ElementClass::POINT) {
      // Delete all points - return empty geometry
      return std::make_shared<core::GeometryContainer>();
    }
    // Delete all primitives - clone with no primitives
    auto result = std::make_shared<core::GeometryContainer>(input->clone());
    result->set_primitive_count(0);
    return result;
  }

  // Check if group exists
  bool group_exists = core::has_group(*input, group_name, element_class);

  if (!group_exists) {
    if (negate) {
      // Group doesn't exist + negate = delete everything
      if (element_class == core::ElementClass::POINT) {
        return std::make_shared<core::GeometryContainer>();
      }
      auto result = std::make_shared<core::GeometryContainer>(input->clone());
      result->set_primitive_count(0);
      return result;
    }
    // Group doesn't exist + no negate = nothing to delete, keep all
    return std::make_shared<core::GeometryContainer>(input->clone());
  }

  // If not negating, we can use delete_elements directly
  if (!negate) {
    auto result_opt = input->delete_elements(group_name, element_class, true);
    if (!result_opt.has_value()) {
      set_error("Failed to delete elements from group");
      return std::make_shared<core::GeometryContainer>(input->clone());
    }
    return std::make_shared<core::GeometryContainer>(std::move(result_opt.value()));
  }

  // Negate case: delete elements NOT in group
  // We need to create a temporary group with inverted elements
  std::cerr << "NEGATE case: creating inverted group\n";

  auto result = std::make_shared<core::GeometryContainer>(input->clone());

  // Get the group elements
  auto group_elements = core::get_group_elements(*result, group_name, element_class);

  std::cerr << "Original group '" << group_name << "' contains " << group_elements.size() << " elements: ";
  for (size_t elem : group_elements) {
    std::cerr << elem << " ";
  }
  std::cerr << "\n";

  std::unordered_set<int> group_set(group_elements.begin(), group_elements.end());

  // Create inverted group
  std::string temp_group = "__blast_temp_" + group_name;
  core::create_group(*result, temp_group, element_class);

  size_t total_count = (element_class == core::ElementClass::POINT) ? result->point_count() : result->primitive_count();

  std::cerr << "Total element count: " << total_count << "\n";
  std::cerr << "Elements to DELETE (not in group): ";

  // Add all elements NOT in original group to temp group
  for (size_t i = 0; i < total_count; ++i) {
    if (group_set.find(static_cast<int>(i)) == group_set.end()) {
      std::cerr << i << " ";
      core::add_to_group(*result, temp_group, element_class, static_cast<int>(i));
    }
  }
  std::cerr << "\n";

  // Now delete the temp group (which contains inverted elements)
  std::cerr << "Calling delete_elements on temp group '" << temp_group << "'\n";
  auto result_opt = result->delete_elements(temp_group, element_class, true);
  if (!result_opt.has_value()) {
    std::cerr << "ERROR: delete_elements failed!\n";
    set_error("Failed to delete inverted elements");
    return std::make_shared<core::GeometryContainer>(input->clone());
  }

  std::cerr << "Result after deletion: " << result_opt->point_count() << " points, " << result_opt->primitive_count()
            << " primitives\n";

  return std::make_shared<core::GeometryContainer>(std::move(result_opt.value()));
}

} // namespace nodo::sop
