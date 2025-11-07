# PMP Library Integration Strategy

## Executive Summary

This document outlines the strategy for integrating the Polygon Mesh Processing (PMP) library into Nodo. PMP provides high-quality implementations of mesh processing algorithms including decimation, remeshing, subdivision, and smoothing. We will wrap the library to maintain architectural flexibility and allow for future replacement if needed.

## Library Overview

### PMP Library (https://www.pmp-library.org/)

**License:** MIT (commercial-friendly)  
**Language:** Modern C++  
**Repository:** https://github.com/pmp-library/pmp-library

### Key Capabilities

#### Algorithms We'll Use
1. **Decimation** - Reduce triangle count while preserving shape
   - Configurable parameters: target vertices, aspect ratio, edge length, normal deviation
   - Feature-aware (preserves sharp edges)
   - Hausdorff error control

2. **Remeshing** - Improve mesh quality
   - Uniform remeshing (isotropic triangulation)
   - Adaptive remeshing (curvature-based)
   - Feature preservation
   - Projection to original surface

3. **Subdivision** - Refine meshes
   - Loop subdivision (triangle meshes)
   - Catmull-Clark subdivision (quad meshes)
   - Quad-Tri subdivision (mixed meshes)
   - Boundary handling options

4. **Smoothing** - Reduce noise
   - Explicit smoothing (Laplacian)
   - Implicit smoothing (better quality)
   - Uniform vs. weighted Laplacian

5. **Hole Filling** - Close mesh holes
   - Automatic boundary detection
   - Fair hole filling

#### Algorithms We Won't Use
- **Mesh Generation** - Nodo already has optimized generators for sphere, box, cylinder, torus
- **I/O Operations** - Nodo has its own OBJ importer/exporter
- **Visualization** - Nodo has Qt-based UI

## Integration Architecture

### Design Principles

1. **Abstraction Layer** - All PMP functionality wrapped behind Nodo interfaces
2. **Replaceability** - Easy to swap PMP for another library or custom implementation
3. **Consistency** - Match existing Nodo patterns (see `BooleanOps` with Manifold)
4. **Type Safety** - Use Nodo's types (`GeometryContainer`, `Mesh`)
5. **Error Handling** - Consistent with Nodo's error system

### Directory Structure

```
nodo_core/
├── include/nodo/
│   └── processing/              # New: mesh processing operations
│       ├── mesh_processor.hpp   # Base interface
│       ├── decimation.hpp       # Decimation wrapper
│       ├── remeshing.hpp        # Remeshing wrapper
│       ├── smoothing.hpp        # Smoothing wrapper
│       ├── hole_filling.hpp     # Hole filling wrapper
│       └── pmp_converter.hpp    # PMP ↔ Nodo conversion
└── src/
    └── processing/              # Implementation files
        ├── decimation.cpp
        ├── remeshing.cpp
        ├── smoothing.cpp
        ├── hole_filling.cpp
        └── pmp_converter.cpp
```

### Namespace Organization

```cpp
namespace nodo::processing {
    // Wrapper classes for mesh processing algorithms
    class Decimation { /* ... */ };
    class Remeshing { /* ... */ };
    class Smoothing { /* ... */ };
    class HoleFilling { /* ... */ };
    
    namespace detail {
        // Internal conversion utilities
        class PMPConverter { /* ... */ };
    }
}
```

## Implementation Strategy

### Phase 1: Infrastructure (Week 1)

**Goal:** Set up conversion between Nodo and PMP data structures

#### 1.1 Add PMP Dependency
```python
# conanfile.py
def requirements(self):
    # ... existing dependencies ...
    self.requires("pmp-library/3.0.0")  # Check if available in Conan Center
```

If not in Conan Center, we'll need to build from source.

#### 1.2 Create Converter (`pmp_converter.hpp`)
```cpp
namespace nodo::processing::detail {

class PMPConverter {
public:
    // Convert Nodo Mesh to PMP SurfaceMesh
    static pmp::SurfaceMesh to_pmp(const core::Mesh& mesh);
    
    // Convert Nodo GeometryContainer to PMP SurfaceMesh
    static pmp::SurfaceMesh to_pmp(const core::GeometryContainer& container);
    
    // Convert PMP SurfaceMesh back to Nodo Mesh
    static core::Mesh from_pmp(const pmp::SurfaceMesh& mesh);
    
    // Convert PMP SurfaceMesh back to GeometryContainer
    static core::GeometryContainer from_pmp_container(
        const pmp::SurfaceMesh& mesh);
};

}
```

#### 1.3 Testing Infrastructure
- Unit tests for conversions
- Verify round-trip (Nodo → PMP → Nodo)
- Test with various primitive types

