#pragma once

#include "attribute_types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace nodo::core {

/**
 * @brief Descriptor/metadata for a single attribute
 *
 * Immutable schema object describing an attribute's properties.
 * Contains everything needed to create, validate, and serialize the attribute.
 *
 * Design notes:
 * - Lightweight (copyable)
 * - Immutable once created (use builder pattern for construction)
 * - Version tracking for change detection
 * - Default values stored as raw bytes for efficiency
 */
class AttributeDescriptor {
public:
  /**
   * @brief Construct an attribute descriptor
   *
   * @param name Attribute name (e.g., "P", "N", "uv")
   * @param type Data type (float, Vec3f, etc.)
   * @param owner Which element class owns this attribute
   * @param interpolation How to interpolate values
   */
  AttributeDescriptor(
      std::string name, AttributeType type, ElementClass owner,
      InterpolationMode interpolation = InterpolationMode::LINEAR);

  // Getters
  const std::string& name() const { return name_; }
  AttributeType type() const { return type_; }
  ElementClass owner() const { return owner_; }
  InterpolationMode interpolation() const { return interpolation_; }
  uint64_t version() const { return version_; }

  /**
   * @brief Get size of a single element in bytes
   */
  size_t element_size() const { return attribute_traits::size_of(type_); }

  /**
   * @brief Get number of scalar components
   */
  size_t component_count() const {
    return attribute_traits::component_count(type_);
  }

  /**
   * @brief Check if this attribute has a default value
   */
  bool has_default() const { return has_default_; }

  /**
   * @brief Get default value as raw bytes
   * @return Pointer to default value data, or nullptr if no default
   */
  const void* default_value_ptr() const {
    return has_default_ ? default_value_.data() : nullptr;
  }

  /**
   * @brief Set default value from raw bytes
   * @param data Pointer to value data
   */
  void set_default_value(const void* data);

  /**
   * @brief Set default value from typed data
   */
  template <typename T>
  void set_default(const T& value) {
    set_default_value(&value);
  }

  /**
   * @brief Get default value as typed data
   */
  template <typename T>
  std::optional<T> get_default() const {
    if (!has_default_ || sizeof(T) != element_size()) {
      return std::nullopt;
    }
    // Use placement new + copy constructor instead of memcpy for proper object
    // initialization
    T result;
    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(&result, default_value_.data(), sizeof(T));
    } else {
      // For non-trivially copyable types (like Eigen), use proper copy
      // construction
      const T* source = reinterpret_cast<const T*>(default_value_.data());
      result = *source;
    }
    return result;
  }

  // Metadata flags
  bool is_numeric() const { return attribute_traits::is_numeric(type_); }
  bool is_vector() const { return attribute_traits::is_vector(type_); }
  bool is_matrix() const { return attribute_traits::is_matrix(type_); }

  /**
   * @brief Get human-readable type name
   */
  const char* type_name() const { return attribute_traits::type_name(type_); }

  /**
   * @brief Get human-readable owner class name
   */
  const char* owner_name() const {
    return attribute_traits::element_class_name(owner_);
  }

  /**
   * @brief Get human-readable interpolation mode name
   */
  const char* interpolation_name() const {
    return attribute_traits::interpolation_mode_name(interpolation_);
  }

  /**
   * @brief Equality comparison (name only, for lookup)
   */
  bool operator==(const AttributeDescriptor& other) const {
    return name_ == other.name_;
  }

  /**
   * @brief Full equality (all fields)
   */
  bool equals(const AttributeDescriptor& other) const {
    return name_ == other.name_ && type_ == other.type_ &&
           owner_ == other.owner_ && interpolation_ == other.interpolation_;
  }

  /**
   * @brief Increment version (for change tracking)
   */
  void increment_version() { ++version_; }

private:
  std::string name_;
  AttributeType type_;
  ElementClass owner_;
  InterpolationMode interpolation_;
  uint64_t version_ = 0;

  // Default value storage (raw bytes)
  bool has_default_ = false;
  std::vector<uint8_t> default_value_;
};

/**
 * @brief Builder for AttributeDescriptor (fluent API)
 *
 * Example usage:
 *   auto desc = AttributeDescriptorBuilder("N", AttributeType::VEC3F,
 * ElementClass::VERTEX) .interpolation(InterpolationMode::LINEAR)
 *               .default_value(Vec3f(0, 0, 1))
 *               .build();
 */
class AttributeDescriptorBuilder {
public:
  AttributeDescriptorBuilder(std::string name, AttributeType type,
                             ElementClass owner)
      : desc_(std::move(name), type, owner) {}

  AttributeDescriptorBuilder& interpolation(InterpolationMode mode) {
    desc_ =
        AttributeDescriptor(desc_.name(), desc_.type(), desc_.owner(), mode);
    return *this;
  }

  template <typename T>
  AttributeDescriptorBuilder& default_value(const T& value) {
    desc_.set_default(value);
    return *this;
  }

  AttributeDescriptor build() const { return desc_; }

private:
  AttributeDescriptor desc_;
};

} // namespace nodo::core
