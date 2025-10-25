#include "nodeflux/core/attribute_group.hpp"
#include "nodeflux/core/attribute_types.hpp"
#include <algorithm>
#include <random>
#include <unordered_map>

namespace nodeflux::core {

// Helper to get group attribute name (prefix with "group_")
static std::string get_group_attr_name(std::string_view group_name) {
  return std::string("group_") + std::string(group_name);
}

// Helper to get attribute set for element class
static AttributeSet *get_attr_set(GeometryContainer &container,
                                  ElementClass element_class) {
  switch (element_class) {
  case ElementClass::POINT:
    return &container.point_attributes();
  case ElementClass::PRIMITIVE:
    return &container.primitive_attributes();
  case ElementClass::VERTEX:
    return &container.vertex_attributes();
  case ElementClass::DETAIL:
    // DETAIL attributes not yet supported for groups
    return nullptr;
  }
  return nullptr;
}

static const AttributeSet *
get_attr_set_const(const GeometryContainer &container,
                   ElementClass element_class) {
  switch (element_class) {
  case ElementClass::POINT:
    return &container.point_attributes();
  case ElementClass::PRIMITIVE:
    return &container.primitive_attributes();
  case ElementClass::VERTEX:
    return &container.vertex_attributes();
  case ElementClass::DETAIL:
    // DETAIL attributes not yet supported for groups
    return nullptr;
  }
  return nullptr;
}

// ============================================================================
// Basic Group Management
// ============================================================================

bool create_group(GeometryContainer &container, std::string_view group_name,
                  ElementClass element_class) {
  const std::string attr_name = get_group_attr_name(group_name);

  switch (element_class) {
  case ElementClass::POINT:
    return container.add_point_attribute(attr_name, AttributeType::INT);
  case ElementClass::PRIMITIVE:
    return container.add_primitive_attribute(attr_name, AttributeType::INT);
  case ElementClass::VERTEX:
    return container.add_vertex_attribute(attr_name, AttributeType::INT);
  case ElementClass::DETAIL:
    // DETAIL groups not supported yet
    return false;
  }
  return false;
}

bool delete_group(GeometryContainer &container, std::string_view group_name,
                  ElementClass element_class) {
  const std::string attr_name = get_group_attr_name(group_name);
  auto *attr_set = get_attr_set(container, element_class);

  if (!attr_set) {
    return false;
  }

  return attr_set->remove_attribute(attr_name);
}

bool has_group(const GeometryContainer &container, std::string_view group_name,
               ElementClass element_class) {
  const std::string attr_name = get_group_attr_name(group_name);
  const auto *attr_set = get_attr_set_const(container, element_class);

  if (!attr_set) {
    return false;
  }

  return attr_set->has_attribute(attr_name);
}

bool add_to_group(GeometryContainer &container, std::string_view group_name,
                  ElementClass element_class, size_t element_index) {
  const std::string attr_name = get_group_attr_name(group_name);

  // Get the group attribute
  IAttributeStorage *group_attr = nullptr;
  switch (element_class) {
  case ElementClass::POINT:
    group_attr = container.get_point_attribute(attr_name);
    break;
  case ElementClass::PRIMITIVE:
    group_attr = container.get_primitive_attribute(attr_name);
    break;
  case ElementClass::VERTEX:
    group_attr = container.get_vertex_attribute(attr_name);
    break;
  case ElementClass::DETAIL:
    return false;
  }

  if (!group_attr || element_index >= group_attr->size()) {
    return false;
  }

  // Set the group membership (cast to int storage)
  auto *int_storage = dynamic_cast<AttributeStorage<int> *>(group_attr);
  if (!int_storage) {
    return false;
  }

  int_storage->values_writable()[element_index] = 1;
  return true;
}

bool add_to_group(GeometryContainer &container, std::string_view group_name,
                  ElementClass element_class,
                  const std::vector<size_t> &element_indices) {
  const std::string attr_name = get_group_attr_name(group_name);

  IAttributeStorage *group_attr = nullptr;
  switch (element_class) {
  case ElementClass::POINT:
    group_attr = container.get_point_attribute(attr_name);
    break;
  case ElementClass::PRIMITIVE:
    group_attr = container.get_primitive_attribute(attr_name);
    break;
  case ElementClass::VERTEX:
    group_attr = container.get_vertex_attribute(attr_name);
    break;
  case ElementClass::DETAIL:
    return false;
  }

  if (!group_attr) {
    return false;
  }

  auto *int_storage = dynamic_cast<AttributeStorage<int> *>(group_attr);
  if (!int_storage) {
    return false;
  }

  auto values = int_storage->values_writable();
  for (size_t idx : element_indices) {
    if (idx < values.size()) {
      values[idx] = 1;
    }
  }

  return true;
}

bool remove_from_group(GeometryContainer &container,
                       std::string_view group_name, ElementClass element_class,
                       size_t element_index) {
  const std::string attr_name = get_group_attr_name(group_name);

  IAttributeStorage *group_attr = nullptr;
  switch (element_class) {
  case ElementClass::POINT:
    group_attr = container.get_point_attribute(attr_name);
    break;
  case ElementClass::PRIMITIVE:
    group_attr = container.get_primitive_attribute(attr_name);
    break;
  case ElementClass::VERTEX:
    group_attr = container.get_vertex_attribute(attr_name);
    break;
  case ElementClass::DETAIL:
    return false;
  }

  if (!group_attr || element_index >= group_attr->size()) {
    return false;
  }

  auto *int_storage = dynamic_cast<AttributeStorage<int> *>(group_attr);
  if (!int_storage) {
    return false;
  }

  int_storage->values_writable()[element_index] = 0;
  return true;
}

bool remove_from_group(GeometryContainer &container,
                       std::string_view group_name, ElementClass element_class,
                       const std::vector<size_t> &element_indices) {
  const std::string attr_name = get_group_attr_name(group_name);

  IAttributeStorage *group_attr = nullptr;
  switch (element_class) {
  case ElementClass::POINT:
    group_attr = container.get_point_attribute(attr_name);
    break;
  case ElementClass::PRIMITIVE:
    group_attr = container.get_primitive_attribute(attr_name);
    break;
  case ElementClass::VERTEX:
    group_attr = container.get_vertex_attribute(attr_name);
    break;
  case ElementClass::DETAIL:
    return false;
  }

  if (!group_attr) {
    return false;
  }

  auto *int_storage = dynamic_cast<AttributeStorage<int> *>(group_attr);
  if (!int_storage) {
    return false;
  }

  auto values = int_storage->values_writable();
  for (size_t idx : element_indices) {
    if (idx < values.size()) {
      values[idx] = 0;
    }
  }

  return true;
}

bool is_in_group(const GeometryContainer &container,
                 std::string_view group_name, ElementClass element_class,
                 size_t element_index) {
  const std::string attr_name = get_group_attr_name(group_name);

  const IAttributeStorage *group_attr = nullptr;
  switch (element_class) {
  case ElementClass::POINT:
    group_attr = container.get_point_attribute(attr_name);
    break;
  case ElementClass::PRIMITIVE:
    group_attr = container.get_primitive_attribute(attr_name);
    break;
  case ElementClass::VERTEX:
    group_attr = container.get_vertex_attribute(attr_name);
    break;
  case ElementClass::DETAIL:
    return false;
  }

  if (!group_attr || element_index >= group_attr->size()) {
    return false;
  }

  const auto *int_storage =
      dynamic_cast<const AttributeStorage<int> *>(group_attr);
  if (!int_storage) {
    return false;
  }

  return int_storage->values()[element_index] != 0;
}

std::vector<size_t> get_group_elements(const GeometryContainer &container,
                                       std::string_view group_name,
                                       ElementClass element_class) {
  std::vector<size_t> result;
  const std::string attr_name = get_group_attr_name(group_name);

  const IAttributeStorage *group_attr = nullptr;
  switch (element_class) {
  case ElementClass::POINT:
    group_attr = container.get_point_attribute(attr_name);
    break;
  case ElementClass::PRIMITIVE:
    group_attr = container.get_primitive_attribute(attr_name);
    break;
  case ElementClass::VERTEX:
    group_attr = container.get_vertex_attribute(attr_name);
    break;
  case ElementClass::DETAIL:
    return result;
  }

  if (!group_attr) {
    return result;
  }

  const auto *int_storage =
      dynamic_cast<const AttributeStorage<int> *>(group_attr);
  if (!int_storage) {
    return result;
  }

  auto values = int_storage->values();
  for (size_t i = 0; i < values.size(); ++i) {
    if (values[i] != 0) {
      result.push_back(i);
    }
  }

  return result;
}

size_t get_group_size(const GeometryContainer &container,
                      std::string_view group_name, ElementClass element_class) {
  return get_group_elements(container, group_name, element_class).size();
}

bool clear_group(GeometryContainer &container, std::string_view group_name,
                 ElementClass element_class) {
  const std::string attr_name = get_group_attr_name(group_name);

  IAttributeStorage *group_attr = nullptr;
  switch (element_class) {
  case ElementClass::POINT:
    group_attr = container.get_point_attribute(attr_name);
    break;
  case ElementClass::PRIMITIVE:
    group_attr = container.get_primitive_attribute(attr_name);
    break;
  case ElementClass::VERTEX:
    group_attr = container.get_vertex_attribute(attr_name);
    break;
  case ElementClass::DETAIL:
    return false;
  }

  if (!group_attr) {
    return false;
  }

  auto *int_storage = dynamic_cast<AttributeStorage<int> *>(group_attr);
  if (!int_storage) {
    return false;
  }

  auto values = int_storage->values_writable();
  std::fill(values.begin(), values.end(), 0);

  return true;
}

// ============================================================================
// Group Operations
// ============================================================================

bool group_union(GeometryContainer &container, std::string_view group_a,
                 std::string_view group_b, std::string_view result_group,
                 ElementClass element_class) {
  // Create result group if it doesn't exist
  if (!has_group(container, result_group, element_class)) {
    if (!create_group(container, result_group, element_class)) {
      return false;
    }
  }

  auto elements_a = get_group_elements(container, group_a, element_class);
  auto elements_b = get_group_elements(container, group_b, element_class);

  // Union: combine both sets
  std::unordered_set<size_t> union_set(elements_a.begin(), elements_a.end());
  union_set.insert(elements_b.begin(), elements_b.end());

  // Clear result and add union
  clear_group(container, result_group, element_class);
  std::vector<size_t> union_vec(union_set.begin(), union_set.end());
  return add_to_group(container, result_group, element_class, union_vec);
}

bool group_intersection(GeometryContainer &container, std::string_view group_a,
                        std::string_view group_b, std::string_view result_group,
                        ElementClass element_class) {
  if (!has_group(container, result_group, element_class)) {
    if (!create_group(container, result_group, element_class)) {
      return false;
    }
  }

  auto elements_a = get_group_elements(container, group_a, element_class);
  auto elements_b = get_group_elements(container, group_b, element_class);

  // Intersection: only elements in both
  std::unordered_set<size_t> set_b(elements_b.begin(), elements_b.end());
  std::vector<size_t> intersection;

  for (size_t elem : elements_a) {
    if (set_b.count(elem) > 0) {
      intersection.push_back(elem);
    }
  }

  clear_group(container, result_group, element_class);
  return add_to_group(container, result_group, element_class, intersection);
}

bool group_difference(GeometryContainer &container, std::string_view group_a,
                      std::string_view group_b, std::string_view result_group,
                      ElementClass element_class) {
  if (!has_group(container, result_group, element_class)) {
    if (!create_group(container, result_group, element_class)) {
      return false;
    }
  }

  auto elements_a = get_group_elements(container, group_a, element_class);
  auto elements_b = get_group_elements(container, group_b, element_class);

  // Difference: elements in A but not in B
  std::unordered_set<size_t> set_b(elements_b.begin(), elements_b.end());
  std::vector<size_t> difference;

  for (size_t elem : elements_a) {
    if (set_b.count(elem) == 0) {
      difference.push_back(elem);
    }
  }

  clear_group(container, result_group, element_class);
  return add_to_group(container, result_group, element_class, difference);
}

bool group_invert(GeometryContainer &container, std::string_view source_group,
                  std::string_view result_group, ElementClass element_class) {
  if (!has_group(container, result_group, element_class)) {
    if (!create_group(container, result_group, element_class)) {
      return false;
    }
  }

  auto elements = get_group_elements(container, source_group, element_class);
  std::unordered_set<size_t> in_group(elements.begin(), elements.end());

  // Get total element count
  size_t total_count = 0;
  switch (element_class) {
  case ElementClass::POINT:
    total_count = container.point_count();
    break;
  case ElementClass::PRIMITIVE:
    total_count = container.primitive_count();
    break;
  case ElementClass::VERTEX:
    total_count = container.vertex_count();
    break;
  case ElementClass::DETAIL:
    return false;
  }

  // Add all elements NOT in the source group
  std::vector<size_t> inverted;
  for (size_t i = 0; i < total_count; ++i) {
    if (in_group.count(i) == 0) {
      inverted.push_back(i);
    }
  }

  clear_group(container, result_group, element_class);
  return add_to_group(container, result_group, element_class, inverted);
}

// ============================================================================
// Pattern-Based Selection
// ============================================================================

bool select_pattern(GeometryContainer &container, std::string_view group_name,
                    ElementClass element_class, size_t step, size_t offset) {
  if (!has_group(container, group_name, element_class)) {
    if (!create_group(container, group_name, element_class)) {
      return false;
    }
  }

  if (step == 0) {
    return false;
  }

  size_t total_count = 0;
  switch (element_class) {
  case ElementClass::POINT:
    total_count = container.point_count();
    break;
  case ElementClass::PRIMITIVE:
    total_count = container.primitive_count();
    break;
  case ElementClass::VERTEX:
    total_count = container.vertex_count();
    break;
  case ElementClass::DETAIL:
    return false;
  }

  std::vector<size_t> selected;
  for (size_t i = offset; i < total_count; i += step) {
    selected.push_back(i);
  }

  clear_group(container, group_name, element_class);
  return add_to_group(container, group_name, element_class, selected);
}

bool select_range(GeometryContainer &container, std::string_view group_name,
                  ElementClass element_class, size_t start, size_t end) {
  if (!has_group(container, group_name, element_class)) {
    if (!create_group(container, group_name, element_class)) {
      return false;
    }
  }

  size_t total_count = 0;
  switch (element_class) {
  case ElementClass::POINT:
    total_count = container.point_count();
    break;
  case ElementClass::PRIMITIVE:
    total_count = container.primitive_count();
    break;
  case ElementClass::VERTEX:
    total_count = container.vertex_count();
    break;
  case ElementClass::DETAIL:
    return false;
  }

  end = std::min(end, total_count);

  std::vector<size_t> selected;
  for (size_t i = start; i < end; ++i) {
    selected.push_back(i);
  }

  clear_group(container, group_name, element_class);
  return add_to_group(container, group_name, element_class, selected);
}

bool select_random(GeometryContainer &container, std::string_view group_name,
                   ElementClass element_class, size_t count,
                   unsigned int seed) {
  if (!has_group(container, group_name, element_class)) {
    if (!create_group(container, group_name, element_class)) {
      return false;
    }
  }

  size_t total_count = 0;
  switch (element_class) {
  case ElementClass::POINT:
    total_count = container.point_count();
    break;
  case ElementClass::PRIMITIVE:
    total_count = container.primitive_count();
    break;
  case ElementClass::VERTEX:
    total_count = container.vertex_count();
    break;
  case ElementClass::DETAIL:
    return false;
  }

  count = std::min(count, total_count);

  // Create indices vector [0, 1, 2, ..., total_count-1]
  std::vector<size_t> indices(total_count);
  std::iota(indices.begin(), indices.end(), 0);

  // Shuffle and take first 'count' elements
  std::mt19937 rng(seed);
  std::shuffle(indices.begin(), indices.end(), rng);

  std::vector<size_t> selected(indices.begin(), indices.begin() + count);

  clear_group(container, group_name, element_class);
  return add_to_group(container, group_name, element_class, selected);
}

// Template specializations for select_by_attribute
template <typename T>
bool select_by_attribute(GeometryContainer &container,
                         std::string_view group_name,
                         ElementClass element_class, std::string_view attr_name,
                         std::function<bool(const T &)> predicate) {
  if (!has_group(container, group_name, element_class)) {
    if (!create_group(container, group_name, element_class)) {
      return false;
    }
  }

  // Get the attribute
  const AttributeStorage<T> *attr = nullptr;
  switch (element_class) {
  case ElementClass::POINT:
    attr = container.get_point_attribute_typed<T>(attr_name);
    break;
  case ElementClass::PRIMITIVE:
    attr = container.get_primitive_attribute_typed<T>(attr_name);
    break;
  case ElementClass::VERTEX:
    attr = container.get_vertex_attribute_typed<T>(attr_name);
    break;
  case ElementClass::DETAIL:
    return false;
  }

  if (!attr) {
    return false;
  }

  auto values = attr->values();
  std::vector<size_t> selected;

  for (size_t i = 0; i < values.size(); ++i) {
    if (predicate(values[i])) {
      selected.push_back(i);
    }
  }

  clear_group(container, group_name, element_class);
  return add_to_group(container, group_name, element_class, selected);
}

// Explicit template instantiations
template bool select_by_attribute<float>(GeometryContainer &, std::string_view,
                                         ElementClass, std::string_view,
                                         std::function<bool(const float &)>);
template bool select_by_attribute<int>(GeometryContainer &, std::string_view,
                                       ElementClass, std::string_view,
                                       std::function<bool(const int &)>);
template bool select_by_attribute<Vec3f>(GeometryContainer &, std::string_view,
                                         ElementClass, std::string_view,
                                         std::function<bool(const Vec3f &)>);

// Grow and shrink are not implemented yet (require connectivity analysis)
bool grow_group(GeometryContainer & /*container*/,
                std::string_view /*group_name*/, ElementClass /*element_class*/,
                size_t /*iterations*/) {
  // TODO: Implement using connectivity information
  return false;
}

bool shrink_group(GeometryContainer & /*container*/,
                  std::string_view /*group_name*/,
                  ElementClass /*element_class*/, size_t /*iterations*/) {
  // TODO: Implement using connectivity information
  return false;
}

} // namespace nodeflux::core