**Deliverables:**
- ✅ PMP library integrated via Conan
- ✅ Conversion utilities implemented
- ✅ Tests passing

---

### Phase 2: Decimation (Week 2)

**Goal:** Implement mesh decimation/simplification

#### 2.1 Decimation Wrapper (`decimation.hpp`)

```cpp
namespace nodo::processing {

struct DecimationParams {
    size_t target_vertex_count = 0;      // 0 = use target_percentage
    float target_percentage = 0.5f;      // Target as % of original
    float aspect_ratio = 0.0f;           // Min triangle quality
    float edge_length = 0.0f;            // Min edge length
    unsigned int max_valence = 0;        // Max edges per vertex
    float normal_deviation = 0.0f;       // Max normal change (degrees)
    float hausdorff_error = 0.0f;        // Max surface deviation
    bool preserve_features = true;       // Keep sharp edges
};

class Decimation {
public:
    /// Decimate a mesh to reduce triangle count
    static std::optional<core::Mesh> decimate(
        const core::Mesh& input,
        const DecimationParams& params);
    
    /// Decimate using GeometryContainer
    static std::optional<core::GeometryContainer> decimate(
        const core::GeometryContainer& input,
        const DecimationParams& params);
    
    /// Get last error
    static const core::Error& last_error();
    
private:
    static thread_local core::Error last_error_;
};

}
```

#### 2.2 Update Existing SubdivisionSOP
Currently, Nodo has a basic subdivision implementation. We can:
- Keep existing implementation as fallback
- Add option to use PMP-based subdivision
- Compare quality/performance

#### 2.3 Create New DecimationSOP

```cpp
class DecimationSOP : public SOPNode {
public:
    DecimationSOP(const std::string& name);
    
    void cook(CookContext& ctx) override;
    
private:
    // Parameters matching DecimationParams
    IntParameter target_vertices_;
    FloatParameter target_percentage_;
    FloatParameter normal_deviation_;
    BoolParameter preserve_features_;
    // ... etc
};
```

**Deliverables:**
- ✅ Decimation wrapper implemented
- ✅ DecimationSOP node created
- ✅ Tests with various mesh types
- ✅ UI integration

---

### Phase 3: Remeshing (Week 3)

**Goal:** Improve mesh quality through remeshing

#### 3.1 Remeshing Wrapper (`remeshing.hpp`)

```cpp
namespace nodo::processing {

enum class RemeshingMode {
    Uniform,    // Isotropic triangulation
    Adaptive    // Curvature-based
};

struct RemeshingParams {
    RemeshingMode mode = RemeshingMode::Uniform;
    
    // Uniform remeshing
    float target_edge_length = 0.0f;     // 0 = use mean edge length
    
    // Adaptive remeshing
    float min_edge_length = 0.0f;
    float max_edge_length = 0.0f;
    float approximation_error = 0.0f;
    
    // Common parameters
    unsigned int iterations = 10;
    bool use_projection = true;          // Project to original surface
    bool preserve_features = true;
};

class Remeshing {
public:
    /// Remesh to improve triangle quality
    static std::optional<core::Mesh> remesh(
        const core::Mesh& input,
        const RemeshingParams& params);
    
    /// Remesh using GeometryContainer
    static std::optional<core::GeometryContainer> remesh(
        const core::GeometryContainer& input,
        const RemeshingParams& params);
    
    /// Get last error
    static const core::Error& last_error();
    
private:
    static thread_local core::Error last_error_;
};

}
```

#### 3.2 Create RemeshSOP

```cpp
class RemeshSOP : public SOPNode {
public:
    RemeshSOP(const std::string& name);
    
    void cook(CookContext& ctx) override;
    
private:
    IntParameter mode_;                  // Uniform/Adaptive
    FloatParameter target_edge_length_;
    FloatParameter min_edge_length_;
    FloatParameter max_edge_length_;
    IntParameter iterations_;
    BoolParameter use_projection_;
};
```

**Deliverables:**
- ✅ Remeshing wrapper implemented
- ✅ RemeshSOP node created
- ✅ Tests with low-quality meshes
- ✅ UI integration

---

### Phase 4: Smoothing (Week 4)

**Goal:** Add mesh smoothing capabilities

#### 4.1 Smoothing Wrapper (`smoothing.hpp`)

