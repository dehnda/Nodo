#pragma once

#include "attribute_descriptor.hpp"
#include "attribute_types.hpp"
#include <cstring>
#include <memory>
#include <span>
#include <vector>

namespace nodo::core {

/**
 * @brief Abstract base class for type-erased attribute storage
 *
 * Provides a polymorphic interface for working with typed storage containers.
 * Actual storage is handled by typed subclasses.
 */
class IAttributeStorage {
public:
  virtual ~IAttributeStorage() = default;

  /**
   * @brief Get the descriptor for this attribute
   */
  virtual const AttributeDescriptor &descriptor() const = 0;

  /**
   * @brief Get number of elements stored
   */
  virtual size_t size() const = 0;

  /**
   * @brief Resize storage to hold N elements
   */
  virtual void resize(size_t count) = 0;

  /**
   * @brief Reserve capacity (avoid reallocation)
   */
  virtual void reserve(size_t capacity) = 0;

  /**
   * @brief Get capacity (allocated space)
   */
  virtual size_t capacity() const = 0;

  /**
   * @brief Clear all data
   */
  virtual void clear() = 0;

  /**
   * @brief Get raw pointer to data (for memcpy/serialization)
   */
  virtual void *data_ptr() = 0;
  virtual const void *data_ptr() const = 0;

  /**
   * @brief Clone this storage (deep copy)
   */
  virtual std::unique_ptr<IAttributeStorage> clone() const = 0;

  /**
   * @brief Copy element from another storage
   * @param from_index Source element index
   * @param to_index Destination element index
   * @param src Source storage (must have same type)
   */
  virtual void copy_element(size_t from_index, size_t to_index,
                            const IAttributeStorage &src) = 0;

