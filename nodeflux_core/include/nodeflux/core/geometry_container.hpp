#pragma once

#include "attribute_set.hpp"
#include "element_topology.hpp"
#include "standard_attributes.hpp"
#include <memory>
#include <optional>

namespace nodeflux::core {

/**
 * @brief Complete geometry representation with topology and attributes
 *
 * Combines ElementTopology (structure) with AttributeSets (data) to represent
 * a complete procedural geometry. This is the main data container used
 * throughout the NodeFlux pipeline.
 *
 * Architecture:
 * - ElementTopology: Point/Vertex/Primitive structure (who connects to whom)
 * - AttributeSet per element class: Typed attribute storage (positions,
 * normals, etc.)
 *
 * Standard workflow:
 * 1. Create topology (set_point_count, set_vertex_count, add_primitives)
 * 2. Add attributes (add_point_attribute, add_vertex_attribute, etc.)
 * 3. Populate data (get_point_attribute_typed<Vec3f>("P"), etc.)
 *
 * Example:
 *   GeometryContainer geo;
 *   geo.set_point_count(8);
 *   geo.set_vertex_count(24);  // Cube with split normals
 *
 *   geo.add_point_attribute("P", AttributeType::VEC3F);
 *   geo.add_vertex_attribute("N", AttributeType::VEC3F);
 *
 *   auto* positions = geo.get_point_attribute_typed<Vec3f>("P");
 *   (*positions)[0] = Vec3f(0, 0, 0);
 */
class GeometryContainer {
public:
  GeometryContainer() = default;
  ~GeometryContainer() = default;

  // Prevent copying (use clone() for explicit copy)
  GeometryContainer(const GeometryContainer &) = delete;
  GeometryContainer &operator=(const GeometryContainer &) = delete;

  // Allow moving
  GeometryContainer(GeometryContainer &&) noexcept = default;
  GeometryContainer &operator=(GeometryContainer &&) noexcept = default;

  // ============================================================================
  // Topology Access
  // ============================================================================

  ElementTopology &topology() { return topology_; }
  const ElementTopology &topology() const { return topology_; }

  // Element counts (delegated to topology)
  size_t point_count() const { return topology_.point_count(); }
  size_t vertex_count() const { return topology_.vertex_count(); }
  size_t primitive_count() const { return topology_.primitive_count(); }

  // Topology modification
  void set_point_count(size_t count) {
    topology_.set_point_count(count);
    point_attrs_.resize(count);
  }

  void set_vertex_count(size_t count) {
    topology_.set_vertex_count(count);
    vertex_attrs_.resize(count);
  }

  void set_primitive_count(size_t count) {
    topology_.set_primitive_count(count);
    primitive_attrs_.resize(count);
  }

  void reserve_vertices(size_t capacity) {
    topology_.reserve_vertices(capacity);
    vertex_attrs_.reserve(capacity);
  }

  void reserve_primitives(size_t capacity) {
    topology_.reserve_primitives(capacity);
    primitive_attrs_.reserve(capacity);
  }

  size_t add_primitive(const std::vector<int> &vertices) {
    size_t idx = topology_.add_primitive(vertices);
    primitive_attrs_.resize(primitive_count());
    return idx;
  }

  // ============================================================================
  // Attribute Management - Point Attributes
  // ============================================================================

  bool add_point_attribute(
      std::string_view name, AttributeType type,
      InterpolationMode interpolation = InterpolationMode::LINEAR) {
    return point_attrs_.add_attribute(name, type, interpolation);
  }

  bool remove_point_attribute(std::string_view name) {
    return point_attrs_.remove_attribute(name);
  }

  bool has_point_attribute(std::string_view name) const {
    return point_attrs_.has_attribute(name);
  }

  IAttributeStorage *get_point_attribute(std::string_view name) {
    return point_attrs_.get_storage(name);
  }

  const IAttributeStorage *get_point_attribute(std::string_view name) const {
    return point_attrs_.get_storage(name);
  }

  template <typename T>
  AttributeStorage<T> *get_point_attribute_typed(std::string_view name) {
    return point_attrs_.get_storage_typed<T>(name);
  }

  template <typename T>
  const AttributeStorage<T> *
  get_point_attribute_typed(std::string_view name) const {
    return point_attrs_.get_storage_typed<T>(name);
  }

  // ============================================================================
  // Attribute Management - Vertex Attributes
  // ============================================================================

