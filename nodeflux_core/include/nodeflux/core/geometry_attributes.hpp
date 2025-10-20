#pragma once

#include "types.hpp"
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <optional>
#include <memory>

namespace nodeflux::core {

/**
 * @brief Defines the scope/level at which an attribute applies
 */
enum class AttributeClass {
  VERTEX,     ///< Per-vertex attributes (position, normal, color, UV)
  FACE,       ///< Per-face attributes (material ID, group ID)
  PRIMITIVE,  ///< Per-primitive attributes (object-level metadata)
  GLOBAL      ///< Global attributes (mesh-level properties)
};

/**
 * @brief Supported attribute data types for procedural modeling
 */
using AttributeValue = std::variant<
  float,           // Single float values
  double,          // Double precision values
  int,             // Integer values
  Vector3,         // 3D vectors (positions, normals, colors)
  Vector2f,        // 2D vectors (UV coordinates)
  std::string      // String metadata
>;

/**
 * @brief Storage container for a single attribute across all elements
 */
class AttributeData {
public:
  AttributeData() = default;
  AttributeData(AttributeClass class_type, size_t size);
  
  // Type-safe attribute access
  template<typename T>
  void set_value(size_t index, const T& value);
  
  template<typename T>
  std::optional<T> get_value(size_t index) const;
  
  // Batch operations
  template<typename T>
  void set_all_values(const std::vector<T>& values);
  
  template<typename T>
  std::vector<T> get_all_values() const;
  
  // Attribute management
  void resize(size_t new_size);
  void clear();
  
  // Properties
  size_t size() const { return data_.size(); }
  AttributeClass get_class() const { return class_; }
  bool empty() const { return data_.empty(); }
  
  // Type introspection
  bool holds_type(const std::type_info& type) const;
  
  template<typename T>
  bool holds_type() const;

private:
  AttributeClass class_;
  std::vector<AttributeValue> data_;
};

/**
 * @brief Complete attribute management system for procedural geometry
 * 
 * This class manages all attributes associated with mesh geometry, including
 * per-vertex, per-face, per-primitive, and global attributes. It provides
 * type-safe access, automatic resizing, and efficient batch operations.
 */
class GeometryAttributes {
public:
  GeometryAttributes() = default;
  ~GeometryAttributes() = default;
  
  // Copy and move semantics
  GeometryAttributes(const GeometryAttributes& other) = default;
  GeometryAttributes& operator=(const GeometryAttributes& other) = default;
  GeometryAttributes(GeometryAttributes&& other) noexcept = default;
  GeometryAttributes& operator=(GeometryAttributes&& other) noexcept = default;

  // ============================================================================
  // Attribute Creation and Management
  // ============================================================================
  
  /**
   * @brief Add a new attribute with specified type and class
   * @param name Unique name for the attribute
   * @param class_type The scope level (vertex, face, primitive, global)
   * @param initial_size Initial size for the attribute array
   */
  template<typename T>
  void add_attribute(const std::string& name, AttributeClass class_type, size_t initial_size = 0);
  
  /**
   * @brief Remove an attribute by name
   */
  bool remove_attribute(const std::string& name);
  
  /**
   * @brief Check if an attribute exists
   */
  bool has_attribute(const std::string& name) const;
  
  /**
   * @brief Get the class type of an attribute
   */
  std::optional<AttributeClass> get_attribute_class(const std::string& name) const;

  // ============================================================================
  // Type-Safe Attribute Access
  // ============================================================================
  
  /**
   * @brief Set a single attribute value
   */
  template<typename T>
  bool set_attribute(const std::string& name, size_t index, const T& value);
  
  /**
   * @brief Get a single attribute value
   */
  template<typename T>
  std::optional<T> get_attribute(const std::string& name, size_t index) const;
  
  /**
   * @brief Set all values for an attribute at once
   */
  template<typename T>
  bool set_attribute_array(const std::string& name, const std::vector<T>& values);
  
  /**
   * @brief Get all values for an attribute
   */
  template<typename T>
  std::optional<std::vector<T>> get_attribute_array(const std::string& name) const;

  // ============================================================================
  // Geometric Attribute Helpers
  // ============================================================================
  
  /**
   * @brief Convenience methods for common 3D attributes
   */
  bool set_position(size_t vertex_index, const Vector3& position);
  bool set_normal(size_t vertex_index, const Vector3& normal);
  bool set_color(size_t vertex_index, const Vector3& color);
  bool set_uv_coordinates(size_t vertex_index, const Vector2f& uv_coords);
  
  std::optional<Vector3> get_position(size_t vertex_index) const;
  std::optional<Vector3> get_normal(size_t vertex_index) const;
  std::optional<Vector3> get_color(size_t vertex_index) const;
  std::optional<Vector2f> get_uv_coordinates(size_t vertex_index) const;

  // ============================================================================
  // Attribute Transfer and Promotion
  // ============================================================================
  
  /**
   * @brief Transfer attributes from another geometry with index mapping
   * @param source Source geometry attributes
   * @param vertex_mapping Mapping from source to destination vertex indices
   * @param face_mapping Mapping from source to destination face indices
   */
  void transfer_attributes(const GeometryAttributes& source,
                          const std::vector<int>& vertex_mapping = {},
                          const std::vector<int>& face_mapping = {});
  
  /**
   * @brief Promote vertex attribute to face attribute (by averaging)
   */
  bool promote_vertex_to_face(const std::string& vertex_attr_name,
                             const std::string& face_attr_name,
                             const std::vector<Vector3i>& faces);
  
  /**
   * @brief Demote face attribute to vertex attribute (by replication)
   */
  bool demote_face_to_vertex(const std::string& face_attr_name,
                            const std::string& vertex_attr_name,
                            const std::vector<Vector3i>& faces,
                            size_t vertex_count);

  // ============================================================================
  // Batch Operations and Resizing
  // ============================================================================
  
  /**
   * @brief Resize all attributes of a specific class
   */
  void resize_attributes(AttributeClass class_type, size_t new_size);
  
  /**
   * @brief Clear all attributes
   */
  void clear_all();
  
  /**
   * @brief Get count of attributes by class
   */
  size_t get_attribute_count(AttributeClass class_type) const;
  
  /**
   * @brief Get names of all attributes of a specific class
   */
  std::vector<std::string> get_attribute_names(AttributeClass class_type) const;
  
  /**
   * @brief Get names of all attributes
   */
  std::vector<std::string> get_all_attribute_names() const;

  // ============================================================================
  // Standard Attribute Initialization
  // ============================================================================
  
  /**
   * @brief Initialize standard mesh attributes with default values
   */
  void initialize_standard_attributes(size_t vertex_count, size_t face_count);
  
  /**
   * @brief Ensure essential attributes exist
   */
  void ensure_attribute_exists(const std::string& name, AttributeClass class_type, size_t size);

private:
  /// Storage for all attributes
  std::unordered_map<std::string, std::unique_ptr<AttributeData>> attributes_;
  
  /// Helper for getting attribute data safely
  AttributeData* get_attribute_data(const std::string& name);
  const AttributeData* get_attribute_data(const std::string& name) const;
};

} // namespace nodeflux::core
