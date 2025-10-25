# Attribute System Migration Guide

**From**: `GeometryAttributes` (legacy)
**To**: `GeometryContainer` + `AttributeSet` (v2.0)
**Status**: Migration Complete ✅

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Key Differences](#key-differences)
3. [Common Patterns](#common-patterns)
4. [API Mapping](#api-mapping)
5. [Migration Checklist](#migration-checklist)
6. [Troubleshooting](#troubleshooting)

---

## Quick Start

### Old System (GeometryAttributes)

```cpp
// Old way
GeometryAttributes geo;
geo.positions = {
    {0.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f}
};
geo.vertex_attributes["uv"] = std::vector<Vec2f>{{0,0}, {1,0}, {0,1}};
```

### New System (GeometryContainer)

```cpp
// New way
GeometryContainer geo;
geo.ensure_position_attribute();

geo.add_point({0.0f, 0.0f, 0.0f});
geo.add_point({1.0f, 0.0f, 0.0f});
geo.add_point({0.0f, 1.0f, 0.0f});

geo.add_vertex_attribute<Vec2f>("uv", {0, 0});
auto* uv = geo.get_vertex_attribute<Vec2f>("uv");
uv->set(0, {0, 0});
uv->set(1, {1, 0});
uv->set(2, {0, 1});
```

---

## Key Differences

### Architecture Changes

| Aspect | Old System | New System |
|--------|-----------|------------|
| **Storage** | Direct member variables | Unified AttributeSet containers |
| **Type Safety** | Runtime variant types | Compile-time + runtime checking |
| **Memory Layout** | Mixed AoS/SoA | Pure Structure-of-Arrays (SoA) |
| **Naming** | Mixed conventions | Houdini-standard ("P", "N", "Cd") |
| **Element Classes** | 3 types (vertex, face, global) | 4 types (point, vertex, primitive, detail) |
| **Performance** | Moderate | Optimized for cache/SIMD |

### Conceptual Differences

#### 1. Points vs Vertices

**Old System**: Conflated points and vertices
```cpp
geo.positions[i];  // Unclear: point or vertex?
```

**New System**: Clear separation
```cpp
geo.get_point_position(point_id);      // Topology-independent
geo.get_vertex_attribute<T>("attr");    // Per-primitive-corner
```

#### 2. Attribute Access

**Old**: Direct container access
```cpp
geo.vertex_attributes["color"][i] = red;
```

**New**: Type-safe accessors
```cpp
auto* color = geo.get_vertex_attribute<Vec3f>("Cd");
color->set(i, red);
```

#### 3. Standard Names

**Old**: Inconsistent naming
```cpp
geo.positions
geo.normals
geo.vertex_attributes["color"]
```

**New**: Houdini conventions
```cpp
auto* P = geo.get_point_attribute<Vec3f>("P");     // Position
auto* N = geo.get_point_attribute<Vec3f>("N");     // Normal
auto* Cd = geo.get_vertex_attribute<Vec3f>("Cd");  // Color
```

---

## Common Patterns

### Pattern 1: Creating Geometry

#### Old Way
```cpp
void create_triangle_old(GeometryAttributes& geo) {
    geo.positions = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };

    geo.faces.push_back({3});  // Triangle

    geo.vertex_attributes["color"] = std::vector<Vec3f>{
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };
}
```

#### New Way
```cpp
void create_triangle_new(GeometryContainer& geo) {
    // Ensure position attribute exists
    geo.ensure_position_attribute();

    // Add points
    size_t p0 = geo.add_point({0.0f, 0.0f, 0.0f});
    size_t p1 = geo.add_point({1.0f, 0.0f, 0.0f});
    size_t p2 = geo.add_point({0.0f, 1.0f, 0.0f});

    // Add primitive (creates vertices automatically)
    std::vector<int> vertex_list = {
        static_cast<int>(p0),
        static_cast<int>(p1),
        static_cast<int>(p2)
    };
    geo.add_primitive(vertex_list);

    // Add vertex colors
    geo.add_vertex_attribute<Vec3f>("Cd", {1, 1, 1});
    auto* Cd = geo.get_vertex_attribute<Vec3f>("Cd");
    Cd->set(0, {1, 0, 0});  // Red
    Cd->set(1, {0, 1, 0});  // Green
    Cd->set(2, {0, 0, 1});  // Blue
}
```

### Pattern 2: Iterating Positions

#### Old Way
```cpp
void offset_positions_old(GeometryAttributes& geo, Vec3f offset) {
    for (auto& pos : geo.positions) {
        pos += offset;
    }
}
```

#### New Way (Multiple Options)
```cpp
// Option A: Via accessor (element-by-element)
void offset_positions_new_a(GeometryContainer& geo, Vec3f offset) {
    for (size_t i = 0; i < geo.num_points(); ++i) {
        Vec3f pos = geo.get_point_position(i);
        geo.set_point_position(i, pos + offset);
    }
}

// Option B: Via span (preferred - fastest)
void offset_positions_new_b(GeometryContainer& geo, Vec3f offset) {
    auto* P = geo.get_point_attribute<Vec3f>("P");
    std::span<Vec3f> positions = P->get_span();

    for (auto& pos : positions) {
        pos += offset;
    }
}

// Option C: Via direct data access (expert)
void offset_positions_new_c(GeometryContainer& geo, Vec3f offset) {
    auto* P = geo.get_point_attribute<Vec3f>("P");
    std::vector<Vec3f>& positions = P->data();

    for (auto& pos : positions) {
        pos += offset;
    }
}
```

**Recommendation**: Use Option B (span-based) for best performance.

### Pattern 3: Custom Attributes

#### Old Way
```cpp
void add_custom_old(GeometryAttributes& geo) {
    // Hope the type matches!
    geo.vertex_attributes["temperature"] = std::vector<float>(
        geo.positions.size(), 20.0f
    );

    // Later... runtime error if type wrong
    auto& temps = std::get<std::vector<float>>(
        geo.vertex_attributes["temperature"]
    );
    temps[0] = 25.0f;
}
```

#### New Way
```cpp
void add_custom_new(GeometryContainer& geo) {
    // Type-safe creation
    geo.add_point_attribute<float>("temperature", 20.0f);

    // Type-safe access
    auto* temp = geo.get_point_attribute<float>("temperature");
    if (temp) {  // nullptr if wrong type or doesn't exist
        temp->set(0, 25.0f);
    }

    // Alternative: span access
    std::span<float> temps = temp->get_span();
    temps[0] = 25.0f;
}
```

### Pattern 4: Checking Attribute Existence

#### Old Way
```cpp
bool has_normals_old(const GeometryAttributes& geo) {
    return !geo.normals.empty();
}

bool has_custom_old(const GeometryAttributes& geo, const std::string& name) {
    return geo.vertex_attributes.find(name) != geo.vertex_attributes.end();
}
```

#### New Way
```cpp
bool has_normals_new(const GeometryContainer& geo) {
    return geo.point_attributes().has_attribute("N");
}

bool has_custom_new(const GeometryContainer& geo,
                    ElementClass element_class,
                    const std::string& name) {
    switch (element_class) {
        case ElementClass::POINT:
            return geo.point_attributes().has_attribute(name);
        case ElementClass::VERTEX:
            return geo.vertex_attributes().has_attribute(name);
        case ElementClass::PRIMITIVE:
            return geo.primitive_attributes().has_attribute(name);
        case ElementClass::DETAIL:
            return geo.detail_attributes().has_attribute(name);
    }
    return false;
}

// Type-aware check
template<typename T>
bool has_attribute_typed(const GeometryContainer& geo,
                         const std::string& name) {
    auto* attr = geo.get_point_attribute<T>(name);
    return attr != nullptr;
}
```

### Pattern 5: Copying Geometry

#### Old Way
```cpp
GeometryAttributes copy_old(const GeometryAttributes& source) {
    GeometryAttributes dest;
    dest.positions = source.positions;
    dest.normals = source.normals;
    dest.faces = source.faces;
    dest.vertex_attributes = source.vertex_attributes;
    // ... copy all members manually
    return dest;
}
```

#### New Way
```cpp
GeometryContainer copy_new(const GeometryContainer& source) {
    // Single method handles everything
    return source.clone();
}
```

### Pattern 6: Clearing Geometry

#### Old Way
```cpp
void clear_old(GeometryAttributes& geo) {
    geo.positions.clear();
    geo.normals.clear();
    geo.faces.clear();
    geo.vertex_attributes.clear();
    geo.face_attributes.clear();
    geo.global_attributes.clear();
}
```

#### New Way
```cpp
void clear_new(GeometryContainer& geo) {
    geo.clear();  // Clears everything
}
```

---

## API Mapping

### Core Operations

| Old API | New API | Notes |
|---------|---------|-------|
| `geo.positions[i]` | `geo.get_point_position(i)` | Getter |
| `geo.positions[i] = p` | `geo.set_point_position(i, p)` | Setter |
| `geo.positions.size()` | `geo.num_points()` | Count |
| `geo.normals[i]` | `geo.get_point_attribute<Vec3f>("N")->get(i)` | Normal access |
| `geo.faces[i]` | `geo.get_primitive_vertices(i)` | Returns span |
| `geo.faces.size()` | `geo.num_primitives()` | Count |

### Attribute Operations

| Old API | New API | Notes |
|---------|---------|-------|
| `geo.vertex_attributes["color"]` | `geo.get_vertex_attribute<Vec3f>("Cd")` | Type-safe |
| `geo.face_attributes["id"]` | `geo.get_primitive_attribute<int>("id")` | Renamed |
| `geo.global_attributes["name"]` | `geo.get_detail_attribute<std::string>("name")` | Renamed |
| `attr.emplace(name, vector)` | `geo.add_point_attribute<T>(name, default)` | Creation |
| `attr.erase(name)` | `geo.point_attributes().remove_attribute(name)` | Deletion |
| `attr.find(name) != end` | `geo.point_attributes().has_attribute(name)` | Existence |

### Utility Operations

| Old API | New API | Notes |
|---------|---------|-------|
| N/A | `geo.ensure_position_attribute()` | Guarantee "P" exists |
| N/A | `geo.ensure_normal_attribute()` | Guarantee "N" exists |
| N/A | `geo.validate()` | Check topology validity |
| N/A | `geo.memory_usage()` | Get memory footprint |
| N/A | `geo.clone()` | Deep copy |

---

## Migration Checklist

### Step 1: Update Includes

```cpp
// Remove old includes
// #include <nodeflux/sop/geometry_attributes.hpp>

// Add new includes
#include <nodeflux/core/geometry_container.hpp>
#include <nodeflux/core/attribute_types.hpp>
```

### Step 2: Update Type Names

- [ ] `GeometryAttributes` → `GeometryContainer`
- [ ] `vertex_attributes` → element class specific
- [ ] `face_attributes` → `primitive_attributes`
- [ ] `global_attributes` → `detail_attributes`

### Step 3: Convert Direct Access

Replace direct member access:

```cpp
// Before
geo.positions[i] = {x, y, z};

// After
geo.set_point_position(i, {x, y, z});

// Or for bulk operations
auto* P = geo.get_point_attribute<Vec3f>("P");
P->get_span()[i] = {x, y, z};
```

### Step 4: Update Attribute Names

Adopt Houdini conventions:

- [ ] `"color"` → `"Cd"`
- [ ] `"normal"` → `"N"`
- [ ] `"position"` → `"P"`
- [ ] `"texcoord"` → `"uv"`
- [ ] `"opacity"` → `"Alpha"`

### Step 5: Add ensure_*_attribute() Calls

```cpp
// At start of cook() methods
geo.ensure_position_attribute();

// For normals if used
if (needs_normals) {
    geo.ensure_normal_attribute();
}
```

### Step 6: Use Type-Safe Access

```cpp
// Before (unsafe)
auto& colors = std::get<std::vector<Vec3f>>(geo.vertex_attributes["color"]);

// After (safe)
auto* Cd = geo.get_vertex_attribute<Vec3f>("Cd");
if (!Cd) {
    // Handle missing/wrong type
    return false;
}
std::span<Vec3f> colors = Cd->get_span();
```

### Step 7: Update Iteration Patterns

Prefer span-based iteration:

```cpp
// Before
for (size_t i = 0; i < geo.positions.size(); ++i) {
    process(geo.positions[i]);
}

// After (preferred)
auto* P = geo.get_point_attribute<Vec3f>("P");
for (const Vec3f& pos : P->get_span()) {
    process(pos);
}

// Or with indices
std::span<const Vec3f> positions = P->get_span();
for (size_t i = 0; i < positions.size(); ++i) {
    process(positions[i]);
}
```

### Step 8: Test Thoroughly

- [ ] Unit tests pass
- [ ] Visual output matches old system
- [ ] Performance is acceptable
- [ ] Memory usage is reasonable
- [ ] No crashes or errors

---

## Troubleshooting

### Issue: Attribute doesn't exist

**Symptom**: `get_*_attribute()` returns `nullptr`

**Solutions**:
```cpp
// 1. Check if it exists
if (!geo.point_attributes().has_attribute("myattr")) {
    geo.add_point_attribute<float>("myattr", 0.0f);
}

// 2. Ensure standard attributes
geo.ensure_position_attribute();  // "P"
geo.ensure_normal_attribute();    // "N"

// 3. Check element class (point vs vertex vs primitive)
auto* attr_p = geo.get_point_attribute<float>("density");
auto* attr_v = geo.get_vertex_attribute<float>("density");
auto* attr_prim = geo.get_primitive_attribute<float>("density");
```

### Issue: Type mismatch

**Symptom**: Template access returns `nullptr` but attribute exists

**Cause**: Requesting wrong type (e.g., `float` when it's `Vec3f`)

**Solution**:
```cpp
// Check type dynamically
const auto* storage = geo.point_attributes().get_storage("myattr");
if (storage) {
    auto type = storage->descriptor().type();
    switch (type) {
        case AttributeType::FLOAT:
            process_float(geo.get_point_attribute<float>("myattr"));
            break;
        case AttributeType::VEC3F:
            process_vec3f(geo.get_point_attribute<Vec3f>("myattr"));
            break;
        // ...
    }
}
```

### Issue: Size mismatch

**Symptom**: Attribute has wrong number of elements

**Cause**: Element class confusion or forgot to resize

**Solution**:
```cpp
// Attributes automatically resize when elements added
geo.set_num_points(100);  // Resizes all point attributes

// Verify sizes
auto* attr = geo.get_point_attribute<float>("mass");
assert(attr->size() == geo.num_points());

// Manual resize if needed
attr->resize(new_size, default_value);
```

### Issue: Performance regression

**Symptom**: New code slower than old code

**Solutions**:
```cpp
// 1. Use span-based access (fastest)
std::span<Vec3f> pos = P->get_span();
for (auto& p : pos) {
    p *= scale;  // Direct memory access
}

// 2. Cache attribute pointers
// BAD: lookup every iteration
for (size_t i = 0; i < n; ++i) {
    auto* attr = geo.get_point_attribute<float>("mass");  // SLOW!
    process(attr->get(i));
}

// GOOD: lookup once
auto* mass = geo.get_point_attribute<float>("mass");
for (size_t i = 0; i < n; ++i) {
    process(mass->get(i));
}

// 3. Use direct data() access for expert use
std::vector<Vec3f>& positions = P->data();
// ... bulk operations on vector
```

### Issue: Missing vertices

**Symptom**: `num_vertices() == 0` but primitives exist

**Cause**: Old system didn't track vertices separately

**Solution**:
```cpp
// Vertices are created automatically when adding primitives
std::vector<int> verts = {0, 1, 2};  // Point indices
geo.add_primitive(verts);  // Creates 3 vertices

// Access vertex attributes by linear index
auto* Cd = geo.get_vertex_attribute<Vec3f>("Cd");
Cd->set(0, red);   // First vertex
Cd->set(1, green); // Second vertex
Cd->set(2, blue);  // Third vertex
```

### Issue: Can't find "P" attribute

**Symptom**: Geometry seems valid but "P" doesn't exist

**Solution**:
```cpp
// Always ensure position exists
geo.ensure_position_attribute();

// Or check first
if (!geo.point_attributes().has_attribute("P")) {
    geo.ensure_position_attribute();
}
```

### Issue: Groups not working

**Symptom**: Group functions return false

**Solution**:
```cpp
// Must create group before using
create_group(geo, ElementClass::POINT, "selection");

// Then add members
add_to_group(geo, ElementClass::POINT, "selection", point_id);

// Check if group exists
if (group_exists(geo, ElementClass::POINT, "selection")) {
    // Use group...
}
```

---

## SOP Node Migration Example

### Complete Before/After

#### Old SOP Node
```cpp
class MyOldSOP : public SOPNode {
public:
    std::optional<GeometryData> cook() override {
        GeometryData result;

        // Create triangle
        result.mesh.positions = {
            {0, 0, 0}, {1, 0, 0}, {0, 1, 0}
        };
        result.mesh.faces = {{3}};

        // Add colors
        result.mesh.vertex_attributes["color"] = std::vector<Vec3f>{
            {1,0,0}, {0,1,0}, {0,0,1}
        };

        return result;
    }
};
```

#### New SOP Node
```cpp
class MyNewSOP : public SOPNode {
public:
    std::optional<GeometryData> cook() override {
        GeometryData result;
        GeometryContainer& geo = result.container;

        // Ensure position attribute
        geo.ensure_position_attribute();

        // Create triangle
        size_t p0 = geo.add_point({0, 0, 0});
        size_t p1 = geo.add_point({1, 0, 0});
        size_t p2 = geo.add_point({0, 1, 0});

        std::vector<int> verts = {
            static_cast<int>(p0),
            static_cast<int>(p1),
            static_cast<int>(p2)
        };
        geo.add_primitive(verts);

        // Add vertex colors (Houdini-standard name)
        geo.add_vertex_attribute<Vec3f>("Cd", {1, 1, 1});
        auto* Cd = geo.get_vertex_attribute<Vec3f>("Cd");
        Cd->set(0, {1, 0, 0});  // Red
        Cd->set(1, {0, 1, 0});  // Green
        Cd->set(2, {0, 0, 1});  // Blue

        return result;
    }
};
```

---

## Advanced Migration Topics

### Using Attribute Promotion

Convert between element classes:

```cpp
// Had per-point colors, need per-vertex for hard edges
promote_point_to_vertex(geo, "Cd");

// Average vertex colors back to points
demote_vertex_to_point(geo, "Cd");

// Promote point attribute to primitives (average)
promote_point_to_primitive(geo, "density");
```

### Using Attribute Groups

Migrate selection sets:

```cpp
// Old: stored as attribute
std::vector<int> selection = {0, 5, 10, 15};
geo.global_attributes["selection"] = selection;

// New: use groups
create_group(geo, ElementClass::POINT, "selection");
for (int id : {0, 5, 10, 15}) {
    add_to_group(geo, ElementClass::POINT, "selection", id);
}

// Process group members
for (size_t pid : get_group_members(geo, ElementClass::POINT, "selection")) {
    process_selected_point(pid);
}
```

### Using Interpolation

Blend attributes smoothly:

```cpp
// Old: manual blending
Vec3f blended = 0.5f * color_a + 0.5f * color_b;

// New: use interpolation utilities
Vec3f blended = interpolate_linear(color_a, color_b, 0.5f);

// Smooth interpolation
Vec3f smooth = interpolate_cubic(color_a, color_b, smootherstep(t));

// Weighted average from multiple sources
std::vector<Vec3f> colors = {red, green, blue};
std::vector<float> weights = {0.5f, 0.3f, 0.2f};
Vec3f result = interpolate_weighted(colors, weights);
```

---

## Next Steps

1. ✅ Read this migration guide
2. ✅ Study the [API Reference](ATTRIBUTE_SYSTEM_API.md)
3. ✅ Review [Usage Examples](ATTRIBUTE_EXAMPLES.md)
4. ✅ Update your code following the checklist
5. ✅ Test thoroughly
6. ✅ Enjoy the improved performance! 🚀

---

**Questions?** Check the [API documentation](ATTRIBUTE_SYSTEM_API.md) or review existing migrated SOPs in `nodeflux_core/src/sop/`.

**Last Updated**: October 26, 2025
**Migration Complete**: All 18 SOPs migrated ✅
