# Attribute System Requirements Analysis
**Future-Proofing for Production Procedural Modeling**

---

## 🎯 Goal
Build an attribute system that won't need fundamental redesign as we add:
- Complex procedural workflows
- Performance optimizations (multi-threading, GPU)
- Advanced features (wrangle, volumes, simulation)
- Large-scale geometry (millions of points)
- Real-time interactivity

---

## 1. Data Type Requirements

### 1.1 What Types Do We NEED?

**Currently Supported**:
- ✅ `float` - Scalars
- ✅ `int` - Integers
- ✅ `Vector3` (double) / `Vector3f` (float) - 3D vectors
- ✅ `Vector2f` - 2D vectors (UVs)
- ✅ `string` - Metadata

**MISSING - Critical for Production**:

#### A) **Vector4** (RGBA, Quaternions)
```cpp
// Use cases:
- RGBA colors with alpha
- Quaternion rotations (more stable than euler angles)
- Homogeneous coordinates (w component)
```
**Priority**: 🔥 **HIGH** - Needed for:
- Proper color with transparency
- Rotation interpolation in animation
- Instancing with orientations

#### B) **Matrix3 & Matrix4** (Transforms)
```cpp
// Use cases:
- Per-point/primitive transforms
- Instance transformations
- Coordinate system conversions
- Tangent space calculations
```
**Priority**: 🔥 **HIGH** - Needed for:
- Copy to points with full transforms
- Normal mapping
- Coordinate frames

#### C) **Integer Variants** (int8, int16, int32, int64, uint variants)
```cpp
// Use cases:
- Memory optimization (int8 for small IDs)
- Large indices (int64 for huge meshes)
- Flags and bitmasks (uint32)
```
**Priority**: 🟡 **MEDIUM** - Optimization, not critical

#### D) **Array/List Types** (Variable-length per element)
```cpp
// Use cases:
- Variable-length primitive vertex lists (polygons with N sides)
- Per-point neighbor lists
- Per-primitive material assignments (multi-material)
```
**Priority**: 🔥 **HIGH** - Needed for:
- N-gons (faces with arbitrary vertex count)
- Packed primitives
- Adjacency queries

#### E) **Dictionary/Map Types** (Key-value pairs)
```cpp
// Use cases:
- Arbitrary metadata (e.g., {"author": "John", "date": "2025"})
- Material property bags
- Custom user data
```
**Priority**: 🟢 **LOW** - Nice to have, can use multiple attributes

#### F) **Custom Struct Types** (User-defined)
```cpp
// Use cases:
- Complex data structures (e.g., BVH nodes)
- Particle system data (position + velocity + lifetime + ...)
- Shader parameters
```
**Priority**: 🟡 **MEDIUM** - Future extensibility

---

### 1.2 Type System Design

**Option A: Fixed Enum (Current)**
```cpp
enum class AttributeType {
    FLOAT, INT, VECTOR2, VECTOR3, VECTOR4,
    MATRIX3, MATRIX4, STRING, ...
};
```
✅ Simple, fast, type-safe
❌ Can't add new types without recompiling
❌ No user-defined types

**Option B: Type Registry (Extensible)**
```cpp
class TypeDescriptor {
    std::string name;
    size_t byte_size;
    std::function<void(void*, const void*)> copy_fn;
    std::function<void(void*, const void*, float)> lerp_fn;
    // etc.
};

// Users can register custom types
TypeRegistry::register_type<MyCustomStruct>("MyStruct", ...);
```
✅ Extensible at runtime
✅ Supports custom types
❌ More complex, slower
❌ Type safety is harder

**Recommendation**: **Hybrid**
- Built-in types (float, int, vec2/3/4, matrix) as enum (fast path)
- Optional type registry for custom types (slow path)
- 95% of attributes use built-in types → optimized
- 5% use custom types → flexibility

---

## 2. Element Model & Topology

### 2.1 Current Confusion: Points vs Vertices

**Problem**: "Vertex" is overloaded
- In computer graphics: "vertex" = point with attributes
- In topology: "vertex" = corner of a face (references a point)

**Houdini's Model** (correct):
- **Point** - Unique position in space (shared)
- **Vertex** - Corner of a primitive (references a point + has unique attributes)
- **Primitive** - Face, curve, volume, etc.
- **Detail** - Global/geometry-level