  bool add_vertex_attribute(
      std::string_view name, AttributeType type,
      InterpolationMode interpolation = InterpolationMode::LINEAR) {
    return vertex_attrs_.add_attribute(name, type, interpolation);
  }

  bool remove_vertex_attribute(std::string_view name) {
    return vertex_attrs_.remove_attribute(name);
  }

  bool has_vertex_attribute(std::string_view name) const {
    return vertex_attrs_.has_attribute(name);
  }

  IAttributeStorage *get_vertex_attribute(std::string_view name) {
    return vertex_attrs_.get_storage(name);
  }

  const IAttributeStorage *get_vertex_attribute(std::string_view name) const {
    return vertex_attrs_.get_storage(name);
  }

  template <typename T>
  AttributeStorage<T> *get_vertex_attribute_typed(std::string_view name) {
    return vertex_attrs_.get_storage_typed<T>(name);
  }

  template <typename T>
  const AttributeStorage<T> *
  get_vertex_attribute_typed(std::string_view name) const {
    return vertex_attrs_.get_storage_typed<T>(name);
  }

  // ============================================================================
  // Attribute Management - Primitive Attributes
  // ============================================================================

  bool add_primitive_attribute(
      std::string_view name, AttributeType type,
      InterpolationMode interpolation = InterpolationMode::LINEAR) {
    return primitive_attrs_.add_attribute(name, type, interpolation);
  }

  bool remove_primitive_attribute(std::string_view name) {
    return primitive_attrs_.remove_attribute(name);
  }

  bool has_primitive_attribute(std::string_view name) const {
    return primitive_attrs_.has_attribute(name);
  }

  IAttributeStorage *get_primitive_attribute(std::string_view name) {
    return primitive_attrs_.get_storage(name);
  }

  const IAttributeStorage *
  get_primitive_attribute(std::string_view name) const {
    return primitive_attrs_.get_storage(name);
  }

  template <typename T>
  AttributeStorage<T> *get_primitive_attribute_typed(std::string_view name) {
    return primitive_attrs_.get_storage_typed<T>(name);
  }

  template <typename T>
  const AttributeStorage<T> *
  get_primitive_attribute_typed(std::string_view name) const {
    return primitive_attrs_.get_storage_typed<T>(name);
  }

  // ============================================================================
  // Attribute Management - Detail (Global) Attributes
  // ============================================================================

  bool add_detail_attribute(
      std::string_view name, AttributeType type,
      InterpolationMode interpolation = InterpolationMode::LINEAR) {
    return detail_attrs_.add_attribute(name, type, interpolation);
  }

  bool remove_detail_attribute(std::string_view name) {
    return detail_attrs_.remove_attribute(name);
  }

  bool has_detail_attribute(std::string_view name) const {
    return detail_attrs_.has_attribute(name);
  }

  IAttributeStorage *get_detail_attribute(std::string_view name) {
    return detail_attrs_.get_storage(name);
  }

  const IAttributeStorage *get_detail_attribute(std::string_view name) const {
    return detail_attrs_.get_storage(name);
  }

  template <typename T>
  AttributeStorage<T> *get_detail_attribute_typed(std::string_view name) {
    return detail_attrs_.get_storage_typed<T>(name);
  }

  template <typename T>
  const AttributeStorage<T> *
  get_detail_attribute_typed(std::string_view name) const {
    return detail_attrs_.get_storage_typed<T>(name);
  }

  // ============================================================================
  // Convenience Accessors for Standard Attributes
  // ============================================================================

  /**
   * @brief Get point positions (standard "P" attribute)
   * @return Pointer to position storage, or nullptr if not present
   */
  AttributeStorage<Vec3f> *positions() {
    return get_point_attribute_typed<Vec3f>(standard_attrs::P);
  }

  const AttributeStorage<Vec3f> *positions() const {
    return get_point_attribute_typed<Vec3f>(standard_attrs::P);
  }

  /**
   * @brief Get vertex normals (standard "N" attribute)
   */
  AttributeStorage<Vec3f> *normals() {
    return get_vertex_attribute_typed<Vec3f>(standard_attrs::N);
  }

  const AttributeStorage<Vec3f> *normals() const {
    return get_vertex_attribute_typed<Vec3f>(standard_attrs::N);
  }

  /**
   * @brief Get vertex UVs (standard "uv" attribute)
   */
  AttributeStorage<Vec2f> *uvs() {
    return get_vertex_attribute_typed<Vec2f>(standard_attrs::uv);
  }

