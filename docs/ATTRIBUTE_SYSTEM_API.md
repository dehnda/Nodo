# Attribute System API Reference

**NodeFluxEngine Attribute System**
Version: 2.0 (October 2025)
Status: Production Ready ✅

---

## Table of Contents

1. [Overview](#overview)
2. [Core Classes](#core-classes)
3. [Element Classes](#element-classes)
4. [Attribute Types](#attribute-types)
5. [Basic Operations](#basic-operations)
6. [Advanced Features](#advanced-features)
7. [Best Practices](#best-practices)
8. [Performance Notes](#performance-notes)

---

## Overview

The NodeFluxEngine attribute system provides a high-performance, type-safe framework for storing and manipulating geometric data attributes. It follows Houdini's proven attribute model with modern C++20 idioms.

### Key Features

- ✅ **Type-safe**: Compile-time and runtime type checking
- ✅ **High-performance**: Structure-of-Arrays (SoA) layout for cache efficiency
- ✅ **Flexible**: Support for multiple element classes and data types
- ✅ **Standard names**: Houdini-compatible naming ("P", "N", "Cd", etc.)
- ✅ **Advanced operations**: Promotion/demotion, groups, interpolation
- ✅ **Thread-safe**: Designed for concurrent read access

### Architecture

```
GeometryContainer
├── AttributeSet (points)
│   └── AttributeStorage<T> × N attributes
├── AttributeSet (vertices)
│   └── AttributeStorage<T> × N attributes
├── AttributeSet (primitives)
│   └── AttributeStorage<T> × N attributes
└── AttributeSet (detail)
    └── AttributeStorage<T> × N attributes
```

---

## Core Classes

### GeometryContainer

**Header**: `nodeflux/core/geometry_container.hpp`

The main container for all geometry data including topology and attributes.

```cpp
namespace nodeflux::core {

class GeometryContainer {
public:
    // Construction
    GeometryContainer() = default;
    explicit GeometryContainer(size_t num_points);

    // Topology
    size_t num_points() const;
    size_t num_vertices() const;
    size_t num_primitives() const;

    void set_num_points(size_t count);
    void set_num_vertices(size_t count);
    void set_num_primitives(size_t count);

    // Points
    size_t add_point(const Vec3f& position);
    Vec3f get_point_position(size_t point_id) const;
    void set_point_position(size_t point_id, const Vec3f& position);

    // Primitives
    size_t add_primitive(std::span<const int> vertex_list);
    std::span<const int> get_primitive_vertices(size_t prim_id) const;

    // Attribute access
    AttributeSet& point_attributes();
    const AttributeSet& point_attributes() const;

    AttributeSet& vertex_attributes();
    const AttributeSet& vertex_attributes() const;

    AttributeSet& primitive_attributes();
    const AttributeSet& primitive_attributes() const;

    AttributeSet& detail_attributes();
    const AttributeSet& detail_attributes() const;

    // Convenience attribute access
    template<typename T>
    bool add_point_attribute(const std::string& name, const T& default_value = T{});

    template<typename T>
    AttributeStorage<T>* get_point_attribute(const std::string& name);

    template<typename T>
    const AttributeStorage<T>* get_point_attribute(const std::string& name) const;

    // Similar methods for vertex, primitive, detail attributes...

    // Standard attributes
    bool ensure_position_attribute();
    bool ensure_normal_attribute();
    bool ensure_color_attribute();
    bool ensure_uv_attribute();

    // Utilities
    void clear();
    GeometryContainer clone() const;
    bool validate() const;
    size_t memory_usage() const;
};

}
```

**Example**:
```cpp
using namespace nodeflux::core;

// Create container
GeometryContainer geo;

// Add position attribute (required)
geo.ensure_position_attribute();

// Add points
size_t p0 = geo.add_point({0.0f, 0.0f, 0.0f});
size_t p1 = geo.add_point({1.0f, 0.0f, 0.0f});
size_t p2 = geo.add_point({0.0f, 1.0f, 0.0f});

// Add custom attribute
geo.add_point_attribute<float>("temperature", 20.0f);
auto* temp_attr = geo.get_point_attribute<float>("temperature");
temp_attr->set(p0, 25.5f);
```

---

### AttributeSet

**Header**: `nodeflux/core/attribute_set.hpp`

Container for all attributes of a specific element class.

```cpp
namespace nodeflux::core {

class AttributeSet {
public:
    // Attribute management
    template<typename T>
    bool add_attribute(const std::string& name, const T& default_value = T{});

    bool remove_attribute(const std::string& name);
    bool has_attribute(const std::string& name) const;

    // Type-safe access
    template<typename T>
    AttributeStorage<T>* get_attribute(const std::string& name);

    template<typename T>
    const AttributeStorage<T>* get_attribute(const std::string& name) const;

    // Type-erased access
    IAttributeStorage* get_storage(std::string_view name);
    const IAttributeStorage* get_storage(std::string_view name) const;

    // Iteration
    std::vector<std::string> attribute_names() const;
    size_t num_attributes() const;

    // Sizing
    void resize(size_t new_size);
    size_t size() const;

    // Utilities
    void clear();
    AttributeSet clone() const;
};

}
```

**Example**:
```cpp
AttributeSet& point_attrs = geo.point_attributes();

// Add various attribute types
point_attrs.add_attribute<float>("mass", 1.0f);
point_attrs.add_attribute<Vec3f>("velocity", {0, 0, 0});
point_attrs.add_attribute<int>("id", 0);

// Check existence
if (point_attrs.has_attribute("mass")) {
    auto* mass = point_attrs.get_attribute<float>("mass");
    // Use mass...
}

// Iterate all attributes
for (const auto& name : point_attrs.attribute_names()) {
    auto* storage = point_attrs.get_storage(name);
    std::cout << name << ": " << storage->descriptor().type_name() << "\n";
}
```

---

### AttributeStorage<T>

**Header**: `nodeflux/core/attribute_storage.hpp`

Type-safe storage for a single attribute's values.

```cpp
namespace nodeflux::core {

template<typename T>
class AttributeStorage : public IAttributeStorage {
public:
    // Element access
    T get(size_t index) const;
    void set(size_t index, const T& value);

    // Span access (fast, zero-copy)
    std::span<const T> get_span() const;
    std::span<T> get_span();

    // Bulk operations
    void fill(const T& value);
    void resize(size_t new_size, const T& fill_value);

    // Metadata
    const AttributeDescriptor& descriptor() const;
    size_t size() const;

    // Data access (advanced)
    const std::vector<T>& data() const;
    std::vector<T>& data();
};

}
```

**Example**:
```cpp
auto* pos = geo.get_point_attribute<Vec3f>("P");

// Element access
Vec3f p = pos->get(0);
pos->set(0, {1.0f, 2.0f, 3.0f});

// Fast span-based access (preferred for loops)
std::span<Vec3f> positions = pos->get_span();
for (size_t i = 0; i < positions.size(); ++i) {
    positions[i].y() += 1.0f;  // Offset all points up
}

// Bulk operations
pos->fill({0, 0, 0});  // Reset all positions
pos->resize(100, {0, 0, 0});  // Resize with default
```

---

## Element Classes

Attributes can be stored at four different element levels:

```cpp
enum class ElementClass {
    POINT,      // Per-point (independent of topology)
    VERTEX,     // Per-vertex (one per primitive corner)
    PRIMITIVE,  // Per-primitive (one per face/curve/etc)
    DETAIL      // Per-geometry (single global value)
};
```

### When to Use Each Class

| Element Class | Use Case | Example Attributes |
|--------------|----------|-------------------|
| `POINT` | Data independent of topology | Position ("P"), velocity, mass |
| `VERTEX` | Data that varies per primitive corner | UVs ("uv"), normals ("N"), colors ("Cd") |
| `PRIMITIVE` | Data shared across entire primitive | Material ID, primitive normal |
| `DETAIL` | Global geometry properties | Bounding box, total mass, metadata |

### Standard Attribute Names

Following Houdini conventions:

| Name | Type | Class | Description |
|------|------|-------|-------------|
| `"P"` | `Vec3f` | Point | Position (required) |
| `"N"` | `Vec3f` | Point/Vertex | Normal vector |
| `"Cd"` | `Vec3f` | Point/Vertex | Color (RGB, 0-1 range) |
| `"uv"` | `Vec2f` | Vertex | UV texture coordinates |
| `"Alpha"` | `float` | Point/Vertex | Opacity (0-1 range) |
| `"v"` | `Vec3f` | Point | Velocity |
| `"id"` | `int` | Point/Primitive | Unique identifier |
| `"pscale"` | `float` | Point | Point scale for instancing |

---

## Attribute Types

Supported types and their C++ mappings:

```cpp
enum class AttributeType {
    FLOAT,      // float
    INT,        // int
    VEC2F,      // Vec2f (2D vector)
    VEC3F,      // Vec3f (3D vector)
    VEC4F,      // Vec4f (4D vector)
    MATRIX3,    // Mat3f (3×3 matrix)
    MATRIX4,    // Mat4f (4×4 matrix)
    QUATERNION, // Quatf (quaternion)
    STRING      // std::string
};
```

### Type Traits

```cpp
// Get default interpolation mode for a type
InterpolationMode default_interpolation(AttributeType type);

// Check if type is numeric
bool is_numeric_type(AttributeType type);

// Get type from C++ type
template<typename T>
constexpr AttributeType type_for = /* ... */;

// Get C++ type size
size_t type_size(AttributeType type);
```

**Example**:
```cpp
// Type dispatch
auto* storage = point_attrs.get_storage("someattr");
switch (storage->descriptor().type()) {
    case AttributeType::FLOAT:
        process_float(static_cast<AttributeStorage<float>*>(storage));
        break;
    case AttributeType::VEC3F:
        process_vec3f(static_cast<AttributeStorage<Vec3f>*>(storage));
        break;
    // ...
}
```

---

## Basic Operations

### Creating Attributes

```cpp
// Direct on container (recommended)
geo.add_point_attribute<Vec3f>("velocity", {0, 0, 0});
geo.add_primitive_attribute<int>("material_id", 0);

// Via AttributeSet
auto& attrs = geo.point_attributes();
attrs.add_attribute<float>("temperature", 20.0f);

// Check before adding
if (!attrs.has_attribute("mass")) {
    attrs.add_attribute<float>("mass", 1.0f);
}
```

### Reading Attributes

```cpp
// Type-safe access (preferred)
auto* pos = geo.get_point_attribute<Vec3f>("P");
if (pos) {
    Vec3f p0 = pos->get(0);
}

// Range-based iteration via span
auto* mass = geo.get_point_attribute<float>("mass");
for (float m : mass->get_span()) {
    std::cout << "Mass: " << m << "\n";
}

// Index-based iteration
std::span<const Vec3f> positions = pos->get_span();
for (size_t i = 0; i < positions.size(); ++i) {
    process_point(i, positions[i]);
}
```

### Modifying Attributes

```cpp
auto* vel = geo.get_point_attribute<Vec3f>("v");

// Single element
vel->set(0, {1.0f, 0.0f, 0.0f});

// Via mutable span (fastest)
std::span<Vec3f> velocities = vel->get_span();
for (auto& v : velocities) {
    v *= 0.99f;  // Apply damping
}

// Bulk fill
vel->fill({0, 0, 0});  // Reset all
```

### Removing Attributes

```cpp
// Remove from container
geo.point_attributes().remove_attribute("temp_data");

// Check first
if (attrs.has_attribute("old_attr")) {
    attrs.remove_attribute("old_attr");
}
```

---

## Advanced Features

### Attribute Promotion/Demotion

**Header**: `nodeflux/core/attribute_promotion.hpp`

Convert attributes between element classes with automatic value aggregation.

```cpp
namespace nodeflux::core {

// Point ↔ Vertex
bool promote_point_to_vertex(
    GeometryContainer& geo,
    const std::string& attr_name,
    const std::string& new_name = ""
);

bool demote_vertex_to_point(
    GeometryContainer& geo,
    const std::string& attr_name,
    const std::string& new_name = ""
);

// Point ↔ Primitive
bool promote_point_to_primitive(
    GeometryContainer& geo,
    const std::string& attr_name,
    const std::string& new_name = ""
);

bool demote_primitive_to_point(
    GeometryContainer& geo,
    const std::string& attr_name,
    const std::string& new_name = ""
);

// Vertex ↔ Primitive
bool promote_vertex_to_primitive(
    GeometryContainer& geo,
    const std::string& attr_name,
    const std::string& new_name = ""
);

bool demote_primitive_to_vertex(
    GeometryContainer& geo,
    const std::string& attr_name,
    const std::string& new_name = ""
);

}
```

**Promotion**: Replicates values to higher-detail level
**Demotion**: Averages values to lower-detail level

**Example**:
```cpp
// Start with per-point color
geo.add_point_attribute<Vec3f>("Cd", {1, 1, 1});

// Promote to per-vertex (replicate to all corners)
promote_point_to_vertex(geo, "Cd");

// Modify vertex colors for hard edges
auto* vertex_color = geo.get_vertex_attribute<Vec3f>("Cd");
vertex_color->set(0, {1, 0, 0});  // Red corner

// Average back to points
demote_vertex_to_point(geo, "Cd");
```

---

### Attribute Groups

**Header**: `nodeflux/core/attribute_group.hpp`

Named selection sets for elements, stored as integer attributes with "group_" prefix.

```cpp
namespace nodeflux::core {

// Group management
bool create_group(
    GeometryContainer& geo,
    ElementClass element_class,
    const std::string& group_name
);

bool delete_group(
    GeometryContainer& geo,
    ElementClass element_class,
    const std::string& group_name
);

bool group_exists(
    const GeometryContainer& geo,
    ElementClass element_class,
    const std::string& group_name
);

// Membership
bool add_to_group(
    GeometryContainer& geo,
    ElementClass element_class,
    const std::string& group_name,
    size_t element_id
);

bool remove_from_group(
    GeometryContainer& geo,
    ElementClass element_class,
    const std::string& group_name,
    size_t element_id
);

bool is_in_group(
    const GeometryContainer& geo,
    ElementClass element_class,
    const std::string& group_name,
    size_t element_id
);

std::vector<size_t> get_group_members(
    const GeometryContainer& geo,
    ElementClass element_class,
    const std::string& group_name
);

// Boolean operations
bool group_union(
    GeometryContainer& geo,
    ElementClass element_class,
    const std::string& group_a,
    const std::string& group_b,
    const std::string& result_group
);

bool group_intersection(/* ... */);
bool group_difference(/* ... */);
bool group_invert(/* ... */);

// Selection
bool select_pattern(
    GeometryContainer& geo,
    ElementClass element_class,
    const std::string& group_name,
    const std::string& pattern  // "0-10", "5,10,15", "0-100:2"
);

bool select_range(/* ... */);

bool select_random(
    GeometryContainer& geo,
    ElementClass element_class,
    const std::string& group_name,
    float percentage,  // 0.0 - 1.0
    unsigned int seed = 0
);

template<typename T>
bool select_by_attribute(
    GeometryContainer& geo,
    ElementClass element_class,
    const std::string& group_name,
    const std::string& attr_name,
    std::function<bool(const T&)> predicate
);

}
```

**Example**:
```cpp
// Create groups
create_group(geo, ElementClass::POINT, "top_points");
create_group(geo, ElementClass::PRIMITIVE, "red_faces");

// Add elements manually
add_to_group(geo, ElementClass::POINT, "top_points", 5);
add_to_group(geo, ElementClass::POINT, "top_points", 10);

// Select by pattern
select_pattern(geo, ElementClass::POINT, "evens", "0-100:2");  // Every 2nd

// Select by attribute
auto* pos = geo.get_point_attribute<Vec3f>("P");
select_by_attribute<Vec3f>(
    geo, ElementClass::POINT, "top_points", "P",
    [](const Vec3f& p) { return p.y() > 5.0f; }
);

// Boolean ops
group_union(geo, ElementClass::POINT, "group_a", "group_b", "combined");

// Iterate group members
for (size_t pid : get_group_members(geo, ElementClass::POINT, "top_points")) {
    process_point(pid);
}
```

---

### Attribute Interpolation

**Header**: `nodeflux/core/attribute_interpolation.hpp`

Advanced blending and sampling of attributes.

```cpp
namespace nodeflux::core {

enum class InterpolationMode {
    LINEAR,      // Linear blend
    CUBIC,       // Cubic (smooth) blend
    WEIGHTED,    // Weighted average
    BARYCENTRIC, // Triangle barycentric
    BILINEAR,    // Quad bilinear
    SLERP        // Spherical linear (for rotations)
};

// Basic interpolation
template<typename T>
T interpolate_linear(const T& a, const T& b, float t);

template<typename T>
T interpolate_cubic(const T& a, const T& b, float t);

template<typename T>
T interpolate_weighted(
    const std::vector<T>& values,
    const std::vector<float>& weights
);

template<typename T>
T interpolate_barycentric(
    const T& v0, const T& v1, const T& v2,
    float u, float v
);

template<typename T>
T interpolate_bilinear(
    const T& v00, const T& v10, const T& v01, const T& v11,
    float u, float v
);

// Specialized interpolation
Vec3f slerp(const Vec3f& a, const Vec3f& b, float t);
Vec3f interpolate_normal(const Vec3f& n0, const Vec3f& n1, float t);
Vec3f interpolate_color(const Vec3f& c0, const Vec3f& c1, float t, bool linearize = false);

template<typename T>
T interpolate_clamped(const T& a, const T& b, float t);

// Attribute blending
template<typename T>
bool blend_attributes(
    GeometryContainer& geo,
    ElementClass element_class,
    const std::string& attr_name,
    const std::vector<size_t>& source_elements,
    size_t target_element,
    const std::vector<float>& weights
);

// Bulk operations
bool copy_and_interpolate_all_attributes(
    GeometryContainer& geo,
    ElementClass element_class,
    const std::vector<size_t>& source_elements,
    size_t target_element,
    const std::vector<float>& weights
);

template<typename T>
bool resample_curve_attribute(
    GeometryContainer& geo,
    const std::string& attr_name,
    size_t num_samples,
    InterpolationMode mode = InterpolationMode::LINEAR
);

// Helpers
float smoothstep(float t);
float smootherstep(float t);
float saturate(float t);

}
```

**Example**:
```cpp
// Linear interpolation
Vec3f p0 = {0, 0, 0};
Vec3f p1 = {10, 10, 10};
Vec3f mid = interpolate_linear(p0, p1, 0.5f);  // {5, 5, 5}

// Blend multiple sources with weights
std::vector<size_t> sources = {0, 1, 2, 3};
std::vector<float> weights = {0.4f, 0.3f, 0.2f, 0.1f};
blend_attributes<Vec3f>(geo, ElementClass::POINT, "Cd", sources, target, weights);

// Copy all attributes with interpolation
copy_and_interpolate_all_attributes(geo, ElementClass::POINT, sources, target, weights);

// Barycentric for triangles
Vec3f v0 = {0, 0, 0};
Vec3f v1 = {1, 0, 0};
Vec3f v2 = {0, 1, 0};
Vec3f center = interpolate_barycentric(v0, v1, v2, 0.33f, 0.33f);

// Smooth interpolation
float t_smooth = smootherstep(0.5f);  // Smooth ease in/out
```

---

## Best Practices

### Naming Conventions

✅ **DO**:
- Use Houdini standard names: "P", "N", "Cd", "uv"
- Use lowercase for custom attributes: "density", "temperature"
- Use descriptive names: "material_id", "uv_seam"

❌ **DON'T**:
- Use CamelCase or PascalCase for attributes
- Use generic names like "data", "value", "temp"
- Start names with numbers or special characters

### Performance Tips

1. **Use spans for bulk access**:
   ```cpp
   // ✅ Fast - direct memory access
   std::span<Vec3f> positions = pos->get_span();
   for (auto& p : positions) {
       p *= 2.0f;
   }

   // ❌ Slow - virtual function calls
   for (size_t i = 0; i < pos->size(); ++i) {
       pos->set(i, pos->get(i) * 2.0f);
   }
   ```

2. **Minimize attribute lookups**:
   ```cpp
   // ✅ Look up once
   auto* attr = geo.get_point_attribute<float>("mass");
   for (size_t i = 0; i < geo.num_points(); ++i) {
       process(attr->get(i));
   }

   // ❌ Look up every iteration
   for (size_t i = 0; i < geo.num_points(); ++i) {
       auto* attr = geo.get_point_attribute<float>("mass");
       process(attr->get(i));
   }
   ```

3. **Reserve capacity**:
   ```cpp
   geo.set_num_points(1000);  // Pre-allocate
   geo.ensure_position_attribute();
   // Now add points without reallocation
   ```

4. **Use appropriate element class**:
   - Point attributes for position-based data
   - Vertex attributes only when corner values differ
   - Primitive attributes for face-wide properties

### Memory Management

```cpp
// Check memory usage
size_t bytes = geo.memory_usage();
std::cout << "Geometry uses " << (bytes / 1024 / 1024) << " MB\n";

// Clean up unused attributes
for (const auto& name : geo.point_attributes().attribute_names()) {
    if (name.starts_with("temp_")) {
        geo.point_attributes().remove_attribute(name);
    }
}

// Clear all data
geo.clear();
```

### Error Handling

```cpp
// Always check attribute existence
auto* attr = geo.get_point_attribute<float>("density");
if (!attr) {
    // Attribute doesn't exist or wrong type
    return false;
}

// Validate before use
if (!geo.validate()) {
    // Geometry has topology errors
    std::cerr << "Invalid geometry detected\n";
    return false;
}

// Check sizes match
if (attr->size() != geo.num_points()) {
    std::cerr << "Attribute size mismatch\n";
    return false;
}
```

---

## Performance Notes

### Benchmark Results (1M Vec3f attributes)

| Operation | Time | Throughput |
|-----------|------|------------|
| Sequential write | 29ms | 34.5 M/s |
| Sequential read | 97ms | 10.3 M/s |
| Random access | ~300ms | 3.3 M/s |

### Memory Layout

The attribute system uses **Structure-of-Arrays (SoA)** layout:

```cpp
// SoA (used by NodeFluxEngine) - cache-friendly
struct PointData {
    std::vector<float> x_coords;  // [x0, x1, x2, ...]
    std::vector<float> y_coords;  // [y0, y1, y2, ...]
    std::vector<float> z_coords;  // [z0, z1, z2, ...]
};

// AoS (traditional) - poor cache usage
struct PointData {
    std::vector<Point> points;  // [{x0,y0,z0}, {x1,y1,z1}, ...]
};
```

**Benefits**:
- ✅ Better cache utilization when processing single attributes
- ✅ SIMD-friendly memory access patterns
- ✅ Efficient for GPU upload (contiguous buffers)

### Optimization Checklist

- [ ] Use `std::span` for attribute access in loops
- [ ] Pre-allocate geometry size with `set_num_points()`
- [ ] Cache attribute pointers outside loops
- [ ] Use appropriate element class (fewer vertices = better)
- [ ] Remove temporary attributes after use
- [ ] Consider promotion/demotion for data reduction
- [ ] Use groups for selective processing
- [ ] Profile before optimizing (use `benchmark_attributes`)

---

## See Also

- [Migration Guide](ATTRIBUTE_MIGRATION_GUIDE.md) - Upgrading from old system
- [Usage Examples](ATTRIBUTE_EXAMPLES.md) - Practical code examples
- [Implementation Guide](attribute_system_implementation_guide.md) - Internal details
- [Requirements](attribute_system_requirements.md) - Original design spec

---

**Last Updated**: October 26, 2025
**Maintainer**: NodeFluxEngine Team