**Example**:
```
A cube has:
- 8 points (corners)
- 24 vertices (6 faces × 4 corners each)
- 6 primitives (faces)

Why 24 vertices? Because each point is referenced multiple times,
and each reference can have different normals/UVs (split normals).
```

**What We Need**:
```cpp
struct ElementTopology {
    size_t point_count;        // Unique positions
    size_t vertex_count;       // Point references in primitives
    size_t primitive_count;    // Faces/curves/volumes

    // Vertex → Point mapping
    std::vector<int> vertex_point;  // vertex[i] -> point index

    // Primitive → Vertex mapping (variable length!)
    std::vector<int> primitive_vertex_start;  // Start index in vertex array
    std::vector<int> primitive_vertex_count;  // How many vertices
    // OR: std::vector<std::vector<int>> (simpler but less cache-friendly)
};
```

**Priority**: 🔥 **CRITICAL** - Foundation for everything

---

### 2.2 Primitive Types

**Current**: Only triangle meshes

**Need to Support**:
- **Polygons** (N-sided faces) - quads, pentagons, etc.
- **Curves** (polylines, NURBS, bezier)
- **Volumes** (voxel grids)
- **Packed primitives** (instanced geometry)
- **Metaballs/Implicit surfaces**
- **Point clouds** (no connectivity)

**Design**:
```cpp
enum class PrimitiveType {
    POLYGON,     // Variable vertex count
    BEZIER_CURVE,
    NURBS_CURVE,
    VOLUME,
    PACKED_PRIM,
    METABALL,
};

struct PrimitiveDescriptor {
    PrimitiveType type;
    int vertex_start;   // Index into vertex array
    int vertex_count;   // How many vertices

    // Type-specific data
    union {
        struct { /* polygon data */ } polygon;
        struct { /* curve data */ } curve;
        // etc.
    };
};
```

**Priority**: 🟡 **MEDIUM** - Start with polygons, add others later

---

## 3. Performance Considerations

### 3.1 Memory Layout (Critical!)

**Current**: `std::vector<std::variant<float, int, Vector3, ...>>`
❌ Each element stores type tag (8+ bytes overhead)
❌ Non-contiguous data (cache-unfriendly)
❌ Type checking on every access

**Target**: Structure of Arrays (SoA)
```cpp
// Instead of:
std::vector<Variant> positions;  // [Variant(Vec3), Variant(Vec3), ...]

// Use:
std::vector<Vector3> positions;  // [Vec3, Vec3, Vec3, ...] - contiguous!
```

✅ No per-element overhead
✅ Cache-friendly iteration
✅ SIMD-friendly (can vectorize)
✅ GPU-friendly (direct upload)

**Implementation**:
```cpp
template<typename T>
class TypedAttributeStorage {
    std::vector<T> data_;  // Contiguous array

    // Fast iteration
    T* begin() { return data_.data(); }
    T* end() { return data_.data() + data_.size(); }
};

// Type-erased interface for storage
class IAttributeStorage {
    virtual void* raw_data() = 0;
    virtual size_t element_size() const = 0;
    // ...
};
```

**Priority**: 🔥 **CRITICAL** - 10-100x performance difference

---

### 3.2 Sparse Attributes

**Problem**: Not every point needs every attribute

Example: Only 10% of points have "temperature" attribute
- Dense storage: Allocate for ALL points (90% wasted)
- Sparse storage: Only store where needed

**Options**:

**A) Dense (Current)**
```cpp
std::vector<float> temperature(point_count);  // Always full size
```
✅ O(1) access
❌ Wastes memory
❌ Slow iteration over sparse data

**B) Sparse Map**
```cpp
std::unordered_map<int, float> temperature;  // Only store non-default
```
✅ Memory efficient
❌ O(log n) or O(1) but with overhead
❌ Not cache-friendly

**C) Hybrid (Best)**
```cpp
class SparseAttributeStorage {
    std::vector<float> data_;       // Compact storage
    std::vector<int> indices_;      // Which elements have values

    // OR: Use bitmask + compact array
    std::vector<uint64_t> presence_mask_;  // Bit per element
    std::vector<float> compact_data_;
};
```
✅ Memory efficient
✅ Fast iteration over present elements
⚠️ More complex

**When to use sparse**:
- Attribute is used by <20% of elements
- Large geometry (>1M elements)
- Memory-constrained environments

**Priority**: 🟢 **LOW** - Optimization, start with dense

---

### 3.3 Hot Path Optimization

**Critical code paths** (must be FAST):
1. Attribute access in node loops
2. Attribute interpolation (subdivision, resampling)
3. Attribute transfer (boolean ops, merging)