  const AttributeStorage<Vec2f> *uvs() const {
    return get_vertex_attribute_typed<Vec2f>(standard_attrs::uv);
  }

  /**
   * @brief Get point colors (standard "Cd" attribute)
   */
  AttributeStorage<Vec3f> *colors() {
    return get_point_attribute_typed<Vec3f>(standard_attrs::Cd);
  }

  const AttributeStorage<Vec3f> *colors() const {
    return get_point_attribute_typed<Vec3f>(standard_attrs::Cd);
  }

  // ============================================================================
  // Utility Methods
  // ============================================================================

  /**
   * @brief Initialize standard position attribute if not present
   */
  void ensure_position_attribute() {
    if (!has_point_attribute(standard_attrs::P)) {
      add_point_attribute(standard_attrs::P, AttributeType::VEC3F);
    }
  }

  /**
   * @brief Initialize standard normal attribute if not present
   */
  void ensure_normal_attribute() {
    if (!has_vertex_attribute(standard_attrs::N)) {
      add_vertex_attribute(standard_attrs::N, AttributeType::VEC3F);
    }
  }

  /**
   * @brief Clear all data (topology and attributes)
   */
  void clear() {
    topology_.clear();
    point_attrs_.clear();
    vertex_attrs_.clear();
    primitive_attrs_.clear();
    detail_attrs_.clear();
  }

  /**
   * @brief Validate topology and attributes are consistent
   */
  bool validate() const {
    // Check topology
    if (!topology_.validate()) {
      return false;
    }

    // Check attribute sizes match topology
    if (point_attrs_.size() != point_count()) {
      return false;
    }
    if (vertex_attrs_.size() != vertex_count()) {
      return false;
    }
    if (primitive_attrs_.size() != primitive_count()) {
      return false;
    }

    // Validate each attribute set
    return point_attrs_.validate() && vertex_attrs_.validate() &&
           primitive_attrs_.validate() && detail_attrs_.validate();
  }

  /**
   * @brief Clone this geometry (deep copy)
   */
  GeometryContainer clone() const {
    GeometryContainer cloned;
    cloned.topology_ = topology_;
    cloned.point_attrs_ = point_attrs_.clone();
    cloned.vertex_attrs_ = vertex_attrs_.clone();
    cloned.primitive_attrs_ = primitive_attrs_.clone();
    cloned.detail_attrs_ = detail_attrs_.clone();
    return cloned;
  }

  /**
   * @brief Get total memory usage in bytes
   */
  size_t memory_usage() const {
    return sizeof(topology_) + point_attrs_.memory_usage() +
           vertex_attrs_.memory_usage() + primitive_attrs_.memory_usage() +
           detail_attrs_.memory_usage();
  }

  /**
   * @brief Get statistics for debugging
   */
  struct Stats {
    size_t points;
    size_t vertices;
    size_t primitives;
    size_t point_attributes;
    size_t vertex_attributes;
    size_t primitive_attributes;
    size_t detail_attributes;
    size_t total_memory_bytes;
  };

  Stats compute_stats() const {
    Stats stats;
    stats.points = point_count();
    stats.vertices = vertex_count();
    stats.primitives = primitive_count();
    stats.point_attributes = point_attrs_.attribute_count();
    stats.vertex_attributes = vertex_attrs_.attribute_count();
    stats.primitive_attributes = primitive_attrs_.attribute_count();
    stats.detail_attributes = detail_attrs_.attribute_count();
    stats.total_memory_bytes = memory_usage();
    return stats;
  }

  // Direct access to attribute sets (for advanced use)
  AttributeSet &point_attributes() { return point_attrs_; }
  const AttributeSet &point_attributes() const { return point_attrs_; }

  AttributeSet &vertex_attributes() { return vertex_attrs_; }
  const AttributeSet &vertex_attributes() const { return vertex_attrs_; }

  AttributeSet &primitive_attributes() { return primitive_attrs_; }
  const AttributeSet &primitive_attributes() const { return primitive_attrs_; }

  AttributeSet &detail_attributes() { return detail_attrs_; }
  const AttributeSet &detail_attributes() const { return detail_attrs_; }

private:
  ElementTopology topology_;

  AttributeSet point_attrs_{ElementClass::POINT};
  AttributeSet vertex_attrs_{ElementClass::VERTEX};
  AttributeSet primitive_attrs_{ElementClass::PRIMITIVE};
  AttributeSet detail_attrs_{ElementClass::DETAIL};
};

} // namespace nodeflux::core
