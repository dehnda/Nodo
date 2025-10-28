#include "nodo/core/geometry_attributes.hpp"
#include <typeinfo>

namespace nodo::core {

// Constants for geometric operations
constexpr double TRIANGLE_VERTEX_COUNT = 3.0;
constexpr double DEFAULT_COLOR_VALUE = 0.8; // Light gray RGB components

// ============================================================================
// AttributeData Implementation
// ============================================================================

AttributeData::AttributeData(AttributeClass class_type, size_t size) 
  : class_(class_type) {
  data_.resize(size);
}

template<typename T>
void AttributeData::set_value(size_t index, const T& value) {
  if (index >= data_.size()) {
    return; // Bounds check
  }
  data_[index] = value;
}

template<typename T>
std::optional<T> AttributeData::get_value(size_t index) const {
  if (index >= data_.size()) {
    return std::nullopt;
  }
  
  if (const T* value = std::get_if<T>(&data_[index])) {
    return *value;
  }
  
  return std::nullopt;
}

template<typename T>
void AttributeData::set_all_values(const std::vector<T>& values) {
  data_.clear();
  data_.reserve(values.size());
  for (const auto& value : values) {
    data_.emplace_back(value);
  }
}

template<typename T>
std::vector<T> AttributeData::get_all_values() const {
  std::vector<T> result;
  result.reserve(data_.size());
  
  for (const auto& variant_value : data_) {
    if (const T* value = std::get_if<T>(&variant_value)) {
      result.push_back(*value);
    } else {
      // Handle type mismatch - could throw or use default value
      result.push_back(T{});
    }
  }
  
  return result;
}

void AttributeData::resize(size_t new_size) {
  data_.resize(new_size);
}

void AttributeData::clear() {
  data_.clear();
}

bool AttributeData::holds_type(const std::type_info& type) const {
  if (data_.empty()) {
    return false;
  }
  
  // Check the first element's type as a representative
  return std::visit([&type](const auto& value) {
    return typeid(value) == type;
  }, data_[0]);
}

template<typename T>
bool AttributeData::holds_type() const {
  return holds_type(typeid(T));
}

// ============================================================================
// GeometryAttributes Implementation
// ============================================================================

template<typename T>
void GeometryAttributes::add_attribute(const std::string& name, AttributeClass class_type, size_t initial_size) {
  auto attribute_data = std::make_unique<AttributeData>(class_type, initial_size);
  
  // Initialize with default values
  if (initial_size > 0) {
    std::vector<T> default_values(initial_size, T{});
    attribute_data->set_all_values(default_values);
  }
  
  attributes_[name] = std::move(attribute_data);
}

bool GeometryAttributes::remove_attribute(const std::string& name) {
  auto attribute_iterator = attributes_.find(name);
  if (attribute_iterator != attributes_.end()) {
    attributes_.erase(attribute_iterator);
    return true;
  }
  return false;
}

bool GeometryAttributes::has_attribute(const std::string& name) const {
  return attributes_.find(name) != attributes_.end();
}

std::optional<AttributeClass> GeometryAttributes::get_attribute_class(const std::string& name) const {
  const auto* data = get_attribute_data(name);
  if (data != nullptr) {
    return data->get_class();
  }
  return std::nullopt;
}

template<typename T>
bool GeometryAttributes::set_attribute(const std::string& name, size_t index, const T& value) {
  auto* data = get_attribute_data(name);
  if (data != nullptr) {
    data->set_value(index, value);
    return true;
  }
  return false;
}

template<typename T>
std::optional<T> GeometryAttributes::get_attribute(const std::string& name, size_t index) const {
  const auto* data = get_attribute_data(name);
  if (data != nullptr) {
    return data->get_value<T>(index);
  }
  return std::nullopt;
}

template<typename T>
bool GeometryAttributes::set_attribute_array(const std::string& name, const std::vector<T>& values) {
  auto* data = get_attribute_data(name);
  if (data != nullptr) {
    data->set_all_values(values);
    return true;
  }
  return false;
}

template<typename T>
std::optional<std::vector<T>> GeometryAttributes::get_attribute_array(const std::string& name) const {
  const auto* data = get_attribute_data(name);
  if (data != nullptr) {
    return data->get_all_values<T>();
  }
  return std::nullopt;
}

// ============================================================================
// Geometric Attribute Helpers
// ============================================================================

bool GeometryAttributes::set_position(size_t vertex_index, const Vector3& position) {
  return set_attribute("position", vertex_index, position);
}

bool GeometryAttributes::set_normal(size_t vertex_index, const Vector3& normal) {
  return set_attribute("normal", vertex_index, normal);
}

bool GeometryAttributes::set_color(size_t vertex_index, const Vector3& color) {
  return set_attribute("color", vertex_index, color);
}

bool GeometryAttributes::set_uv_coordinates(size_t vertex_index, const Vector2f& uv_coords) {
  return set_attribute("uv", vertex_index, uv_coords);
}

std::optional<Vector3> GeometryAttributes::get_position(size_t vertex_index) const {
  return get_attribute<Vector3>("position", vertex_index);
}

std::optional<Vector3> GeometryAttributes::get_normal(size_t vertex_index) const {
  return get_attribute<Vector3>("normal", vertex_index);
}

std::optional<Vector3> GeometryAttributes::get_color(size_t vertex_index) const {
  return get_attribute<Vector3>("color", vertex_index);
}

std::optional<Vector2f> GeometryAttributes::get_uv_coordinates(size_t vertex_index) const {
  return get_attribute<Vector2f>("uv", vertex_index);
}

// ============================================================================
// Attribute Transfer and Promotion
// ============================================================================

void GeometryAttributes::transfer_attributes(const GeometryAttributes& source,
                                           const std::vector<int>& vertex_mapping,
                                           const std::vector<int>& face_mapping) {
  // Transfer vertex attributes if mapping provided
  if (!vertex_mapping.empty()) {
    for (const auto& attr_name : source.get_attribute_names(AttributeClass::VERTEX)) {
      const auto* source_data = source.get_attribute_data(attr_name);
      if (source_data == nullptr) continue;
      
      // Create matching attribute if needed
      if (!has_attribute(attr_name)) {
        if (source_data->holds_type<Vector3>()) {
          add_attribute<Vector3>(attr_name, AttributeClass::VERTEX, vertex_mapping.size());
        } else if (source_data->holds_type<Vector2f>()) {
          add_attribute<Vector2f>(attr_name, AttributeClass::VERTEX, vertex_mapping.size());
        } else if (source_data->holds_type<float>()) {
          add_attribute<float>(attr_name, AttributeClass::VERTEX, vertex_mapping.size());
        }
      }
      
      // Transfer values using mapping
      for (size_t dest_idx = 0; dest_idx < vertex_mapping.size(); ++dest_idx) {
        int source_idx = vertex_mapping[dest_idx];
        if (source_idx >= 0) {
          if (source_data->holds_type<Vector3>()) {
            auto value = source_data->get_value<Vector3>(source_idx);
            if (value.has_value()) {
              set_attribute(attr_name, dest_idx, value.value());
            }
          }
        }
      }
    }
  }
  
  // Transfer face attributes if mapping provided
  if (!face_mapping.empty()) {
    for (const auto& attr_name : source.get_attribute_names(AttributeClass::FACE)) {
      const auto* source_data = source.get_attribute_data(attr_name);
      if (source_data == nullptr) continue;
      
      // Create and transfer face attributes (simplified implementation)
      if (source_data->holds_type<int>()) {
        add_attribute<int>(attr_name, AttributeClass::FACE, face_mapping.size());
      }
    }
  }
}

bool GeometryAttributes::promote_vertex_to_face(const std::string& vertex_attr_name,
                                               const std::string& face_attr_name,
                                               const std::vector<Vector3i>& faces) {
  const auto* vertex_data = get_attribute_data(vertex_attr_name);
  if (vertex_data == nullptr || vertex_data->get_class() != AttributeClass::VERTEX) {
    return false;
  }
  
  // For Vector3 attributes, average the vertex values for each face
  if (vertex_data->holds_type<Vector3>()) {
    add_attribute<Vector3>(face_attr_name, AttributeClass::FACE, faces.size());
    
    for (size_t face_idx = 0; face_idx < faces.size(); ++face_idx) {
      const auto& face = faces[face_idx];
      
      auto vertex_0_value = vertex_data->get_value<Vector3>(face[0]);
      auto vertex_1_value = vertex_data->get_value<Vector3>(face[1]);
      auto vertex_2_value = vertex_data->get_value<Vector3>(face[2]);
      
      if (vertex_0_value.has_value() && vertex_1_value.has_value() && vertex_2_value.has_value()) {
        Vector3 face_value = (vertex_0_value.value() + vertex_1_value.value() + vertex_2_value.value()) / TRIANGLE_VERTEX_COUNT;
        set_attribute(face_attr_name, face_idx, face_value);
      }
    }
    return true;
  }
  
  return false;
}

bool GeometryAttributes::demote_face_to_vertex(const std::string& face_attr_name,
                                              const std::string& vertex_attr_name,
                                              const std::vector<Vector3i>& faces,
                                              size_t vertex_count) {
  const auto* face_data = get_attribute_data(face_attr_name);
  if (face_data == nullptr || face_data->get_class() != AttributeClass::FACE) {
    return false;
  }
  
  // For Vector3 attributes, replicate face values to vertices
  if (face_data->holds_type<Vector3>()) {
    add_attribute<Vector3>(vertex_attr_name, AttributeClass::VERTEX, vertex_count);
    
    // Initialize with zero values
    std::vector<Vector3> vertex_values(vertex_count, Vector3::Zero());
    std::vector<int> vertex_counts(vertex_count, 0);
    
    // Accumulate face values to vertices
    for (size_t face_idx = 0; face_idx < faces.size(); ++face_idx) {
      auto face_value = face_data->get_value<Vector3>(face_idx);
      if (face_value.has_value()) {
        const auto& face = faces[face_idx];
        for (int i = 0; i < 3; ++i) {
          vertex_values[face[i]] += face_value.value();
          vertex_counts[face[i]]++;
        }
      }
    }
    
    // Average the accumulated values
    for (size_t vertex_idx = 0; vertex_idx < vertex_count; ++vertex_idx) {
      if (vertex_counts[vertex_idx] > 0) {
        vertex_values[vertex_idx] /= static_cast<double>(vertex_counts[vertex_idx]);
      }
      set_attribute(vertex_attr_name, vertex_idx, vertex_values[vertex_idx]);
    }
    
    return true;
  }
  
  return false;
}

// ============================================================================
// Batch Operations and Resizing
// ============================================================================

void GeometryAttributes::resize_attributes(AttributeClass class_type, size_t new_size) {
  for (auto& [name, data] : attributes_) {
    if (data->get_class() == class_type) {
      data->resize(new_size);
    }
  }
}

void GeometryAttributes::clear_all() {
  attributes_.clear();
}

size_t GeometryAttributes::get_attribute_count(AttributeClass class_type) const {
  size_t count = 0;
  for (const auto& [name, data] : attributes_) {
    if (data->get_class() == class_type) {
      ++count;
    }
  }
  return count;
}

std::vector<std::string> GeometryAttributes::get_attribute_names(AttributeClass class_type) const {
  std::vector<std::string> names;
  for (const auto& [name, data] : attributes_) {
    if (data->get_class() == class_type) {
      names.push_back(name);
    }
  }
  return names;
}

std::vector<std::string> GeometryAttributes::get_all_attribute_names() const {
  std::vector<std::string> names;
  names.reserve(attributes_.size());
  for (const auto& [name, data] : attributes_) {
    names.push_back(name);
  }
  return names;
}

// ============================================================================
// Standard Attribute Initialization
// ============================================================================

void GeometryAttributes::initialize_standard_attributes(size_t vertex_count, size_t face_count) {
  // Essential vertex attributes
  add_attribute<Vector3>("position", AttributeClass::VERTEX, vertex_count);
  add_attribute<Vector3>("normal", AttributeClass::VERTEX, vertex_count);
  add_attribute<Vector3>("color", AttributeClass::VERTEX, vertex_count);
  add_attribute<Vector2f>("uv", AttributeClass::VERTEX, vertex_count);
  
  // Common face attributes
  add_attribute<int>("material_id", AttributeClass::FACE, face_count);
  add_attribute<int>("group_id", AttributeClass::FACE, face_count);
  
  // Initialize with reasonable defaults
  std::vector<Vector3> default_colors(vertex_count, Vector3(DEFAULT_COLOR_VALUE, DEFAULT_COLOR_VALUE, DEFAULT_COLOR_VALUE)); // Light gray
  set_attribute_array("color", default_colors);
  
  std::vector<int> default_material_ids(face_count, 0);
  set_attribute_array("material_id", default_material_ids);
}

void GeometryAttributes::ensure_attribute_exists(const std::string& name, AttributeClass class_type, size_t size) {
  if (!has_attribute(name)) {
    // Default to Vector3 for unknown attributes
    add_attribute<Vector3>(name, class_type, size);
  }
}

// ============================================================================
// Private Helpers
// ============================================================================

AttributeData* GeometryAttributes::get_attribute_data(const std::string& name) {
  auto attribute_iterator = attributes_.find(name);
  return (attribute_iterator != attributes_.end()) ? attribute_iterator->second.get() : nullptr;
}

const AttributeData* GeometryAttributes::get_attribute_data(const std::string& name) const {
  auto attribute_iterator = attributes_.find(name);
  return (attribute_iterator != attributes_.end()) ? attribute_iterator->second.get() : nullptr;
}

// Explicit Template Instantiations
// ============================================================================

// AttributeData templates
template void AttributeData::set_value<Vector3>(size_t, const Vector3&);
template void AttributeData::set_value<Vector2f>(size_t, const Vector2f&);
template void AttributeData::set_value<float>(size_t, const float&);
template void AttributeData::set_value<int>(size_t, const int&);
template void AttributeData::set_value<std::string>(size_t, const std::string&);

template std::optional<Vector3> AttributeData::get_value<Vector3>(size_t) const;
template std::optional<Vector2f> AttributeData::get_value<Vector2f>(size_t) const;
template std::optional<float> AttributeData::get_value<float>(size_t) const;
template std::optional<int> AttributeData::get_value<int>(size_t) const;
template std::optional<std::string> AttributeData::get_value<std::string>(size_t) const;

template bool AttributeData::holds_type<Vector3>() const;
template bool AttributeData::holds_type<Vector2f>() const;
template bool AttributeData::holds_type<float>() const;
template bool AttributeData::holds_type<int>() const;
template bool AttributeData::holds_type<std::string>() const;

// GeometryAttributes templates
template void GeometryAttributes::add_attribute<Vector3>(const std::string&, AttributeClass, size_t);
template void GeometryAttributes::add_attribute<Vector2f>(const std::string&, AttributeClass, size_t);
template void GeometryAttributes::add_attribute<float>(const std::string&, AttributeClass, size_t);
template void GeometryAttributes::add_attribute<int>(const std::string&, AttributeClass, size_t);
template void GeometryAttributes::add_attribute<std::string>(const std::string&, AttributeClass, size_t);

template bool GeometryAttributes::set_attribute<Vector3>(const std::string&, size_t, const Vector3&);
template bool GeometryAttributes::set_attribute<Vector2f>(const std::string&, size_t, const Vector2f&);
template bool GeometryAttributes::set_attribute<float>(const std::string&, size_t, const float&);
template bool GeometryAttributes::set_attribute<int>(const std::string&, size_t, const int&);
template bool GeometryAttributes::set_attribute<std::string>(const std::string&, size_t, const std::string&);

template std::optional<Vector3> GeometryAttributes::get_attribute<Vector3>(const std::string&, size_t) const;
template std::optional<Vector2f> GeometryAttributes::get_attribute<Vector2f>(const std::string&, size_t) const;
template std::optional<float> GeometryAttributes::get_attribute<float>(const std::string&, size_t) const;
template std::optional<int> GeometryAttributes::get_attribute<int>(const std::string&, size_t) const;
template std::optional<std::string> GeometryAttributes::get_attribute<std::string>(const std::string&, size_t) const;

} // namespace nodo::core
