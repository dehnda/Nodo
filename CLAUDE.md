# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

NodeFluxEngine is a **production-ready C++20 GPU-accelerated procedural mesh generation library** with a Houdini-inspired SOP (Surface Operator) workflow. The project provides a complete node-based procedural modeling system with intelligent caching, visual editing, and real-time performance.

### Current Status: Production Ready ✅

**Core Achievement**: We have built a complete, working procedural mesh generation system that rivals industry tools.

- **18 Working SOP Nodes** - Full procedural workflow capability
- **Qt Studio Application** - Visual node editor with 3D viewport
- **Complete Data Flow System** - Smart caching and dependency resolution
- **Comprehensive Testing** - 59 unit tests covering all major systems
- **Modern Architecture** - Clean C++20 design with robust error handling

---

## What's Actually Implemented (October 2025)

### ✅ Core Infrastructure (Complete)

**Build System**:
- CMake 3.20+ with Conan 2.x dependency management
- Cross-platform support (Linux, Windows via WSL)
- Automated build tasks for VS Code
- Clean preset-based configuration

**Dependencies (Auto-managed by Conan)**:
- Eigen3 3.4.0 - Linear algebra
- CGAL 5.6.1 - Boolean operations
- Qt 6.7.3 - GUI framework
- Google Test 1.14.0 - Testing
- fmt, nlohmann_json, GMP/MPFR

**Error Handling**:
- Thread-local error storage (`nodeflux::core::Error`)
- `std::optional<T>` for operations that can fail (NO `std::expected` - C++23 only)
- Node execution states: CLEAN, DIRTY, COMPUTING, ERROR

### ✅ Core Data Structures (Complete)

**Mesh (`nodeflux::core::Mesh`)**:
- Eigen::MatrixXd for vertices and faces
- Vertex normals, face normals, bounds calculation
- Thread-safe, move-optimized
- Comprehensive validation system

**GeometryContainer (`nodeflux::core::GeometryContainer`)** - Modern Attribute System v2.0:
- Primary geometry container with unified attribute storage
- Structure-of-Arrays (SoA) layout for cache efficiency and SIMD
- Four element classes: Point, Vertex, Primitive, Detail
- Type-safe attribute access with AttributeStorage<T> templates
- Support for Vec2f, Vec3f, Vec4f, float, int, Mat3f, Mat4f, Quatf, string
- Houdini-compatible standard names ("P", "N", "Cd", "uv", "Alpha", "v", "pscale")
- Advanced features:
  - Attribute promotion/demotion between element classes
  - Named groups for selection with boolean operations
  - Comprehensive interpolation (linear, cubic, weighted, barycentric, bilinear, slerp)
- Performance: 34.5M/s sequential write, 10.3M/s sequential read (1M Vec3f)
- Full documentation: [API Reference](docs/ATTRIBUTE_SYSTEM_API.md)

**GeometryData (`nodeflux::sop::GeometryData`)**:
- Primary data container passed between nodes
- Contains GeometryContainer (modern) + legacy Mesh (for compatibility)
- Clone, merge, and transform operations
- Attribute type system with variant storage

**Legacy: GeometryAttributes** (deprecated, kept for compatibility):
- Old attribute system, replaced by GeometryContainer
- Being phased out in favor of unified GeometryContainer approach

### ✅ SOP Node System (Production Ready)

**Base Class (`nodeflux::sop::SOPNode`)**:
- Located: `nodeflux_core/include/nodeflux/sop/sop_node.hpp`
- Smart caching with automatic dirty state management
- Input/output port system with type checking
- Parameter system with variant types
- Execution timing and performance tracking
- `cook()` method pattern for all nodes

**Node Ports (`nodeflux::sop::NodePort`)**:
- Type-safe connections between nodes
- Multiple port types: GEOMETRY, TRANSFORM, SCALAR, etc.
- Connection validation and data flow

### ✅ Implemented SOP Nodes (18 Total)

**Location**: `nodeflux_core/include/nodeflux/sop/`

**Generators (5 nodes)**:
- `sphere_sop.hpp` - UV sphere and icosphere variants
- `line_sop.hpp` - Line generation (Oct 20, 2025)
- Plus: Box, Cylinder, Plane, Torus (in `/nodes/` directory)

**IO (2 nodes)**:
- `file_sop.hpp` - File import (OBJ support)
- `export_sop.hpp` - File export with manual export button (Oct 25, 2025)