**Requirements**:
```cpp
// Must be zero-cost abstraction
for (auto& pos : geo->get_positions()) {  // No overhead!
    pos += offset;
}

// Should compile to same as:
Vector3* positions = geo->positions_raw();
for (size_t i = 0; i < count; ++i) {
    positions[i] += offset;
}
```

**Design**:
- Return `std::span<T>` (zero-overhead view)
- Inline small functions
- Avoid virtual calls in hot loops
- Use templates for type resolution (compile-time)

**Priority**: 🔥 **HIGH** - Core performance

---

## 4. Concurrency & Thread Safety

### 4.1 Read-Mostly Workloads

**Common pattern**: Many nodes reading, few writing
```cpp
// Multiple threads reading positions
auto positions = geo->get_positions();  // Read-only

// One thread writing new attribute
geo->add_attribute("temp", ...);  // Write
```

**Requirements**:
- Read-only access must be lock-free
- Writes can be serialized
- No data races

**Design**:
```cpp
class GeometryData {
    // Immutable after creation (read-only)
    const std::span<const Vector3> get_positions() const;

    // Mutable (requires lock or COW)
    std::span<Vector3> get_positions_writable();
};
```

**Priority**: 🟡 **MEDIUM** - Important but not blocking

---

### 4.2 Copy-on-Write (COW) Strategy

**Problem**: Copying geometry is expensive

**Solution**: Shared ownership until modification
```cpp
auto geo2 = geo1->clone();  // Fast: shares data

geo2->modify_positions();   // Triggers copy of positions only
// Other attributes still shared
```

**Implementation**:
```cpp
template<typename T>
class COWAttributeStorage {
    std::shared_ptr<std::vector<T>> data_;  // Shared

    std::span<T> get_writable() {
        if (data_.use_count() > 1) {
            // Copy-on-write
            data_ = std::make_shared<std::vector<T>>(*data_);
        }
        return {data_->data(), data_->size()};
    }
};
```

**Priority**: 🟡 **MEDIUM** - Performance optimization

---

## 5. Interpolation & Attribute Transfer

### 5.1 Interpolation Modes

**Needed for**:
- Subdivision surfaces (split edges → interpolate attributes)
- Resampling curves (add points → interpolate)
- Boolean operations (new vertices → blend attributes)

**Modes**:
```cpp
enum class InterpolationMode {
    NONE,           // Don't interpolate (e.g., material IDs)
    LINEAR,         // Linear blend (positions, colors)
    SMOOTH,         // Catmull-Rom or similar
    QUATERNION,     // Slerp for rotations
    DISCRETE,       // Nearest neighbor
};
```

**Example**:
```cpp
// Split edge between vertices v0 and v1 at t=0.5
Vector3 mid_pos = lerp(positions[v0], positions[v1], 0.5);  // LINEAR
int mid_id = ids[v0];  // DISCRETE (take first)
Quaternion mid_rot = slerp(rotations[v0], rotations[v1], 0.5);  // QUATERNION
```

**Storage**:
```cpp
struct AttributeDescriptor {
    InterpolationMode interpolation;

    // Function pointer for custom interpolation
    std::function<Value(const Value&, const Value&, float)> interpolate_fn;
};
```

**Priority**: 🔥 **HIGH** - Needed for subdivision, resampling

---

### 5.2 Attribute Promotion/Demotion

