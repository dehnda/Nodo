#include "nodo/core/attribute_set.hpp"

#include <algorithm>
#include <iostream>

namespace nodo::core {

void AttributeSet::resize(size_t count) {
  element_count_ = count;
  for (auto& [name, storage] : attributes_) {
    storage->resize(count);
  }
}

void AttributeSet::reserve(size_t capacity) {
  for (auto& [name, storage] : attributes_) {
    storage->reserve(capacity);
  }
}

void AttributeSet::clear() {
  element_count_ = 0;
  for (auto& [name, storage] : attributes_) {
    storage->clear();
  }
}

void AttributeSet::clear_all() {
  attributes_.clear();
  element_count_ = 0;
}

bool AttributeSet::add_attribute(std::string_view name, AttributeType type, InterpolationMode interpolation) {
  std::string name_str(name);

  // Check if already exists
  if (attributes_.contains(name_str)) {
    return false;
  }

  // Create descriptor
  AttributeDescriptor desc(name_str, type, element_class_, interpolation);

  // Create storage
  auto storage = create_attribute_storage(desc);

  // Resize to match current element count
  if (element_count_ > 0) {
    storage->resize(element_count_);
  } else if (!attributes_.empty()) {
    // If element_count_ is 0 but we have other attributes, infer count from
    // them This handles cases where geometry was created without calling
    // set_point_count()
    auto first_attr = attributes_.begin()->second.get();
    if (first_attr && first_attr->size() > 0) {
      element_count_ = first_attr->size();
      storage->resize(element_count_);
    }
  }

  // Add to map
  attributes_[name_str] = std::move(storage);
  return true;
}

bool AttributeSet::add_attribute(const AttributeDescriptor& desc) {
  std::string name_str(desc.name());

  // Check if already exists
  if (attributes_.contains(name_str)) {
    return false;
  }

  // Verify element class matches
  if (desc.owner() != element_class_) {
    throw std::runtime_error("Attribute descriptor element class mismatch");
  }

  // Create storage
  auto storage = create_attribute_storage(desc);

  // Resize to match current element count
  if (element_count_ > 0) {
    storage->resize(element_count_);
  }

  // Add to map
  attributes_[name_str] = std::move(storage);
  return true;
}

bool AttributeSet::remove_attribute(std::string_view name) {
  std::string name_str(name);
  return attributes_.erase(name_str) > 0;
}

bool AttributeSet::has_attribute(std::string_view name) const {
  std::string name_str(name);
  return attributes_.contains(name_str);
}

std::optional<AttributeDescriptor> AttributeSet::get_descriptor(std::string_view name) const {
  auto* storage = get_storage(name);
  if (!storage) {
    return std::nullopt;
  }
  return storage->descriptor();
}

std::vector<std::string> AttributeSet::attribute_names() const {
  std::vector<std::string> names;
  names.reserve(attributes_.size());
  for (const auto& [name, storage] : attributes_) {
    names.push_back(name);
  }
  std::sort(names.begin(), names.end()); // Alphabetical order
  return names;
}

IAttributeStorage* AttributeSet::get_storage(std::string_view name) {
  // Fast path: inline string conversion for hot path
  std::string name_str(name);
  auto iter = attributes_.find(name_str);
  return (iter != attributes_.end()) ? iter->second.get() : nullptr;
}

const IAttributeStorage* AttributeSet::get_storage(std::string_view name) const {
  std::string name_str(name);
  auto iter = attributes_.find(name_str);
  return (iter != attributes_.end()) ? iter->second.get() : nullptr;
}

AttributeSet AttributeSet::clone() const {
  AttributeSet cloned(element_class_);
  cloned.element_count_ = element_count_;

  for (const auto& [name, storage] : attributes_) {
    cloned.attributes_[name] = storage->clone();
  }

  return cloned;
}

void AttributeSet::merge(const AttributeSet& other, bool overwrite) {
  if (other.element_class_ != element_class_) {
    throw std::runtime_error("Cannot merge attribute sets with different "
                             "element classes");
  }

  // If sizes differ, resize this set
  if (other.element_count_ != element_count_) {
    resize(other.element_count_);
  }

  for (const auto& [name, other_storage] : other.attributes_) {
    // Skip if already exists and not overwriting
    if (attributes_.contains(name) && !overwrite) {
      continue;
    }

    // Clone and add
    attributes_[name] = other_storage->clone();
  }
}

bool AttributeSet::validate() const {
  // Check all attributes have the same size
  for (const auto& [name, storage] : attributes_) {
    if (storage->size() != element_count_) {
      return false;
    }
  }
  return true;
}

size_t AttributeSet::memory_usage() const {
  size_t total = 0;

  for (const auto& [name, storage] : attributes_) {
    // Storage overhead + actual data
    total += sizeof(*storage);
    total += storage->capacity() * storage->descriptor().element_size();
  }

  return total;
}

} // namespace nodo::core
