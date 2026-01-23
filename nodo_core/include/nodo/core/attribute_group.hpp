#pragma once

#include "attribute_types.hpp"
#include "geometry_container.hpp"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace nodo::core {

/**
 * @brief Attribute groups for selecting subsets of geometry elements
 *
 * Groups are named sets of element indices (points or primitives).
 * They're used for:
 * - Selective operations (delete selected, modify group, etc.)
 * - Pattern-based selection (every Nth, random, by attribute)
 * - Boolean operations (union, intersection, difference)
 *
 * Groups are stored as attributes:
 * - Point groups: int attribute on points (0 = not in group, 1 = in group)
 * - Primitive groups: int attribute on primitives
 *
 * This matches Houdini's group system where groups are special attributes.
 */

/**
 * @brief Create a new group attribute
 *
 * @param container Geometry container
 * @param group_name Name of the group
 * @param element_class Which element class (POINT or PRIMITIVE)
 * @return true if created successfully, false if group already exists
 */
bool create_group(GeometryContainer& container, std::string_view group_name, ElementClass element_class);

/**
 * @brief Delete a group attribute
 *
 * @param container Geometry container
 * @param group_name Name of the group to delete
 * @param element_class Which element class
 * @return true if deleted successfully, false if group doesn't exist
 */
bool delete_group(GeometryContainer& container, std::string_view group_name, ElementClass element_class);

/**
 * @brief Check if a group exists
 */
bool has_group(const GeometryContainer& container, std::string_view group_name, ElementClass element_class);

/**
 * @brief Add an element to a group
 *
 * @param container Geometry container
 * @param group_name Name of the group
 * @param element_class Which element class
 * @param element_index Index of the element to add
 * @return true if successful
 */
bool add_to_group(GeometryContainer& container, std::string_view group_name, ElementClass element_class,
                  size_t element_index);

/**
 * @brief Add multiple elements to a group
 */
bool add_to_group(GeometryContainer& container, std::string_view group_name, ElementClass element_class,
                  const std::vector<size_t>& element_indices);

/**
 * @brief Remove an element from a group
 */
bool remove_from_group(GeometryContainer& container, std::string_view group_name, ElementClass element_class,
                       size_t element_index);

/**
 * @brief Remove multiple elements from a group
 */
bool remove_from_group(GeometryContainer& container, std::string_view group_name, ElementClass element_class,
                       const std::vector<size_t>& element_indices);

/**
 * @brief Check if an element is in a group
 */
bool is_in_group(const GeometryContainer& container, std::string_view group_name, ElementClass element_class,
                 size_t element_index);

/**
 * @brief Get all elements in a group
 *
 * @return Vector of element indices that are in the group
 */
std::vector<size_t> get_group_elements(const GeometryContainer& container, std::string_view group_name,
                                       ElementClass element_class);

/**
 * @brief Get the size (number of elements) in a group
 */
size_t get_group_size(const GeometryContainer& container, std::string_view group_name, ElementClass element_class);

/**
 * @brief Clear all elements from a group (but keep the group)
 */
bool clear_group(GeometryContainer& container, std::string_view group_name, ElementClass element_class);

// ============================================================================
// Group Operations (Boolean algebra on groups)
// ============================================================================

/**
 * @brief Union of two groups: result = A ∪ B
 *
 * Elements in either group_a OR group_b will be in the result.
 */
bool group_union(GeometryContainer& container, std::string_view group_a, std::string_view group_b,
                 std::string_view result_group, ElementClass element_class);

/**
 * @brief Intersection of two groups: result = A ∩ B
 *
 * Only elements in both group_a AND group_b will be in the result.
 */
bool group_intersection(GeometryContainer& container, std::string_view group_a, std::string_view group_b,
                        std::string_view result_group, ElementClass element_class);

/**
 * @brief Difference of two groups: result = A - B
 *
 * Elements in group_a but NOT in group_b will be in the result.
 */
bool group_difference(GeometryContainer& container, std::string_view group_a, std::string_view group_b,
                      std::string_view result_group, ElementClass element_class);

/**
 * @brief Invert a group: result = ~A
 *
 * Elements NOT in the source group will be in the result.
 */
bool group_invert(GeometryContainer& container, std::string_view source_group, std::string_view result_group,
                  ElementClass element_class);

// ============================================================================
// Pattern-Based Selection
// ============================================================================

/**
 * @brief Select every Nth element
 *
 * @param step Select every Nth element (e.g., step=2 selects 0,2,4,6...)
 * @param offset Starting offset (e.g., offset=1, step=2 selects 1,3,5,7...)
 */
bool select_pattern(GeometryContainer& container, std::string_view group_name, ElementClass element_class, size_t step,
                    size_t offset = 0);

/**
 * @brief Select a range of elements [start, end)
 */
bool select_range(GeometryContainer& container, std::string_view group_name, ElementClass element_class, size_t start,
                  size_t end);

/**
 * @brief Select random elements
 *
 * @param count Number of elements to randomly select
 * @param seed Random seed for reproducibility
 */
bool select_random(GeometryContainer& container, std::string_view group_name, ElementClass element_class, size_t count,
                   unsigned int seed = 0);

/**
 * @brief Select elements by attribute value
 *
 * @param attr_name Attribute to test
 * @param predicate Function that returns true if element should be in group
 *
 * Example:
 *   select_by_attribute(geo, "tall_points", ElementClass::POINT, "P",
 *       [](const Vec3f& P) { return P.y > 5.0f; });
 */
template <typename T>
bool select_by_attribute(GeometryContainer& container, std::string_view group_name, ElementClass element_class,
                         std::string_view attr_name, std::function<bool(const T&)> predicate);

/**
 * @brief Grow group to include neighbors
 *
 * For primitives: Add primitives that share edges with current group members.
 * For points: Add points connected by edges to current group members.
 *
 * @param iterations Number of times to grow (1 = immediate neighbors)
 */
bool grow_group(GeometryContainer& container, std::string_view group_name, ElementClass element_class,
                size_t iterations = 1);

/**
 * @brief Shrink group by removing boundary elements
 *
 * @param iterations Number of times to shrink
 */
bool shrink_group(GeometryContainer& container, std::string_view group_name, ElementClass element_class,
                  size_t iterations = 1);

/**
 * @brief Get all group names for a specific element class
 *
 * Groups are stored as attributes with the "group_" prefix.
 * This function scans all attributes and returns those that are groups.
 *
 * @param container Geometry container
 * @param element_class Which element class (POINT or PRIMITIVE)
 * @return Vector of group names (without the "group_" prefix)
 */
std::vector<std::string> get_group_names(const GeometryContainer& container, ElementClass element_class);

} // namespace nodo::core
