#pragma once

#include "nodo/core/geometry_container.hpp"

#include <memory>

namespace nodo::core {

/**
 * @brief Copy-on-Write handle for GeometryContainer
 *
 * GeometryHandle provides automatic copy-on-write semantics for geometry data.
 * Multiple handles can share the same underlying geometry until one needs to modify it,
 * at which point a copy is made automatically.
 *
 * Usage:
 * @code
 * // Share geometry (no copy)
 * GeometryHandle handle1 = create_box();
 * GeometryHandle handle2 = handle1;  // Shares data, use_count = 2
 *
 * // Read access (no copy)
 * const auto& geo = handle1.read();
 * float volume = calculate_volume(geo);
 *
 * // Write access (copies if shared)
 * auto& geo = handle2.write();  // Creates copy since use_count > 1
 * geo.transform(matrix);        // Modifies the copy
 * @endcode
 */
class GeometryHandle {
public:
  /**
   * @brief Construct empty handle
   */
  GeometryHandle() = default;

  /**
   * @brief Construct from existing container (takes ownership)
   * @param container Shared pointer to geometry (can be nullptr)
   */
  explicit GeometryHandle(std::shared_ptr<const GeometryContainer> container) : data_(std::move(container)) {}

  /**
   * @brief Construct from mutable container (converts to const shared_ptr)
   * @param container Shared pointer to mutable geometry
   */
  explicit GeometryHandle(std::shared_ptr<GeometryContainer> container)
      : data_(std::const_pointer_cast<const GeometryContainer>(std::move(container))) {}

  /**
   * @brief Copy constructor - shares the underlying data
   */
  GeometryHandle(const GeometryHandle&) = default;

  /**
   * @brief Move constructor
   */
  GeometryHandle(GeometryHandle&&) noexcept = default;

  /**
   * @brief Copy assignment - shares the underlying data
   */
  GeometryHandle& operator=(const GeometryHandle&) = default;

  /**
   * @brief Move assignment
   */
  GeometryHandle& operator=(GeometryHandle&&) noexcept = default;

  /**
   * @brief Check if handle contains valid geometry
   */
  bool is_valid() const { return data_ != nullptr; }

  /**
   * @brief Check if handle is empty
   */
  bool is_empty() const { return data_ == nullptr; }

  /**
   * @brief Get read-only access to geometry (no copy)
   * @return Const reference to geometry
   * @throws std::runtime_error if handle is empty
   */
  const GeometryContainer& read() const {
    if (!data_) {
      throw std::runtime_error("GeometryHandle::read() called on empty handle");
    }
    return *data_;
  }

  /**
   * @brief Get read-only pointer access (no copy)
   * @return Const pointer to geometry (nullptr if empty)
   */
  const GeometryContainer* operator->() const { return data_.get(); }

  /**
   * @brief Get read-only reference (no copy)
   * @return Const reference to geometry
   * @throws std::runtime_error if handle is empty
   */
  const GeometryContainer& operator*() const { return read(); }

  /**
   * @brief Get writable access to geometry (copies if shared)
   *
   * If this handle shares data with other handles (use_count > 1),
   * a copy is made before returning writable access.
   *
   * @return Mutable reference to geometry
   * @throws std::runtime_error if handle is empty
   */
  GeometryContainer& write() {
    if (!data_) {
      throw std::runtime_error("GeometryHandle::write() called on empty handle");
    }
    make_unique();
    return *const_cast<GeometryContainer*>(data_.get());
  }

  /**
   * @brief Check if this handle is the sole owner of the data
   * @return True if use_count == 1 (no copy needed for write)
   */
  bool is_unique() const { return data_ && data_.use_count() == 1; }

  /**
   * @brief Get reference count (number of handles sharing this data)
   * @return Use count, or 0 if handle is empty
   */
  long use_count() const { return data_ ? data_.use_count() : 0; }

  /**
   * @brief Force a copy if data is shared
   *
   * Makes this handle the sole owner by copying if use_count > 1.
   * Called automatically by write(), but can be called explicitly
   * if mutation is guaranteed.
   */
  void make_unique() {
    if (data_ && !is_unique()) {
      data_ = std::make_shared<GeometryContainer>(data_->clone());
    }
  }

  /**
   * @brief Create an independent deep copy
   * @return New handle with cloned geometry
   */
  GeometryHandle clone() const {
    if (!data_) {
      return GeometryHandle();
    }
    return GeometryHandle(std::make_shared<GeometryContainer>(data_->clone()));
  }

  /**
   * @brief Reset to empty state
   */
  void reset() { data_.reset(); }

  /**
   * @brief Swap contents with another handle
   */
  void swap(GeometryHandle& other) noexcept { data_.swap(other.data_); }

private:
  std::shared_ptr<const GeometryContainer> data_;
};

} // namespace nodo::core
