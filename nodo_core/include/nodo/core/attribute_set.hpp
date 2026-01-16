#pragma once

#include "attribute_descriptor.hpp"
#include "attribute_storage.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace nodo::core {

/**
 * @brief Container for all attributes of a specific element class
 *
 * Manages a collection of typed attribute storages (one per attribute).
 * Provides fast lookup by name and type-safe access.
 *
 * Design:
 * - One AttributeSet per element class (points, vertices, primitives, detail)
 * - Attributes stored as typed SoA (AttributeStorage<T>)
 * - Fast name-based lookup via hash map
 * - Automatic resizing of all attributes when element count changes
 *
 * Example usage:
 *   AttributeSet point_attrs(ElementClass::POINT);
 *   point_attrs.add_attribute("P", AttributeType::VEC3F);
 *   point_attrs.add_attribute("Cd", AttributeType::VEC3F);
 *   point_attrs.resize(100); // All attributes now have 100 elements
 *
 *   auto* positions = point_attrs.get_storage_typed<Vec3f>("P");
 *   (*positions)[0] = Vec3f(1, 2, 3);
 */
class AttributeSet {
public:
  explicit AttributeSet(ElementClass element_class) : element_class_(element_class) {}

  // Prevent copying (use clone() for explicit copy)
  AttributeSet(const AttributeSet&) = delete;
  AttributeSet& operator=(const AttributeSet&) = delete;

  // Allow moving
  AttributeSet(AttributeSet&&) noexcept = default;
  AttributeSet& operator=(AttributeSet&&) noexcept = default;

  /**
   * @brief Get the element class this set belongs to
   */
  ElementClass element_class() const { return element_class_; }

  /**
   * @brief Get number of elements (all attributes have this size)
   */
  size_t size() const { return element_count_; }

  /**
   * @brief Resize all attributes to hold N elements
   */
  void resize(size_t count);

  /**
   * @brief Reserve capacity in all attributes
   */
  void reserve(size_t capacity);

  /**
   * @brief Clear all attribute data (keep descriptors)
   */
  void clear();

  /**
   * @brief Remove all attributes
   */
  void clear_all();

  /**
   * @brief Add a new attribute
   * @param name Attribute name
   * @param type Attribute type
   * @param interpolation Interpolation mode (defaults to type's default)
   * @return true if added, false if already exists
   */
  bool add_attribute(std::string_view name, AttributeType type,
                     InterpolationMode interpolation = InterpolationMode::LINEAR);

  /**
   * @brief Add attribute from descriptor
   */
  bool add_attribute(const AttributeDescriptor& desc);

  /**
   * @brief Remove an attribute by name
   * @return true if removed, false if not found
   */
  bool remove_attribute(std::string_view name);

  /**
   * @brief Check if attribute exists
   */
  bool has_attribute(std::string_view name) const;

  /**
   * @brief Get attribute descriptor
   */
  std::optional<AttributeDescriptor> get_descriptor(std::string_view name) const;

  /**
   * @brief Get number of attributes
   */
  size_t attribute_count() const { return attributes_.size(); }

  /**
   * @brief Get all attribute names
   */
  std::vector<std::string> attribute_names() const;

  /**
   * @brief Get type-erased storage (for generic operations)
   */
  IAttributeStorage* get_storage(std::string_view name);
  const IAttributeStorage* get_storage(std::string_view name) const;

  /**
   * @brief Get typed storage (fast, zero-cost access)
   * @return Pointer to typed storage, or nullptr if not found or wrong type
   */
  template <typename T>
  AttributeStorage<T>* get_storage_typed(std::string_view name) {
    auto* storage = get_storage(name);
    if (!storage) {
      return nullptr;
    }
    return dynamic_cast<AttributeStorage<T>*>(storage);
  }

  template <typename T>
  const AttributeStorage<T>* get_storage_typed(std::string_view name) const {
    auto* storage = get_storage(name);
    if (!storage) {
      return nullptr;
    }
    return dynamic_cast<const AttributeStorage<T>*>(storage);
  }

  /**
   * @brief Clone this attribute set (deep copy)
   */
  AttributeSet clone() const;

  /**
   * @brief Merge another attribute set into this one
   * @param other Source attribute set
   * @param overwrite If true, overwrite existing attributes
   */
  void merge(const AttributeSet& other, bool overwrite = false);

  /**
   * @brief Get element count for validation
   */
  bool validate() const;

  /**
   * @brief Get memory usage in bytes
   */
  size_t memory_usage() const;

  /**
   * @brief Iterator access to attributes (for advanced use)
   */
  auto begin() { return attributes_.begin(); }
  auto end() { return attributes_.end(); }
  auto begin() const { return attributes_.begin(); }
  auto end() const { return attributes_.end(); }

private:
  ElementClass element_class_;
  size_t element_count_ = 0;

  // Map of attribute name -> storage
  std::unordered_map<std::string, std::unique_ptr<IAttributeStorage>> attributes_;
};

} // namespace nodo::core