```cpp
namespace nodo::processing {

enum class SmoothingMethod {
    Explicit,   // Fast, basic Laplacian
    Implicit    // Higher quality, slower
};

struct SmoothingParams {
    SmoothingMethod method = SmoothingMethod::Implicit;
    unsigned int iterations = 1;
    float timestep = 0.001f;             // For implicit smoothing
    bool use_uniform_laplace = false;    // Uniform vs. cotangent weights
    bool rescale = true;                 // Maintain volume
};

class Smoothing {
public:
    /// Smooth a mesh to reduce noise
    static std::optional<core::Mesh> smooth(
        const core::Mesh& input,
        const SmoothingParams& params);
    
    /// Smooth using GeometryContainer
    static std::optional<core::GeometryContainer> smooth(
        const core::GeometryContainer& input,
        const SmoothingParams& params);
    
    /// Get last error
    static const core::Error& last_error();
    
private:
    static thread_local core::Error last_error_;
};

}
```

#### 4.2 Update LaplacianSOP
Nodo already has `LaplacianSOP`. We can:
- Extend it to use PMP's smoothing
- Add method selection (Nodo's vs. PMP's)
- Keep existing as "Legacy" option

**Deliverables:**
- ✅ Smoothing wrapper implemented
- ✅ LaplacianSOP enhanced with PMP backend
- ✅ Tests comparing methods
- ✅ UI integration

---

### Phase 5: Subdivision Enhancement (Week 5)

**Goal:** Replace/enhance current subdivision with PMP

#### 5.1 Subdivision Wrapper (`subdivision.hpp`)

```cpp
namespace nodo::processing {

enum class SubdivisionMethod {
    Loop,           // For triangle meshes
    CatmullClark,   // For quad meshes
    QuadTri,        // Mixed triangle/quad
    Linear          // Simple linear
};

enum class BoundaryHandling {
    Interpolate,    // Smooth boundaries
    Preserve        // Keep sharp boundaries
};

struct SubdivisionParams {
    SubdivisionMethod method = SubdivisionMethod::CatmullClark;
    unsigned int levels = 1;
    BoundaryHandling boundary = BoundaryHandling::Interpolate;
};

class Subdivision {
public:
    /// Subdivide a mesh
    static std::optional<core::Mesh> subdivide(
        const core::Mesh& input,
        const SubdivisionParams& params);
    
    /// Subdivide using GeometryContainer
    static std::optional<core::GeometryContainer> subdivide(
        const core::GeometryContainer& input,
        const SubdivisionParams& params);
    
    /// Get last error
    static const core::Error& last_error();
    
private:
    static thread_local core::Error last_error_;
};

}
```

#### 5.2 Update SubdivisionSOP
- Add method selection
- Keep existing implementation as option
- Use PMP by default

**Deliverables:**
- ✅ Subdivision wrapper implemented
- ✅ SubdivisionSOP enhanced
- ✅ Tests comparing methods
- ✅ UI integration

---

### Phase 6: Hole Filling (Week 6)

**Goal:** Add automatic hole detection and filling

#### 6.1 Hole Filling Wrapper (`hole_filling.hpp`)

```cpp
namespace nodo::processing {

class HoleFilling {
public:
    /// Fill all holes in a mesh
    static std::optional<core::Mesh> fill_holes(
        const core::Mesh& input);
    
    /// Fill holes using GeometryContainer
    static std::optional<core::GeometryContainer> fill_holes(
        const core::GeometryContainer& input);
    
    /// Detect and return boundary loops
    static std::vector<std::vector<int>> detect_holes(
        const core::Mesh& input);
    
    /// Get last error
    static const core::Error& last_error();
    
private:
    static thread_local core::Error last_error_;
};

}
```

#### 6.2 Create HoleFillSOP

```cpp
class HoleFillSOP : public SOPNode {
public:
    HoleFillSOP(const std::string& name);
    
    void cook(CookContext& ctx) override;
    
private:
    // Could add parameters for:
    // - Hole size threshold
    // - Fill method (fair vs. minimal)
};
```

**Deliverables:**
- ✅ Hole filling wrapper implemented
- ✅ HoleFillSOP node created
- ✅ Tests with open meshes
- ✅ UI integration

---

## Testing Strategy

### Unit Tests

Each wrapper class needs comprehensive tests:

```cpp
TEST(DecimationTest, BasicDecimation) {
    // Create test mesh (sphere with known vertex count)
    auto sphere = SphereGenerator::generate_icosphere(1.0, 3);
    
    // Decimate to 50%
    DecimationParams params;
    params.target_percentage = 0.5f;
    
    auto result = Decimation::decimate(*sphere, params);
    ASSERT_TRUE(result.has_value());
    
    // Verify vertex count reduced
    size_t original_count = sphere->topology().point_count();
    size_t result_count = result->topology().point_count();
    EXPECT_LT(result_count, original_count);
}
```

### Integration Tests

Test SOPs in actual node graphs:

```cpp
TEST(RemeshSOPTest, InGraphExecution) {
    // Create graph: Sphere -> Remesh -> Output
    NodeGraph graph;
    auto sphere_node = graph.add_node("sphere_gen");
    auto remesh_node = graph.add_node("remesh");
    
    graph.connect(sphere_node->output(0), remesh_node->input(0));
    
    // Execute
    ExecutionEngine engine(graph);
    engine.evaluate(remesh_node);
    
    // Verify output
    auto result = remesh_node->get_output_geometry(0);
    ASSERT_NE(result, nullptr);
    EXPECT_GT(result->topology().primitive_count(), 0);
}
```

### Quality Metrics

Track algorithm performance:

```cpp
struct MeshQuality {
    float min_triangle_quality;
    float avg_triangle_quality;
    float max_triangle_quality;
    size_t degenerate_count;
};

MeshQuality analyze_mesh(const core::Mesh& mesh);
```

---

## Dependency Management

### Conan Integration

If PMP is in Conan Center:
```python
def requirements(self):
    self.requires("pmp-library/3.0.0")
```

If not available, options:
1. Create Conan recipe for PMP
2. Use CMake FetchContent as fallback
3. Build as external dependency

### CMake Integration

```cmake
# Find PMP
find_package(pmp REQUIRED)

# Link to nodo_core
target_link_libraries(nodo_core
    PRIVATE
        pmp::pmp
)

# Include directories
target_include_directories(nodo_core
    PRIVATE
        ${PMP_INCLUDE_DIRS}
)
```

---

## Migration Path

### For Existing Users

1. **Backwards Compatibility**
   - Keep existing subdivision implementation
   - Add "Use PMP" option to relevant SOPs
   - Default to legacy behavior initially

2. **Gradual Migration**
   - Phase 1: Add new nodes (DecimationSOP, RemeshSOP)
   - Phase 2: Enhance existing nodes (LaplacianSOP)
   - Phase 3: Deprecate old implementations (with warnings)

3. **Performance Testing**
   - Benchmark PMP vs. current implementations
   - Ensure no regression in common workflows

---

## Future Considerations

### Potential Replacements

If we need to replace PMP, the wrapper design makes it straightforward:

1. **OpenMesh** (LGPL) - More permissive than GPL but copyleft
2. **CGAL** (GPL/Commercial) - Very powerful but license restrictions
3. **Custom Implementation** - Based on research papers
4. **Other Libraries** - libigl, MeshLab components, etc.

### Extension Points

The wrapper architecture allows adding:
- Custom heuristics for algorithm parameters
- Nodo-specific optimizations
- Attribute preservation logic
- Progress callbacks for long operations

---

## Success Criteria

### Functional Requirements
- ✅ All 5 algorithm categories working (decimation, remeshing, subdivision, smoothing, hole filling)
- ✅ SOPs integrated into UI
- ✅ Comprehensive test coverage (>80%)
- ✅ Documentation for each node

### Non-Functional Requirements
- ✅ No breaking changes to existing API
- ✅ Performance comparable to or better than existing implementations
- ✅ Clean separation from PMP (easy to replace)
- ✅ Consistent error handling

### Quality Metrics
- ✅ All unit tests passing
- ✅ No memory leaks (valgrind clean)
- ✅ No crashes with invalid input
- ✅ Proper cleanup of PMP resources

---

## Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| PMP not in Conan Center | Medium | Build from source, create recipe |
| Performance worse than current | Low | Benchmark early, optimize wrapper |
| Breaking API changes in PMP | Low | Pin to specific version |
| License issues | Very Low | MIT license, very permissive |
| Integration complexity | Medium | Start small, iterate |

---

## Timeline Summary

| Phase | Duration | Deliverables |
|-------|----------|--------------|
| Infrastructure | Week 1 | Converters, tests |
| Decimation | Week 2 | DecimationSOP |
| Remeshing | Week 3 | RemeshSOP |
| Smoothing | Week 4 | Enhanced LaplacianSOP |
| Subdivision | Week 5 | Enhanced SubdivisionSOP |
| Hole Filling | Week 6 | HoleFillSOP |
| **Total** | **6 weeks** | **5 new/enhanced SOPs** |

---

## Conclusion

Integrating PMP library provides Nodo with production-quality mesh processing algorithms while maintaining architectural flexibility. The wrapper design allows easy replacement if needed, and the phased approach minimizes risk while delivering value incrementally.

The key advantages:
1. ✅ **Quality** - Battle-tested algorithms from academic research
2. ✅ **License** - MIT license, commercial-friendly
3. ✅ **Maintainability** - Active development, modern C++
4. ✅ **Flexibility** - Wrapped design allows replacement
5. ✅ **Integration** - Matches existing patterns (like Manifold wrapper)

This strategy positions Nodo to compete with commercial tools while maintaining its open-source nature and architectural cleanliness.