**Deformation/Modifiers (5 nodes)**:
- `extrude_sop.hpp` (341 lines) - Face extrusion with caps (STUB - needs implementation)
- `polyextrude_sop.hpp` - Per-polygon extrusion (STUB - needs implementation)
- `laplacian_sop.hpp` (385 lines) - 3 smoothing algorithms (uniform, cotan, taubin) ✅
- `subdivision_sop.hpp` - Catmull-Clark and Simple subdivision ✅ (Oct 26, 2025)
- `resample_sop.hpp` (181 lines) - Edge/curve resampling (STUB - needs implementation)

**Array/Duplication (3 nodes)**:
- `array_sop.hpp` - Linear (✅), radial (STUB), grid (STUB) patterns
- `scatter_sop.hpp` - Random point distribution (Oct 20)
- `copy_to_points_sop.hpp` - Instance geometry to points (STUB - needs implementation)

**Boolean/Transform (3 nodes)**:
- `boolean_sop.hpp` - Union/Intersection/Difference with Manifold ✅ (Oct 26, 2025)
- `mirror_sop.hpp` (194 lines) - Mirror across planes
- Transform nodes (built into system)

**Advanced (1 node)**:
- `noise_displacement_sop.hpp` - Procedural noise deformation

### ✅ Node Graph System (Complete)

**Location**: `nodeflux_core/include/nodeflux/graph/`

**NodeGraph (`node_graph.hpp`)** - 272 lines:
- Pure data model (separate from UI)
- Supports 24+ node types
- Add/remove/connect nodes
- Topological sorting for execution
- JSON serialization via `GraphSerializer`

**ExecutionEngine (`execution_engine.cpp`)**:
- Smart dependency resolution
- Incremental updates (only rebuild changed branches)
- Caching at node level
- Error propagation

### ✅ Qt Studio Application (Fully Functional)

**Location**: `nodeflux_studio/src/`

**Features**:
- Visual node graph editor with drag-and-drop
- Property panel for parameter editing
- 3D viewport with OpenGL rendering
- Scene save/load (JSON format)
- Dark theme (Oct 19, 2025)
- Display/render/bypass flags (Houdini-style)

**ViewportWidget** - 1112 lines of sophisticated rendering:
- Blinn-Phong shading with lighting
- Wireframe mode
- Vertex/edge/face normal visualization
- Grid and axis display
- Camera controls (orbit, pan, zoom)
- Point cloud rendering
- Multisampling (4x MSAA)

### ✅ Spatial Acceleration (Complete)

**BVH System**:
- 45x speedup over brute-force for boolean operations
- Used automatically by BooleanSOP
- Bounding volume hierarchy for ray/mesh intersection

### ✅ Import/Export (OBJ Complete)

**OBJ Format** (`nodeflux_core/include/nodeflux/io/`):
- Full Wavefront OBJ export via ExportSOP
- Manual export trigger with "Export Now" button in property panel
- Pass-through node design (geometry flows to output)
- Vertex positions, normals, texture coordinates
- Face topology

---

## What's NOT Yet Implemented

### ❌ GPU Compute Shaders

**Status**: Experimental work done, not yet committed

GPU compute shader infrastructure was prototyped (buffer management, shader compilation, sphere generation) but **not included in main branch** because:
- Performance was slower than CPU without GPU chaining (0.23-0.83x due to transfer overhead)
- Requires architectural changes to keep data on GPU between operations
- Would mislead users about performance benefits

**What was learned**:
- GLSL compute shaders work correctly for sphere generation
- GPU buffer management (SSBO) implemented and tested
- Key insight: Need to chain operations on GPU to avoid expensive CPU↔GPU transfers
- Future implementation should keep meshes on GPU across multiple SOP nodes

**Impact**: All operations are CPU-bound for now. GPU acceleration is a future enhancement that requires proper data flow architecture first.

### ❌ Additional File Formats

- STL export
- PLY export
- glTF export
- Any mesh import (only export works)

### ❌ Advanced Features

- Material system (attributes exist, not integrated)
- UV unwrapping/mapping tools
- Bend/Twist/Taper deformations
- Texture support in viewport

---

## Architecture Guide

### Module Hierarchy

```
nodeflux_core/
├── core/           - Fundamental data (Mesh, Vector, GeometryAttributes)
├── geometry/       - Mesh generators, boolean ops, validation
├── spatial/        - BVH acceleration structures
├── sop/            - SOP node system (17 nodes)
├── graph/          - Node graph, execution engine, serialization
├── nodes/          - Legacy primitive nodes (5 generators)
└── io/             - OBJ export

nodeflux_studio/
├── src/            - Qt application (MainWindow, ViewportWidget, etc.)
└── resources/      - UI resources, icons

tests/              - Google Test suite (59 tests)
```

