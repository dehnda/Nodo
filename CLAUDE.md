# CLAUDE.md - Developer Reference# CLAUDE.md - Developer Reference



> **For strategic planning and roadmap:** See [ROADMAP.md](ROADMAP.md)  > **For strategic planning and roadmap:** See [ROADMAP.md](ROADMAP.md)  

> **For documentation navigation:** See [docs/NAVIGATION.md](docs/NAVIGATION.md)> **For documentation navigation:** See [docs/NAVIGATION.md](docs/NAVIGATION.md)



This file provides technical reference for developers working on Nodo's codebase.This file provides technical reference for developers working on Nodo's codebase.



------



## Quick Project Context## Quick Project Context



**Nodo** is a C++20 procedural modeling application with:**Nodo** is a C++20 procedural modeling application with:

- **44 node types** across 7 categories (geometry, transform, boolean, etc.)- **44 node types** across 7 categories (geometry, transform, boolean, etc.)

- **Qt-based Studio** with visual node editor and 3D viewport- **Qt-based Studio** with visual node editor and 3D viewport

- **nodo_core library** (pure C++, no Qt) + **nodo_studio** (Qt UI)- **nodo_core library** (pure C++, no Qt) + **nodo_studio** (Qt UI)

- **Manifold geometry kernel** for robust boolean operations- **Manifold geometry kernel** for robust boolean operations

- **Modern attribute system** with point/primitive/vertex/detail classes- **Modern attribute system** with point/primitive/vertex/detail classes



**Current Phase (Oct 2025):** Phase 1 - Implementing backend parameters + property panel UI**Current Phase (Oct 2025):** Phase 1 - Implementing backend parameters + property panel UI



------



## Code Style Guide## Code Style Guide



### Naming Conventions### Naming Conventions



- **Variables/Functions**: `snake_case` (e.g., `vertex_count`, `calculate_bounds()`)- **Variables/Functions**: `snake_case` (e.g., `vertex_count`, `calculate_bounds()`)

- **Types/Classes**: `PascalCase` (e.g., `GeometryData`, `SOPNode`)- **Types/Classes**: `PascalCase` (e.g., `GeometryData`, `SOPNode`)

- **Constants**: `UPPER_SNAKE_CASE` (e.g., `DEFAULT_RADIUS`)- **Constants**: `UPPER_SNAKE_CASE` (e.g., `DEFAULT_RADIUS`)

- **Private Members**: trailing underscore (e.g., `vertex_count_`, `cache_`)- **Private Members**: trailing underscore (e.g., `vertex_count_`, `cache_`)

- **Namespaces**: `lowercase` (e.g., `nodo::core`, `nodo::sop`)- **Namespaces**: `lowercase` (e.g., `nodo::core`, `nodo::sop`)



### C++ Standards & Patterns### C++ Standards & Patterns



**Use C++20 Features**:**Use C++20 Features**:

```cpp```cpp

// ✅ GOOD// ✅ GOOD

std::optional<Mesh> result = generate_mesh();std::optional<Mesh> result = generate_mesh();

if (result.has_value()) { /* use result */ }if (result.has_value()) { /* use result */ }



auto [vertices, faces] = extract_geometry();  // structured bindingsauto [vertices, faces] = extract_geometry();  // structured bindings



if (auto input = get_input(); input) { /* use input */ }  // if-initif (auto input = get_input(); input) { /* use input */ }  // if-init



// ❌ BAD - Don't use C++23 features// ❌ BAD - Don't use C++23 features

std::expected<Mesh, Error> result;  // ❌ Not available in C++20std::expected<Mesh, Error> result;  // ❌ Not available in C++20

``````



**Error Handling Pattern**:**Error Handling Pattern**:

```cpp```cpp

// Return std::optional for operations that can fail// Return std::optional for operations that can fail

std::optional<std::shared_ptr<GeometryData>> cook() override {std::optional<std::shared_ptr<GeometryData>> cook() override {

    auto input = get_input_data(0);    auto input = get_input_data(0);

    if (!input) {    if (!input) {

        set_error("No input geometry");        set_error("No input geometry");

        return std::nullopt;  // ✅        return std::nullopt;  // ✅

    }    }

    // ... process ...    // ... process ...

    return output;    return output;

}}

``````



**Style Requirements**:**Style Requirements**:

```cpp```cpp

// ✅ GOOD// ✅ GOOD

float radius = 2.0F;           // Uppercase Ffloat radius = 2.0F;           // Uppercase F

if (ptr != nullptr) { }        // Explicit nullptr checkif (ptr != nullptr) { }        // Explicit nullptr check

MyClass::static_method();      // Static methods via classMyClass::static_method();      // Static methods via class



// ❌ BAD// ❌ BAD

float radius = 2.0f;           // Lowercase ffloat radius = 2.0f;           // Lowercase f

if (ptr) { }                   // Implicit pointer checkif (ptr) { }                   // Implicit pointer check

instance.static_method();      // Don't call static via instanceinstance.static_method();      // Don't call static via instance

``````



### Performance Guidelines### Performance Guidelines



- Use **Eigen** for vectorized math (avoid manual loops)- Use **Eigen** for vectorized math (avoid manual loops)

- **Reserve** container capacity when size is known- **Reserve** container capacity when size is known

- Prefer **move semantics** for large data (`std::move`)- Prefer **move semantics** for large data (`std::move`)

- **Profile before optimizing** (use benchmarks)- **Profile before optimizing** (use benchmarks)

- Use **const references** for input parameters: `const Mesh&`- Use **const references** for input parameters: `const Mesh&`



------



## Architecture Overview## Architecture Overview



### Project Structure### Project Structure



``````

nodo_core/           # Pure C++ library (NO Qt dependencies)nodo_core/           # Pure C++ library (NO Qt dependencies)

├── include/nodo/├── include/nodo/

│   ├── core/        # Mesh, GeometryContainer, attributes│   ├── core/        # Mesh, GeometryContainer, attributes

│   ├── sop/         # SOPNode base class + all node types│   ├── sop/         # SOPNode base class + all node types

│   └── graph/       # NodeGraph, execution engine│   └── graph/       # NodeGraph, execution engine

└── src/             # Implementations└── src/             # Implementations



nodo_studio/         # Qt GUI applicationnodo_studio/         # Qt GUI application

├── src/├── src/

│   ├── MainWindow.cpp       # Main application window│   ├── MainWindow.cpp       # Main application window

│   ├── NodeGraphWidget.cpp  # Visual node editor│   ├── NodeGraphWidget.cpp  # Visual node editor

│   ├── ViewportWidget.cpp   # 3D OpenGL viewport│   ├── ViewportWidget.cpp   # 3D OpenGL viewport

│   └── PropertyPanel.cpp    # Node parameter UI│   └── PropertyPanel.cpp    # Node parameter UI

└── resources/       # Icons, shaders, UI resources└── resources/       # Icons, shaders, UI resources



tests/               # Google Test unit teststests/               # Google Test unit tests

docs/                # Documentation, design conceptsdocs/                # Documentation, design concepts

``````



### Key Design Patterns### Key Design Patterns



**SOP Node Pattern**:**SOP Node Pattern**:

```cpp```cpp

class MyCustomSOP : public SOPNode {class MyCustomSOP : public SOPNode {

public:public:

    MyCustomSOP(const std::string& name) : SOPNode(name, "MyCustom") {    MyCustomSOP(const std::string& name) : SOPNode(name, "MyCustom") {

        cook_setup();  // Define parameters        cook_setup();  // Define parameters

    }    }



protected:protected:

    void cook_setup() override {    void cook_setup() override {

        // Define parameters with fluent API        // Define parameters with fluent API

        define_float_parameter("radius", 1.0F)        define_float_parameter("radius", 1.0F)

            .label("Radius")            .label("Radius")

            .range(0.01F, 100.0F)            .range(0.01F, 100.0F)

            .description("Controls sphere radius");            .description("Controls sphere radius");

    }    }



    std::optional<std::shared_ptr<GeometryData>> cook() override {    std::optional<std::shared_ptr<GeometryData>> cook() override {

        // Get input        // Get input

        auto input = get_input_data(0);        auto input = get_input_data(0);

        if (!input) {        if (!input) {

            set_error("No input geometry");            set_error("No input geometry");

            return std::nullopt;            return std::nullopt;

        }        }



        // Get parameters        // Get parameters

        float radius = get_parameter<float>("radius");        float radius = get_parameter<float>("radius");



        // Create output        // Create output

        auto output = std::make_shared<GeometryData>(*input);        auto output = std::make_shared<GeometryData>(*input);

                

        // TODO: Modify geometry        // TODO: Modify geometry

                

        return output;        return output;

    }    }

};};