**Common operations**:
- Point → Vertex (replicate: each vertex gets point's value)
- Vertex → Point (average: point = average of its vertices)
- Point → Primitive (average: primitive = average of its points)
- Primitive → Point (splat: each point gets value from its primitives)

**Example**:
```cpp
// Promote point normals to vertex normals (replicate)
geo->promote_attribute(
    "N",                    // Attribute name
    ElementClass::POINT,    // From
    ElementClass::VERTEX,   // To
    PromotionMode::REPLICATE
);

// Demote vertex colors to point colors (average)
geo->demote_attribute(
    "Cd",
    ElementClass::VERTEX,
    ElementClass::POINT,
    DemotionMode::AVERAGE
);
```

**Priority**: 🟡 **MEDIUM** - Very useful but not blocking

---

## 6. Attribute Groups & Selection

### 6.1 Why Groups?

**Use case**: "Select front-facing polygons and extrude them"
- Need to mark subset of primitives
- Operate only on that subset

**Houdini approach**: Attribute groups
```cpp
// Create primitive group
geo->create_group("front_faces", ElementClass::PRIMITIVE);

// Add primitives to group
for (int prim_id : front_facing_prims) {
    geo->add_to_group("front_faces", prim_id);
}

// Iterate only group members
for (int prim_id : geo->get_group("front_faces")) {
    // Extrude this primitive
}
```

**Implementation**:
```cpp
class AttributeGroup {
    std::string name;
    ElementClass element_class;

    // Option A: Bitset (fast membership test)
    std::vector<bool> membership_;

    // Option B: Set (sparse, memory efficient)
    std::unordered_set<int> members_;

    // Option C: Ordered list (fast iteration)
    std::vector<int> member_indices_;
};
```

**Priority**: 🟡 **MEDIUM** - Very common workflow pattern

---

### 6.2 Attribute Wrangle (Expression System)

**Goal**: Per-element code execution
```cpp
// Pseudo-code: "For each point, offset position by noise"
@P += noise(@P) * 0.1;  // VEX-style

// Or Python-style:
def wrangle_point(point):
    point.P += noise(point.P) * 0.1
```

**Requirements**:
- Safe sandbox execution
- Access to attributes via names
- Fast (compiled or JIT)

**Design considerations**:
```cpp
class WrangleContext {
    GeometryData* geo;
    int current_element_index;

    // Attribute accessors
    Vector3& P() { return geo->get_positions()[current_element_index]; }
    Vector3& N() { return geo->get_normals()[current_element_index]; }
    // etc.
};

// Execute wrangle code
for (int i = 0; i < point_count; ++i) {
    context.current_element_index = i;
    execute_wrangle_code(context, user_code);
}
```

**Priority**: 🟢 **LOW** - Advanced feature, not needed initially

---

## 7. GPU Interoperability

### 7.1 Compute Shader Integration

**Goal**: Upload attributes to GPU, compute, download results

**Requirements**:
```cpp
// Pack attributes into GPU buffer
GLuint buffer = geo->pack_to_gpu({"P", "N", "Cd"});

// Compute shader operates on buffer
dispatch_compute_shader(buffer, vertex_count);

// Unpack results
geo->unpack_from_gpu(buffer, {"P"});  // Only position modified
```

**Memory layout** must be GPU-friendly:
```cpp
// Interleaved (AoS) - easier for rendering
struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 uv;
};

// Separate (SoA) - better for compute
std::vector<Vector3> positions;
std::vector<Vector3> normals;
std::vector<Vector2> uvs;
```

**Attribute system should support BOTH**:
- SoA for CPU compute (cache-friendly)
- Pack to interleaved for GPU when needed

**Priority**: 🟡 **MEDIUM** - Important for GPU acceleration

---

### 7.2 Staging Buffers

**Problem**: CPU↔GPU transfers are expensive

**Solution**: Keep data on GPU across multiple ops
```cpp
// Pipeline: Sphere → Transform → Subdivide → Extrude
// All on GPU, no transfers until final result needed

auto geo = create_sphere_gpu();  // Lives on GPU
geo->transform_gpu(matrix);      // Compute shader
geo->subdivide_gpu(2);           // Compute shader
geo->extrude_gpu(distance);      // Compute shader
auto cpu_result = geo->download();  // One transfer at end
```

**Attribute system needs**:
- Track where data lives (CPU/GPU/both)
- Lazy synchronization
- Staging buffer pool

**Priority**: 🟢 **LOW** - Optimization, not core functionality

---

## 8. Serialization & Caching

### 8.1 Save/Load Requirements

**What to serialize**:
- Attribute data (values)
- Attribute descriptors (types, metadata)
- Topology (point/vertex/primitive structure)
- Groups

**Formats needed**:
- **JSON** - Human-readable scene files
- **Binary** - Fast load/save for caching
- **OBJ/FBX/etc.** - Interop with other tools

**Design**:
```cpp
class GeometryData {
    // JSON (human-readable)
    nlohmann::json to_json() const;
    static GeometryData from_json(const nlohmann::json& j);

    // Binary (fast)
    std::vector<uint8_t> to_binary() const;
    static GeometryData from_binary(std::span<const uint8_t> data);
};
```

**Attribute-specific concerns**:
- Custom types: How to serialize?
- Large arrays: Stream instead of load all?
- Versioning: Old saves must load

**Priority**: 🟡 **MEDIUM** - Needed for practical use

---

### 8.2 Incremental Caching

**Problem**: Recooking entire graph is slow

**Solution**: Cache attribute versions, only recompute changed
```cpp
struct CachedAttribute {
    std::vector<uint8_t> data;
    uint64_t version;
    size_t data_hash;
};

// Check if cache valid
if (cache.version == attr.version && cache.hash == hash(attr.data)) {
    return cache;  // Reuse
}
```

**Attribute system support**:
- Version tracking per attribute (already planned)
- Content hashing
- Dependency tracking

**Priority**: 🟢 **LOW** - Optimization

---

## 9. Validation & Safety

### 9.1 Size Consistency

**Problem**: Attributes must match element count
```cpp
// INVALID:
geo->point_count = 100;
geo->set_attribute("P", positions);  // Only 50 values!
```

**Requirements**:
- Enforce size on set
- Auto-resize when topology changes
- Clear error messages

**Design**:
```cpp
void set_attribute(const std::string& name,
                   ElementClass cls,
                   std::span<T> values) {
    size_t expected = get_element_count(cls);
    if (values.size() != expected) {
        throw AttributeSizeError(
            fmt::format("{} attribute '{}' size mismatch: expected {}, got {}",
                       cls, name, expected, values.size())
        );
    }
    // OK, set
}
```

**Priority**: 🔥 **HIGH** - Correctness

---

### 9.2 Type Safety

**Problem**: Accessing wrong type
```cpp
auto ids = geo->get_attribute<int>("Cd");  // "Cd" is Vector3, not int!
```

**Requirements**:
- Runtime type checking
- Clear error messages
- Optional compile-time checks

**Design**:
```cpp
template<typename T>
std::span<T> get_attribute(const std::string& name, ElementClass cls) {
    auto desc = get_descriptor(name);

    // Runtime check
    if (!desc.is_type<T>()) {
        throw AttributeTypeError(
            fmt::format("Attribute '{}' is {}, not {}",
                       name, desc.type_name(), type_name<T>())
        );
    }

    return /* cast and return */;
}
```

**Priority**: 🔥 **HIGH** - Correctness

---

## 10. Missing Pieces Summary

### 🔥 CRITICAL (Must Have Before Launch)

1. **Point vs Vertex Separation**
   - Formalize topology model
   - vertex_to_point mapping
   - Support split normals/UVs

2. **SoA Memory Layout**
   - Replace variant storage
   - Typed contiguous arrays
   - 10-100x faster iteration

3. **Matrix & Vector4 Types**
   - Transforms, rotations
   - RGBA colors
   - Needed for instancing

4. **Interpolation System**
   - Linear, discrete, quaternion
   - Needed for subdivision/resampling

5. **Size & Type Validation**
   - Enforce correctness
   - Clear error messages

### 🟡 IMPORTANT (Should Have Soon)

6. **Attribute Promotion/Demotion**
   - Point ↔ Vertex ↔ Primitive
   - Very common workflow

7. **Attribute Groups**
   - Select subsets
   - Operate on selection

8. **Array/Variable-Length Types**
   - N-gons (variable vertex count)
   - Neighbor lists

9. **Serialization**
   - Binary format for caching
   - JSON for scenes

10. **GPU Staging**
    - Pack for compute shaders
    - Interleaved format support

### 🟢 NICE TO HAVE (Later)

11. **Sparse Attributes**
    - Memory optimization
    - Large geometry support

12. **Copy-on-Write**
    - Performance optimization

13. **Wrangle/Expression System**
    - Per-element scripting

14. **Custom User Types**
    - Type registry
    - Extensibility

15. **Incremental Caching**
    - Smart dependency tracking

---

## 11. Recommended Phased Approach

### Phase 1: Core Foundation (Weeks 1-3)
**Build the absolute minimum for correctness**:
- ✅ Point/Vertex/Primitive topology model
- ✅ AttributeDescriptor with type metadata
- ✅ SoA storage (typed arrays)
- ✅ Standard attribute names ("P", "N", "Cd")
- ✅ Basic types: float, int, Vector2/3/4, Matrix3/4, string
- ✅ Size & type validation
- ✅ Interpolation modes (Linear, Discrete)

**Deliverable**: Solid, correct foundation

---

### Phase 2: Integration (Weeks 4-6)
**Make it work with existing SOPs**:
- ✅ Enhance GeometryAttributes
- ✅ Integrate into GeometryData
- ✅ Migrate SOPs to new API
- ✅ Full test coverage

**Deliverable**: Working system, all SOPs migrated

---

### Phase 3: Performance (Weeks 7-9)
**Optimize hot paths**:
- ✅ Profile attribute access
- ✅ Add span-based views
- ✅ Inline critical functions
- ✅ Benchmark vs old system

**Deliverable**: Fast attribute operations

---

### Phase 4: Advanced Features (Weeks 10-12)
**Add power-user features**:
- ✅ Attribute groups
- ✅ Promotion/demotion
- ✅ Array types for N-gons
- ✅ GPU staging (basic)

**Deliverable**: Feature-complete attribute system

---

### Phase 5: Future Enhancements (Post-Launch)
**When needed**:
- Sparse attributes (when handling >10M elements)
- Wrangle system (when adding scripting)
- Advanced GPU integration (when doing real-time)
- Custom types (when users request it)

---

## 12. Open Questions for You

### A) Type System Scope
**Question**: How far should we go with types initially?

**Option 1: Minimal** (faster to ship)
- float, int, Vector2/3, string
- Add Matrix/Vector4/etc. later

**Option 2: Comprehensive** (future-proof)
- float, int, Vector2/3/4, Matrix3/4, Quaternion, string
- All common types from start

**My recommendation**: Option 2 - types are foundational, hard to add later

---

### B) Array/Variable-Length Support
**Question**: Do we need N-gons (polygons with arbitrary vertex count) now?

**Current**: Only triangles (3 vertices per face)

**With N-gons**: Faces can have 3, 4, 5, ... vertices

**Pros**: More flexibility, matches industry tools
**Cons**: More complex topology, slower initial development

**My recommendation**: Support variable-length primitives from the start
- Simpler to design in now than retrofit later
- Industry standard (Houdini, Blender all support)

---

### C) GPU Priority
**Question**: How important is GPU compute integration?

**Option 1: Core Feature** (design for GPU from start)
- Attribute layout optimized for GPU
- Staging buffers built-in
- More upfront complexity

**Option 2: Future Addition** (CPU-first, add GPU later)
- Simpler initial implementation
- Add GPU when needed
- Might require some refactoring later

**My recommendation**: Design for GPU but implement later
- Use SoA layout (GPU-friendly)
- Add pack/unpack methods in Phase 4
- Don't block initial development

---

### D) Scripting/Wrangle Priority
**Question**: How soon do we need per-element scripting?

**Option 1: Core Feature**
- Build wrangle system early
- Enables power users
- Significant development time

**Option 2: Post-Launch**
- Focus on solid C++ API first
- Add scripting when requested
- Simpler initial scope

**My recommendation**: Post-launch
- Get core attribute system solid first
- Wrangle builds on top, can add later

---

## 13. Final Recommendation

**Build in this order**:

### Immediate (Weeks 1-3): FOUNDATION
1. ✅ Point/Vertex/Primitive topology
2. ✅ AttributeDescriptor
3. ✅ SoA storage
4. ✅ Types: float, int, Vec2/3/4, Matrix3/4, string
5. ✅ Standard names ("P", "N", etc.)
6. ✅ Interpolation (Linear, Discrete, Quaternion)
7. ✅ Validation (size, type)

### Next (Weeks 4-6): INTEGRATION
8. ✅ GeometryData integration
9. ✅ SOP migration
10. ✅ Testing

### Then (Weeks 7-9): PERFORMANCE
11. ✅ Span-based views
12. ✅ Profiling & optimization
13. ✅ Benchmarks

### Future (Weeks 10+): ADVANCED
14. ⬜ Attribute groups
15. ⬜ GPU staging
16. ⬜ Wrangle system (Phase 5)

---

## 14. What Could We Be Missing?

**Potential gaps to consider**:

1. **Attribute Defaults** - When adding new points, what values?
2. **Attribute Metadata** - Beyond type, need units? Display hints?
3. **Attribute Dependencies** - "uv" requires "N" to be meaningful?
4. **Attribute Versioning** - Different versions of same attribute?
5. **Attribute Compression** - For large datasets?
6. **Attribute Streaming** - Load portions on-demand?
7. **Attribute Validation** - Range constraints? (e.g., UV must be [0,1])
8. **Attribute Locking** - Prevent modification?

**My take**: Start with core, add these as needs arise. Don't over-engineer.

---

**What are YOUR thoughts on these open questions?**
- Type system scope?
- N-gon support timing?
- GPU priority?
- Any concerns I haven't addressed?