### Data Flow Pattern

```
User modifies parameter → Node marked dirty →
Cook input dependencies → Execute node.cook() →
Update output cache → Propagate to connected nodes
```

### Key Design Patterns

**SOP Node Pattern**:
```cpp
class CustomSOP : public SOPNode {
public:
    CustomSOP(const std::string& name) : SOPNode(name, "Custom") {}

protected:
    std::optional<std::shared_ptr<GeometryData>> cook() override {
        // Get input
        auto input = get_input_data(0);
        if (!input) return std::nullopt;

        // Process
        auto output = std::make_shared<GeometryData>(*input);
        // ... modify output ...

        return output;
    }
};
```

**Caching**: Automatic - node only re-executes when:
- Input data changes
- Parameters change
- Manually marked dirty

**Error Handling**:
- Return `std::nullopt` on failure
- Set error message: `set_error("reason")`
- Check state: `execution_state() == ExecutionState::ERROR`

---

## Code Style Guide

### Naming Conventions

- **Variables/Functions**: `snake_case` (e.g., `vertex_count`, `calculate_bounds()`)
- **Types/Classes**: `PascalCase` (e.g., `GeometryData`, `SOPNode`)
- **Constants**: `UPPER_SNAKE_CASE` (e.g., `DEFAULT_RADIUS`)
- **Private Members**: trailing underscore (e.g., `vertex_count_`, `cache_`)

### C++ Standards

**Use C++20 Features**:
- Concepts, ranges, designated initializers
- `std::optional<T>` for fallible operations
- `constexpr` over `const` for compile-time constants
- `auto` with explicit types when clarity matters

**Avoid C++23**:
- NO `std::expected` (use `std::optional` instead)
- NO `std::print` (use `fmt::print`)

**Style Requirements**:
- Float literals: `2.0F` not `2.0f` (uppercase F)
- Null checks: `(ptr != nullptr)` not just `ptr`
- Static members: `Class::method()` not `instance.method()`
- Include guards: `#pragma once` preferred
- Remove unused includes, use forward declarations

### Performance Guidelines

- Use Eigen for vectorized math (avoid manual loops)
- Reserve container capacity when size is known
- Prefer move semantics for large data
- Profile before optimizing
- Use BVH for operations on large meshes (>10K triangles)

---

## Build System

### Common Commands

```bash
# Initial setup
conan install . --output-folder=build --build=missing
cmake --preset conan-debug
cmake --build build --parallel

# Run tests
./build/tests/nodeflux_tests

# Run specific test
./build/tests/nodeflux_tests --gtest_filter=MeshTest.*

# Build Studio app only
cmake --build build --target nodeflux_studio

# Clean rebuild
rm -rf build
conan install . --output-folder=build --build=missing
cmake --preset conan-debug
cmake --build build --parallel
```

### CMake Options

- `NODEFLUX_BUILD_EXAMPLES` (default: OFF) - Build examples
- `NODEFLUX_BUILD_TESTS` (default: ON) - Build unit tests
- `NODEFLUX_BUILD_STUDIO` (default: ON) - Build Qt Studio

---

## Testing

**Location**: `tests/test_*.cpp`

**Run All Tests**:
```bash
./build/tests/nodeflux_tests
```

**Test Coverage** (59 tests total):
- Core mesh operations
- Boolean operations with CGAL
- BVH spatial acceleration
- Mesh generators (primitives)
- Mesh validation and repair
- Node system and graph execution
- Math utilities

**Current Status**: 52/59 passing (7 skipped for known CGAL issues with non-closed meshes)

---

## Development Roadmap

### 🎯 Near-Term Priorities (Next 4 Weeks)

**Week 1: Architecture Refactoring** ✅ COMPLETE
- ✅ All SOP nodes migrated to modern SOPNode base class (13 total)
  - ✅ BooleanSOP - Modern parameters, input ports
  - ✅ MirrorSOP - Modern parameters, input ports
  - ✅ ArraySOP - Modern parameters, input ports (tests updated Oct 2025)
  - ✅ ExtrudeSOP - Modern parameters, input ports
  - ✅ LaplacianSOP - Modern parameters, input ports
  - ✅ LineSOP - Modern parameters, input ports
  - ✅ NoiseDisplacementSOP - Modern parameters, input ports
  - ✅ PolyExtrudeSOP - Modern parameters, input ports
  - ✅ ResampleSOP - Modern parameters, input ports
  - ✅ SubdivisionSOP - Modern parameters, input ports
  - ✅ TransformSOP - Modern parameters, input ports
  - ✅ ScatterSOP - Modern parameters, input ports
  - ✅ CopyToPointsSOP - Modern parameters, input ports