``````



**Data Flow Pattern**:**Data Flow Pattern**:

``````

User modifies parameter → Node marked dirty →User modifies parameter → Node marked dirty →

Cook input dependencies → Execute node.cook() →Cook input dependencies → Execute node.cook() →

Update output cache → Propagate to connected nodesUpdate output cache → Propagate to connected nodes

``````



**Caching**: Automatic - node only re-executes when:**Caching**: Automatic - node only re-executes when:

- Input data changes- Input data changes

- Parameters change via `mark_dirty()`- Parameters change via `mark_dirty()`

- Manually forced- Manually forced



------



## Core Data Structures## Core Data Structures



### GeometryContainer (Modern)### GeometryContainer (Modern)

Primary geometry container with Houdini-style attributes:Primary geometry container with Houdini-style attributes:

```cpp```cpp

GeometryContainer container;GeometryContainer container;



// Add attributes// Add attributes

container.addPointAttribute<Vec3f>("P");     // Position (standard)container.addPointAttribute<Vec3f>("P");     // Position (standard)

container.addPointAttribute<Vec3f>("N");     // Normalcontainer.addPointAttribute<Vec3f>("N");     // Normal

container.addPointAttribute<Vec3f>("Cd");    // Colorcontainer.addPointAttribute<Vec3f>("Cd");    // Color

container.addPrimitiveAttribute<int>("material_id");container.addPrimitiveAttribute<int>("material_id");



// Access attributes// Access attributes

auto& positions = container.getPointAttribute<Vec3f>("P");auto& positions = container.getPointAttribute<Vec3f>("P");

positions[0] = Vec3f(1.0f, 2.0f, 3.0f);positions[0] = Vec3f(1.0f, 2.0f, 3.0f);



// Groups// Groups

container.addPointGroup("selected_points");container.addPointGroup("selected_points");

container.addPrimitiveGroup("material_1");container.addPrimitiveGroup("material_1");

``````



See [docs/ATTRIBUTE_SYSTEM_API.md](docs/ATTRIBUTE_SYSTEM_API.md) for complete API.See [docs/ATTRIBUTE_SYSTEM_API.md](docs/ATTRIBUTE_SYSTEM_API.md) for complete API.



### SOPNode Parameter System### SOPNode Parameter System



```cpp```cpp

// In cook_setup():// In cook_setup():

define_float_parameter("radius", 1.0F)define_float_parameter("radius", 1.0F)

    .label("Radius")                      // UI display name    .label("Radius")                      // UI display name

    .range(0.01F, 100.0F)                // Min/max    .range(0.01F, 100.0F)                // Min/max

    .description("Sphere radius")         // Tooltip    .description("Sphere radius")         // Tooltip

    .category("Size");                    // Section grouping    .category("Size");                    // Section grouping



define_int_parameter("mode", 0)define_int_parameter("mode", 0)

    .label("Type")    .label("Type")

    .options({"UV Sphere", "Icosphere"})  // Dropdown    .options({"UV Sphere", "Icosphere"})  // Dropdown

    .category("Mode");    .category("Mode");



define_vector3_parameter("center", {0, 0, 0})define_vector3_parameter("center", {0, 0, 0})

    .label("Center")    .label("Center")

    .description("Sphere center position");    .description("Sphere center position");



// In cook():// In cook():

float radius = get_parameter<float>("radius");float radius = get_parameter<float>("radius");

int mode = get_parameter<int>("mode");int mode = get_parameter<int>("mode");

Vec3f center = get_parameter<Vec3f>("center");Vec3f center = get_parameter<Vec3f>("center");

``````



------



## Adding a New SOP Node## Adding a New SOP Node



### Step 1: Create Header File### Step 1: Create Header File

`nodo_core/include/nodo/sop/nodes/my_node_sop.hpp`:`nodo_core/include/nodo/sop/nodes/my_node_sop.hpp`:

```cpp```cpp

#pragma once#pragma once



#include "nodo/sop/sop_node.hpp"#include "nodo/sop/sop_node.hpp"