  /**
   * @brief Swap two elements within this storage
   */
  virtual void swap_elements(size_t idx1, size_t idx2) = 0;
};

/**
 * @brief Typed attribute storage (SoA - Structure of Arrays)
 *
 * Stores attribute data as a contiguous typed array (std::vector<T>).
 * This is 10-100x faster than variant-based storage due to:
 * - Cache-friendly memory layout
 * - No type checks/dispatching during iteration
 * - Vectorization-friendly
 * - Zero-cost span views
 *
 * Template parameter T can be:
 * - float, int
 * - Vec2f, Vec3f, Vec4f
 * - Matrix3f, Matrix4f
 * - Quaternionf
 * - std::string
 */
template <typename T> class AttributeStorage : public IAttributeStorage {
public:
  explicit AttributeStorage(AttributeDescriptor desc)
      : descriptor_(std::move(desc)) {
    // Initialize with default value if available
    if (descriptor_.has_default()) {
      auto default_val = descriptor_.get_default<T>();
      if (default_val) {
        default_value_ = *default_val;
        has_default_ = true;
      }
    }
  }

  // IAttributeStorage interface
  const AttributeDescriptor &descriptor() const override { return descriptor_; }

  size_t size() const override { return data_.size(); }

  void resize(size_t count) override {
    if (has_default_) {
      data_.resize(count, default_value_);
    } else {
      data_.resize(count);
    }
  }

  void reserve(size_t capacity) override { data_.reserve(capacity); }

  size_t capacity() const override { return data_.capacity(); }

  void clear() override { data_.clear(); }

  void *data_ptr() override { return data_.data(); }

  const void *data_ptr() const override { return data_.data(); }

  std::unique_ptr<IAttributeStorage> clone() const override {
    auto cloned = std::make_unique<AttributeStorage<T>>(descriptor_);
    cloned->data_ = data_;
    cloned->default_value_ = default_value_;
    cloned->has_default_ = has_default_;
    return cloned;
  }

  void copy_element(size_t from_index, size_t to_index,
                    const IAttributeStorage &src) override {
    const auto *typed_src = dynamic_cast<const AttributeStorage<T> *>(&src);
    if (!typed_src) {
      throw std::runtime_error("Type mismatch in copy_element");
    }
    if (from_index >= typed_src->size() || to_index >= size()) {
      throw std::out_of_range("Index out of range in copy_element");
    }
    data_[to_index] = typed_src->data_[from_index];
  }

  void swap_elements(size_t idx1, size_t idx2) override {
    if (idx1 >= size() || idx2 >= size()) {
      throw std::out_of_range("Index out of range in swap_elements");
    }
    std::swap(data_[idx1], data_[idx2]);
  }

  // Typed accessors (fast, zero-overhead)

  /**
   * @brief Get element by index (read-only)
   * Inline for maximum performance in tight loops
   */
  inline const T &operator[](size_t index) const { return data_[index]; }

  /**
   * @brief Get element by index (writable)
   * Inline for maximum performance in tight loops
   */
  inline T &operator[](size_t index) { return data_[index]; }

  /**
   * @brief Get element with bounds checking
   */
  inline const T &at(size_t index) const { return data_.at(index); }
  inline T &at(size_t index) { return data_.at(index); }

  /**
   * @brief Get all values as a span (zero-cost view)
   * Inline for zero overhead
   */
  inline std::span<const T> values() const { return data_; }
  inline std::span<T> values_writable() { return data_; }

  /**
   * @brief Set value at index
   */
  void set(size_t index, const T &value) {
    if (index >= size()) {
      throw std::out_of_range("Index out of range");
    }
    data_[index] = value;
  }

  /**
   * @brief Push back a new element
   */
  void push_back(const T &value) { data_.push_back(value); }

  /**
   * @brief Emplace back a new element
   */
  template <typename... Args> void emplace_back(Args &&...args) {
    data_.emplace_back(std::forward<Args>(args)...);
  }

  /**
   * @brief Get underlying vector (for advanced use)
   */
  const std::vector<T> &get_vector() const { return data_; }
  std::vector<T> &get_vector_writable() { return data_; }

private:
  AttributeDescriptor descriptor_;
  std::vector<T> data_; // SoA: contiguous typed array
  T default_value_{};
  bool has_default_ = false;
};

// Specialization for string (slightly different default handling)
template <> class AttributeStorage<std::string> : public IAttributeStorage {
public:
  explicit AttributeStorage(AttributeDescriptor desc)
      : descriptor_(std::move(desc)) {}

  const AttributeDescriptor &descriptor() const override { return descriptor_; }
  size_t size() const override { return data_.size(); }
  void resize(size_t count) override { data_.resize(count); }
  void reserve(size_t capacity) override { data_.reserve(capacity); }
  size_t capacity() const override { return data_.capacity(); }
  void clear() override { data_.clear(); }

  void *data_ptr() override { return data_.data(); }
  const void *data_ptr() const override { return data_.data(); }

  std::unique_ptr<IAttributeStorage> clone() const override {
    auto cloned = std::make_unique<AttributeStorage<std::string>>(descriptor_);
    cloned->data_ = data_;
    return cloned;
  }

  void copy_element(size_t from_index, size_t to_index,
                    const IAttributeStorage &src) override {
    const auto *typed_src =
        dynamic_cast<const AttributeStorage<std::string> *>(&src);
    if (!typed_src) {
      throw std::runtime_error("Type mismatch in copy_element");
    }
    if (from_index >= typed_src->size() || to_index >= size()) {
      throw std::out_of_range("Index out of range in copy_element");
    }
    data_[to_index] = typed_src->data_[from_index];
  }

  void swap_elements(size_t idx1, size_t idx2) override {
    if (idx1 >= size() || idx2 >= size()) {
      throw std::out_of_range("Index out of range in swap_elements");
    }
    std::swap(data_[idx1], data_[idx2]);
  }

  // Typed accessors
  const std::string &operator[](size_t index) const { return data_[index]; }
  std::string &operator[](size_t index) { return data_[index]; }
  const std::string &at(size_t index) const { return data_.at(index); }
  std::string &at(size_t index) { return data_.at(index); }

  std::span<const std::string> values() const { return data_; }
  std::span<std::string> values_writable() { return data_; }

  void set(size_t index, const std::string &value) {
    if (index >= size()) {
      throw std::out_of_range("Index out of range");
    }
    data_[index] = value;
  }

  void push_back(const std::string &value) { data_.push_back(value); }

  const std::vector<std::string> &get_vector() const { return data_; }
  std::vector<std::string> &get_vector_writable() { return data_; }

private:
  AttributeDescriptor descriptor_;
  std::vector<std::string> data_;
};

/**
 * @brief Factory to create typed AttributeStorage from descriptor
 */
inline std::unique_ptr<IAttributeStorage>
create_attribute_storage(const AttributeDescriptor &desc) {
  switch (desc.type()) {
  case AttributeType::FLOAT:
    return std::make_unique<AttributeStorage<float>>(desc);
  case AttributeType::INT:
    return std::make_unique<AttributeStorage<int>>(desc);
  case AttributeType::VEC2F:
    return std::make_unique<AttributeStorage<Vec2f>>(desc);
  case AttributeType::VEC3F:
    return std::make_unique<AttributeStorage<Vec3f>>(desc);
  case AttributeType::VEC4F:
    return std::make_unique<AttributeStorage<Vec4f>>(desc);
  case AttributeType::MATRIX3:
    return std::make_unique<AttributeStorage<Matrix3f>>(desc);
  case AttributeType::MATRIX4:
    return std::make_unique<AttributeStorage<Matrix4f>>(desc);
  case AttributeType::QUATERNION:
    return std::make_unique<AttributeStorage<Quaternionf>>(desc);
  case AttributeType::STRING:
    return std::make_unique<AttributeStorage<std::string>>(desc);
  default:
    throw std::runtime_error("Unknown attribute type");
  }
}

} // namespace nodo::core