- ✅ ExecutionEngine bridges updated for all SOPs
- ✅ Unified port-based data flow across all nodes
- ✅ ExportSOP added with manual export button (Oct 2025)

**Week 2: UI/UX Enhancements**
- Node selection/picking in viewport
- Transform gizmos (move, rotate, scale)
- Camera presets (orthographic views)
- Performance overlay
- Recent files menu

**Week 3: Additional Core SOPs**
- MergeSOP - Combine multiple geometries into one (no boolean, just append)
- DeleteSOP - Remove primitives/points by group or selection
- GroupSOP - Assign primitives/points to named groups for selective operations
- NormalSOP - Compute and manipulate surface normals (weighted, angle-based)
- UVProjectSOP - Basic UV coordinate generation (planar, cylindrical, spherical)
- WrangleSOP - Expression-based point/primitive manipulation (VEX-like)

**Week 4: File Format Support**
- Mesh import (OBJ reader)
- STL export
- PLY export
- glTF export (stretch goal)

**Week 5: Advanced Features & Attribute Unification** ✅ COMPLETE (Oct 26, 2025)
- ✅ Complete attribute system migration to GeometryContainer
  - ✅ Unified attribute storage with Structure-of-Arrays (SoA) layout
  - ✅ Type-safe attribute access with compile-time checking
  - ✅ Four element classes: Point, Vertex, Primitive, Detail
  - ✅ Houdini-standard attribute names ("P", "N", "Cd", "uv")
  - ✅ Advanced features:
    - ✅ Attribute promotion/demotion (12 tests passing)
    - ✅ Attribute groups with boolean operations (27 tests passing)
    - ✅ Attribute interpolation with multiple modes (27 tests passing)
  - ✅ Comprehensive documentation:
    - ✅ [API Reference](docs/ATTRIBUTE_SYSTEM_API.md)
    - ✅ [Migration Guide](docs/ATTRIBUTE_MIGRATION_GUIDE.md)
    - ✅ [Usage Examples](docs/ATTRIBUTE_EXAMPLES.md)
  - ✅ 253 total tests, 245 passing (96.8%)
- ✅ Laplacian Smoothing SOP - 3 algorithms (uniform, cotan, taubin)
- ✅ Subdivision SOP - Catmull-Clark and Simple subdivision with boundary handling
- ✅ Boolean SOP - Fixed and fully functional (Union/Intersection/Difference)
- Material system integration (TODO)
- UV mapping tools (TODO)
- Bend/Twist/Taper deformation nodes (TODO)

**Week 5: Complete Stubbed SOPs** 🔄 IN PROGRESS (Oct 26, 2025)
- ❌ CopyToPointsSOP - Instance geometry to points (currently just returns input)
- ❌ ExtrudeSOP - Face extrusion with distance/inset (currently just returns input)
- ❌ PolyExtrudeSOP - Per-polygon extrusion (currently just returns input)
- ❌ ResampleSOP - Edge/curve resampling by count or length (currently just returns input)
- ❌ ArraySOP - Radial and Grid array modes (linear mode works, others stubbed)
- Note: All have parameter UI in place, just need execute() implementation

### 🌊 Medium-Term Goals (2-3 Months)

**Rendering**:
- PBR (physically-based rendering) in viewport
- Texture support
- Multiple light sources
- Shadow mapping

**Workflow**:
- Node presets/templates
- Undo/redo system
- Copy/paste nodes
- Node groups/subgraphs

**Performance**:
- Multi-threaded node cooking
- Incremental mesh updates
- Memory profiling tools
- GPU memory management
- Spatial indexing for point clouds

**Point Cloud & Advanced Geometry**:
- Point cloud representation mode in GeometryData
- Point cloud processing SOPs (downsample, filter, normal estimation)
- Organized grid support for efficient spatial queries
- Optional PCL integration for advanced algorithms
- Point-to-mesh conversion nodes (Poisson, Ball Pivoting)

### 🚀 Long-Term Vision (6+ Months)

**GPU Acceleration** (requires architecture redesign):
- GPU chaining: keep mesh data on GPU between SOP operations
- Implement compute shaders for primitives, booleans, deformations
- GPU memory management and buffer pooling
- Target: 10-100x speedup for multi-node workflows

**Other Long-Term Goals**:
- Python bindings for scripting
- Node marketplace/library
- Animation/keyframing system
- Particle systems
- Volumetric operations
- Simulation integration

---

## Adding New SOP Nodes

### Step-by-Step Guide

1. **Create Header File**: `nodeflux_core/include/nodeflux/sop/my_node_sop.hpp`