namespace nodo::sop {namespace nodo::sop {



class MyNodeSOP : public SOPNode {class MyNodeSOP : public SOPNode {

public:public:

    explicit MyNodeSOP(const std::string& name);    explicit MyNodeSOP(const std::string& name);

    ~MyNodeSOP() override = default;    ~MyNodeSOP() override = default;



    // Node metadata    // Node metadata

    static constexpr int NODE_VERSION = 1;    static constexpr int NODE_VERSION = 1;

    int getVersion() const override { return NODE_VERSION; }    int getVersion() const override { return NODE_VERSION; }



protected:protected:

    void cook_setup() override;    void cook_setup() override;

    std::optional<std::shared_ptr<GeometryData>> cook() override;    std::optional<std::shared_ptr<GeometryData>> cook() override;



private:private:

    // Parameters (cached for performance)    // Parameters (cached for performance)

    float my_param_ = 1.0F;    float my_param_ = 1.0F;

};};



} // namespace nodo::sop} // namespace nodo::sop

``````



### Step 2: Create Implementation### Step 2: Create Implementation

`nodo_core/src/sop/nodes/my_node_sop.cpp`:`nodo_core/src/sop/nodes/my_node_sop.cpp`:

```cpp```cpp

#include "nodo/sop/nodes/my_node_sop.hpp"#include "nodo/sop/nodes/my_node_sop.hpp"



namespace nodo::sop {namespace nodo::sop {



MyNodeSOP::MyNodeSOP(const std::string& name)MyNodeSOP::MyNodeSOP(const std::string& name)

