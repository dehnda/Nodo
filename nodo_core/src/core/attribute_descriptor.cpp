#include "nodo/core/attribute_descriptor.hpp"

#include <cstring>

namespace nodo::core {

AttributeDescriptor::AttributeDescriptor(std::string name, AttributeType type,
                                         ElementClass owner,
                                         InterpolationMode interpolation)
    : name_(std::move(name)),
      type_(type),
      owner_(owner),
      interpolation_(interpolation) {
  // Use default interpolation if LINEAR was specified and type has a better
  // default
  if (interpolation_ == InterpolationMode::LINEAR) {
    interpolation_ = attribute_traits::default_interpolation(type_);
  }
}

void AttributeDescriptor::set_default_value(const void* data) {
  if (data == nullptr) {
    has_default_ = false;
    default_value_.clear();
    return;
  }

  size_t size = element_size();
  default_value_.resize(size);
  std::memcpy(default_value_.data(), data, size);
  has_default_ = true;
}

} // namespace nodo::core