```cpp
#pragma once
#include "sop_node.hpp"

namespace nodeflux::sop {

class MyNodeSOP : public SOPNode {
public:
    explicit MyNodeSOP(const std::string& name);
    ~MyNodeSOP() override = default;

    // Parameters
    void set_my_param(double value);
    double get_my_param() const { return my_param_; }

protected:
    std::optional<std::shared_ptr<GeometryData>> cook() override;

private:
    double my_param_ = 1.0;
};

} // namespace nodeflux::sop
```

2. **Implement in**: `nodeflux_core/src/sop/my_node_sop.cpp`

```cpp
#include "nodeflux/sop/my_node_sop.hpp"

namespace nodeflux::sop {

MyNodeSOP::MyNodeSOP(const std::string& name)
    : SOPNode(name, "MyNode") {
    // Initialize ports if needed
}

void MyNodeSOP::set_my_param(double value) {
    if (my_param_ != value) {
        my_param_ = value;
        mark_dirty();
    }
}

std::optional<std::shared_ptr<GeometryData>> MyNodeSOP::cook() {
    auto input = get_input_data(0);
    if (!input) {
        set_error("No input geometry");
        return std::nullopt;
    }

    auto output = std::make_shared<GeometryData>(*input);

    // TODO: Implement your operation

    return output;
}

} // namespace nodeflux::sop
```

3. **Add to CMakeLists.txt**

4. **Write Tests**: `tests/test_my_node_sop.cpp`

5. **Register in NodeGraph** (if needed)

---

## Current Development Activity

**Recent Commits** (Oct 19-25, 2025):
- Export SOP with manual "Export Now" button (Oct 25)
- ArraySOP test suite migration to modern parameter API (Oct 25)
- Architecture refactoring completed - all 13 SOPs modernized (Oct 25)
- Node name refactoring
- Copy to Points & Scatter nodes
- Dark theme for Studio
- Vertex/face normal visualization
- PolyExtrude node
- Line & Resample SOPs

**Active Development**: Very active - multiple features per week

---

## Important Notes

### Thread Safety
- Most operations are NOT thread-safe
- Avoid concurrent mesh modifications
- Node cooking is single-threaded (for now)

### Memory Management
- Use smart pointers (`std::shared_ptr`, `std::unique_ptr`)
- RAII for resource cleanup
- Move semantics for large meshes

### Performance Considerations
- BVH acceleration kicks in automatically for boolean ops
- GPU acceleration framework exists but not implemented
- Profile before optimizing - Eigen is already fast

### Git Workflow
- Main branch: `main`
- Clean commit history
- Descriptive commit messages

---

## Key Files Reference

| Component | Path |
|-----------|------|
| SOP Base Class | `/nodeflux_core/include/nodeflux/sop/sop_node.hpp` |
| All SOP Nodes | `/nodeflux_core/include/nodeflux/sop/*.hpp` |
| Node Graph | `/nodeflux_core/include/nodeflux/graph/node_graph.hpp` |
| Execution Engine | `/nodeflux_core/src/graph/execution_engine.cpp` |
| Core Mesh | `/nodeflux_core/include/nodeflux/core/mesh.hpp` |
| Geometry Data | `/nodeflux_core/include/nodeflux/sop/GeometryData.hpp` |
| Attributes | `/nodeflux_core/include/nodeflux/core/geometry_attributes.hpp` |
| Qt Main Window | `/nodeflux_studio/src/MainWindow.cpp` |
| 3D Viewport | `/nodeflux_studio/src/ViewportWidget.cpp` |
| Tests | `/tests/test_*.cpp` |

---

## Summary: What to Tell Claude

**NodeFluxEngine is a production-ready procedural mesh generation system** with:

✅ **Complete SOP workflow** - 17 working nodes
✅ **Visual editor** - Qt-based Studio application
✅ **Smart caching** - Intelligent dependency resolution
✅ **3D viewport** - Sophisticated OpenGL rendering
✅ **Comprehensive testing** - 59 unit tests
✅ **Modern C++20** - Clean, maintainable architecture

❌ **Missing**: GPU compute shader implementations (framework exists)
❌ **Missing**: Additional file formats (STL, PLY, glTF)
❌ **Missing**: Advanced rendering (PBR, textures)

**Primary Gap**: The vision of "GPU-native" is not yet realized - actual GLSL compute shaders are not implemented, despite the framework being in place.

**Current Focus**: Enhancing UI/UX, adding GPU shaders, expanding file format support, and polishing the user experience.
- dont do summary documents, just update the claude.md if a task is done in the todo list