    : SOPNode(name, "MyNode") {    : SOPNode(name, "MyNode") {

    cook_setup();    cook_setup();

}}



void MyNodeSOP::cook_setup() {void MyNodeSOP::cook_setup() {

    // Universal parameter (inherited from SOPNode)    // Universal parameter (inherited from SOPNode)

    // Group parameter is automatic    // Group parameter is automatic

        

    // Define node-specific parameters    // Define node-specific parameters

    define_float_parameter("my_param", 1.0F)    define_float_parameter("my_param", 1.0F)

        .label("My Parameter")        .label("My Parameter")

        .range(0.0F, 10.0F)        .range(0.0F, 10.0F)

        .description("Controls my operation")        .description("Controls my operation")

        .category("Parameters");        .category("Parameters");

}}



std::optional<std::shared_ptr<GeometryData>> MyNodeSOP::cook() {std::optional<std::shared_ptr<GeometryData>> MyNodeSOP::cook() {

    auto input = get_input_data(0);    auto input = get_input_data(0);

    if (!input) {    if (!input) {

        set_error("No input geometry");        set_error("No input geometry");

        return std::nullopt;        return std::nullopt;

    }    }



    // Get parameters    // Get parameters

    my_param_ = get_parameter<float>("my_param");    my_param_ = get_parameter<float>("my_param");

        

    // Create output (copy input)    // Create output (copy input)

    auto output = std::make_shared<GeometryData>(*input);    auto output = std::make_shared<GeometryData>(*input);

        

    // TODO: Implement your geometry operation    // TODO: Implement your geometry operation

    // Example: scale all points    // Example: scale all points

    auto& container = output->container;    auto& container = output->container;

    auto& positions = container.getPointAttribute<Vec3f>("P");    auto& positions = container.getPointAttribute<Vec3f>("P");

    for (size_t i = 0; i < positions.size(); ++i) {    for (size_t i = 0; i < positions.size(); ++i) {

        positions[i] *= my_param_;        positions[i] *= my_param_;

    }    }

        

    return output;    return output;

}}



} // namespace nodo::sop} // namespace nodo::sop

``````



### Step 3: Add to CMakeLists.txt### Step 3: Add to CMakeLists.txt

Add your files to `nodo_core/CMakeLists.txt`.Add your files to `nodo_core/CMakeLists.txt`.



### Step 4: Write Tests### Step 4: Write Tests

`tests/test_my_node_sop.cpp`:`tests/test_my_node_sop.cpp`:

```cpp```cpp

#include <gtest/gtest.h>#include <gtest/gtest.h>

#include "nodo/sop/nodes/my_node_sop.hpp"#include "nodo/sop/nodes/my_node_sop.hpp"



TEST(MyNodeSOPTest, BasicFunctionality) {TEST(MyNodeSOPTest, BasicFunctionality) {

    auto node = std::make_shared<nodo::sop::MyNodeSOP>("test");    auto node = std::make_shared<nodo::sop::MyNodeSOP>("test");

    node->set_parameter("my_param", 2.0F);    node->set_parameter("my_param", 2.0F);

        

    // ... test logic ...    // ... test logic ...

        

    EXPECT_TRUE(result.has_value());    EXPECT_TRUE(result.has_value());

}}

``````



### Step 5: Register in Studio (Optional)### Step 5: Register in Studio (Optional)

Add to node factory in `nodo_studio` if you want it in the UI.Add to node factory in `nodo_studio` if you want it in the UI.



------

- `file_sop.hpp` - File import (OBJ support) ✅

## Build System- `export_sop.hpp` - File export with manual export button ✅



### Common Commands**Deformation/Modifiers (5 nodes)**:

- `extrude_sop.hpp` - Face extrusion with caps (normal/uniform modes) ✅

```bash- `polyextrude_sop.hpp` - Per-polygon extrusion (individual/connected) ✅

# Initial setup- `laplacian_sop.hpp` - 3 smoothing algorithms (uniform, cotan, taubin) ✅

conan install . --output-folder=build --build=missing- `subdivision_sop.hpp` - Catmull-Clark and Simple subdivision ✅

cmake --preset conan-debug- `resample_sop.hpp` - Edge/curve resampling (by count/length) ✅



# Build everything**Expression/Code (1 node)**:

cmake --build build --parallel- `wrangle_sop.hpp` - Expression-based manipulation with exprtk ✅ (Oct 27, 2025)

  - Attribute access: @P, @N, @Cd, @ptnum, @numpt

# Build specific target  - Mathematical expressions for per-point/primitive operations

cmake --build build --target nodo_studio  - Automatic preprocessing of syntax (@P.y → Py, = → :=)



# Run tests**Array/Duplication (3 nodes)**:

./build/tests/nodo_tests- `array_sop.hpp` - Linear, radial, and grid duplication patterns ✅

- `scatter_sop.hpp` - Random point distribution ✅

# Run specific test- `copy_to_points_sop.hpp` - Instance geometry to points ✅

./build/tests/nodo_tests --gtest_filter=SphereSOPTest.*

**Utility/Selection (3 nodes)**:

# Clean rebuild- `delete_sop.hpp` - Remove elements by group/pattern ✅

rm -rf build- `group_sop.hpp` - Assign elements to named groups ✅

# ... then repeat setup steps- `normal_sop.hpp` - Compute and manipulate normals ✅



# Release build**UV/Texture (1 node)**:

cmake --preset conan-release- `uv_unwrap_sop.hpp` - UV mapping with xatlas (full seam control) ✅

cmake --build build --config Release

```**Boolean/Transform (3 nodes)**:

- `boolean_sop.hpp` - Union/Intersection/Difference with Manifold ✅ (Oct 26, 2025)

### Dependencies (Auto-managed by Conan)- `mirror_sop.hpp` (194 lines) - Mirror across planes

- **Manifold** - Robust geometry kernel- Transform nodes (built into system)

- **Qt 6.7.3** - GUI framework  

- **Eigen3 3.4.0** - Linear algebra**Advanced (1 node)**:

- **Google Test 1.14.0** - Unit testing- `noise_displacement_sop.hpp` - Procedural noise deformation

- **fmt**, **nlohmann_json** - Utilities

### ✅ Node Graph System (Complete)

---

**Location**: `nodeflux_core/include/nodeflux/graph/`

## Key File Locations

**NodeGraph (`node_graph.hpp`)** - 272 lines:

| Component | Path |- Pure data model (separate from UI)

|-----------|------|- Supports 24+ node types

| SOP Base Class | `nodo_core/include/nodo/sop/sop_node.hpp` |- Add/remove/connect nodes

| All SOP Nodes | `nodo_core/include/nodo/sop/nodes/*.hpp` |- Topological sorting for execution

| Node Graph | `nodo_core/include/nodo/graph/node_graph.hpp` |- JSON serialization via `GraphSerializer`

| Execution Engine | `nodo_core/src/graph/execution_engine.cpp` |

| Geometry Container | `nodo_core/include/nodo/core/geometry_container.hpp` |**ExecutionEngine (`execution_engine.cpp`)**:

| Geometry Data | `nodo_core/include/nodo/sop/geometry_data.hpp` |- Smart dependency resolution

| Qt Main Window | `nodo_studio/src/MainWindow.cpp` |- Incremental updates (only rebuild changed branches)

| Node Graph Widget | `nodo_studio/src/NodeGraphWidget.cpp` |- Caching at node level

| 3D Viewport | `nodo_studio/src/ViewportWidget.cpp` |- Error propagation

| Property Panel | `nodo_studio/src/PropertyPanel.cpp` |

| Tests | `tests/test_*.cpp` |### ✅ Qt Studio Application (Fully Functional)



---**Location**: `nodeflux_studio/src/`



## Important Notes**Features**:

- Visual node graph editor with drag-and-drop

### Thread Safety- Property panel for parameter editing

- Most operations are NOT thread-safe- 3D viewport with OpenGL rendering

- Avoid concurrent mesh modifications- Scene save/load (JSON format)

- Node cooking is single-threaded (for now)- Dark theme (Oct 19, 2025)

- Display/render/bypass flags (Houdini-style)

### Memory Management

- Use smart pointers (`std::shared_ptr`, `std::unique_ptr`)**ViewportWidget** - 1112 lines of sophisticated rendering:

- RAII for resource cleanup- Blinn-Phong shading with lighting

- Move semantics for large meshes- Wireframe mode

- Vertex/edge/face normal visualization

### API Stability (Phase 1 Goal)- Grid and axis display

- **Parameter names are stable IDs** - never change after release- Camera controls (orbit, pan, zoom)

- **Labels can change** - they're just display strings- Point cloud rendering

- **Add node versions** - `static constexpr int NODE_VERSION = 1;`- Multisampling (4x MSAA)

- **Keep Qt out of nodo_core** - verify with `grep -r "Q[A-Z]" nodo_core/`

### ✅ Spatial Acceleration (Complete)

### Git Workflow

- Main branch: `main`**BVH System**:

- Clean commit history- 45x speedup over brute-force for boolean operations

- Descriptive commit messages- Used automatically by BooleanSOP

- Don't create summary documents unless requested- Bounding volume hierarchy for ray/mesh intersection



---### ✅ Import/Export (OBJ Complete)



*For more information, see [ROADMAP.md](ROADMAP.md) and [docs/NAVIGATION.md](docs/NAVIGATION.md)***OBJ Format** (`nodeflux_core/include/nodeflux/io/`):

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

- OBJ import (export works!)
- STL export
- PLY export
- glTF export

### ❌ Advanced Features Still TODO

- WrangleSOP - Expression-based manipulation (VEX-like scripting)
- Material system (attributes exist, not integrated)
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

**Week 3: Additional Core SOPs** ✅ COMPLETE (Oct 27, 2025)
- ✅ MergeSOP - Combine multiple geometries into one (no boolean, just append)
- ✅ DeleteSOP - Remove primitives/points by group or selection
- ✅ GroupSOP - Assign primitives/points to named groups for selective operations
- ✅ NormalSOP - Compute and manipulate surface normals (weighted, angle-based)
- ✅ UVUnwrapSOP - UV coordinate generation with xatlas (with seam control parameters)
- ✅ ScatterSOP - Random point distribution with seed control
- ✅ CopyToPointsSOP - Instance geometry to points
- ✅ ArraySOP - Linear, radial, and grid duplication patterns (all modes working)
- ✅ WrangleSOP - Expression-based point/primitive manipulation using exprtk
  - Attribute access: @P, @N, @Cd, @ptnum, @numpt
  - Mathematical expressions with full math library support
  - 5 comprehensive tests passing

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

**Week 5: Complete Stubbed SOPs** ✅ COMPLETE (Oct 27, 2025)
- ✅ CopyToPointsSOP - Instance geometry to points (fully implemented)
- ✅ ExtrudeSOP - Face extrusion with distance/inset (fully implemented)
- ✅ PolyExtrudeSOP - Per-polygon extrusion (fully implemented)
- ✅ ResampleSOP - Edge/curve resampling by count or length (fully implemented)
- ✅ ArraySOP - Radial and Grid array modes (all modes fully implemented)
- Note: All SOPs have complete implementations with proper parameter UI

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
