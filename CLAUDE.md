# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

NodeFluxEngine is a modern C++20 GPU-accelerated procedural mesh generation library with a Houdini-inspired SOP (Surface Operator) workflow. The project provides node-based procedural modeling with real-time performance through GPU compute shaders and intelligent caching.

## Build System

### Prerequisites
- CMake 3.20+
- C++20 compatible compiler (GCC 11+, Clang 12+, MSVC 2022+)
- Conan 2.x for dependency management
- Python 3.8+ (for Conan)

### Common Build Commands

```bash
# Initial setup - install dependencies with Conan
conan install . --output-folder=build --build=missing

# Configure build with CMake (generates CMakePresets.json)
cmake --preset conan-debug

# Build the project
cmake --build build --parallel

# Run tests
./build/tests/nodeflux_tests

# Run specific test suite
./build/tests/nodeflux_tests --gtest_filter=MeshTest.*

# Build only the Studio Qt application
cmake --build build --target nodeflux_studio

# Clean and rebuild from scratch
rm -rf build
conan install . --output-folder=build --build=missing
cmake --preset conan-debug
cmake --build build --parallel
```

### CMake Build Options
- `NODEFLUX_BUILD_EXAMPLES` (default: OFF) - Build example applications
- `NODEFLUX_BUILD_TESTS` (default: ON) - Build unit tests
- `NODEFLUX_BUILD_STUDIO` (default: ON) - Build Qt Studio application

### Dependencies (Managed by Conan)
- **Eigen3 3.4.0** - Linear algebra and vectorized operations
- **CGAL 5.6.1** - Boolean operations and computational geometry
- **Qt 6.7.3** - GUI framework for Studio application
- **Google Test 1.14.0** - Testing framework
- **fmt 10.2.1** - String formatting
- **nlohmann_json 3.11.3** - JSON serialization
- **GMP/MPFR** - Multiple precision arithmetic (CGAL dependency)

## Architecture

### Core Module Hierarchy

```
nodeflux/
├── core/           - Fundamental data structures (Mesh, Point, Vector, GeometryAttributes)
├── geometry/       - Mesh generators (primitives, boolean ops, validation, repair)
├── spatial/        - Spatial acceleration (BVH with 45x speedup)
├── sop/            - Surface Operator node system (procedural workflow)
├── graph/          - Clean node graph architecture (data model, execution, serialization)
├── nodes/          - Primitive node implementations (Sphere, Box, Cylinder, etc.)
└── io/             - Import/export (OBJ format, future: STL, glTF)
```

### Key Architecture Patterns

**SOP Node System** - Houdini-inspired procedural workflow:
- `SOPNode` - Base class for all procedural nodes with caching and dependency tracking
- `GeometryData` - Unified container for mesh data and attributes passed between nodes
- `NodePort` - Connection system for data flow between nodes
- `ExecutionEngine` - Smart dependency resolution and incremental updates

**Data Flow**:
```
User modifies parameter → Node marked dirty → Cook input dependencies →
Execute node → Update output cache → Propagate to connected nodes
```

**Error Handling**:
- Use `std::optional<T>` for operations that can fail (NOT `std::expected` - C++23 only)
- Thread-local error storage for detailed error information via `nodeflux::core::Error`
- Node execution states: CLEAN, DIRTY, COMPUTING, ERROR

### Core Classes

**Mesh (`nodeflux::core::Mesh`)**: Primary geometry container
- Uses Eigen matrices for vertices and faces
- Stores positions, normals, and topology
- Thread-safe and move-optimized

**SOPNode (`nodeflux::sop::SOPNode`)**: Base class for all procedural nodes
- Automatic dependency cooking and caching
- Parameter system with typed values
- Input/output port management
- Execution state tracking with timing info

**GeometryData (`nodeflux::sop::GeometryData`)**: Data carrier between nodes
- Contains mesh data plus attribute layers (vertex, face, global)
- Supports attribute transfer and interpolation
- Clone and merge operations for procedural workflows

**NodeGraph (`nodeflux::graph::NodeGraph`)**: Pure data model
- Node and connection management
- JSON serialization via `GraphSerializer`
- Separate from UI layer (clean architecture)

## Code Style

Follow the style guide in `.github/copilot-instructions.md`:

**Naming Conventions**:
- Variables & Functions: `snake_case` (e.g., `vertex_count`, `calculate_bounds()`)
- Types & Classes: `PascalCase` (e.g., `GeometryData`, `SOPNode`)
- Constants: `UPPER_SNAKE_CASE` (e.g., `DEFAULT_RADIUS`)
- Private Members: trailing underscore (e.g., `vertex_count_`, `last_error_`)

**Key Requirements**:
- Use C++20 features (concepts, ranges, std::optional)
- **Avoid C++23** - NO `std::expected`, use `std::optional<T>` instead
- Float literals with uppercase 'F': `2.0F` not `2.0f`
- Explicit null checks: `(ptr != nullptr)` not just `ptr`
- Prefer `constexpr` over `const` for compile-time constants
- Access static members through class: `ObjExporter::export_mesh()` not `instance.export_mesh()`
- Remove unused includes, use forward declarations when possible

## Testing

Tests are located in `tests/` directory using Google Test framework:

```bash
# Run all tests
./build/tests/nodeflux_tests

# Run specific test suite
./build/tests/nodeflux_tests --gtest_filter=MeshTest.*

# Run with verbose output
./build/tests/nodeflux_tests --gtest_verbose

# List all tests
./build/tests/nodeflux_tests --gtest_list_tests
```

**Test Coverage**:
- Core mesh operations (`test_mesh.cpp`)
- Boolean operations with CGAL (`test_boolean_ops.cpp`)
- BVH spatial acceleration (`test_bvh.cpp`)
- Mesh generation primitives (`test_mesh_generator.cpp`)
- Mesh validation and repair (`test_mesh_validator.cpp`, `test_mesh_repairer.cpp`)
- Node system (`test_nodes.cpp`)
- Math utilities (`test_math.cpp`)

## SOP Node Development

When creating new SOP nodes:

1. **Inherit from SOPNode**:
```cpp
class MyCustomSOP : public SOPNode {
public:
    MyCustomSOP() : SOPNode("my_custom", "CustomSOP") {
        // Add input ports if needed
        input_ports_.add_port("input", NodePort::Type::INPUT,
                             NodePort::DataType::GEOMETRY, this);
    }

protected:
    std::shared_ptr<GeometryData> execute() override {
        // Get input data
        auto input_data = get_input_data("input");
        if (!input_data || input_data->is_empty()) {
            set_error("No input geometry");
            return nullptr;
        }

        // Process geometry
        auto result = std::make_shared<GeometryData>();
        // ... your processing logic ...

        return result;
    }
};
```

2. **Parameter Management**: Use `set_parameter()` and `get_parameter()` with type safety
3. **Caching**: Automatic - node only re-executes when dirty
4. **Error Handling**: Use `set_error()` for failures, return nullptr on error
5. **Performance**: Prefer move semantics, reserve container capacity

## Qt Studio Application

The `nodeflux_studio` directory contains a Qt6-based visual editor:
- Located in: `nodeflux_studio/`
- Main files: `MainWindow.h/cpp`, `main.cpp`
- Built with Qt 6.7.3 (Widgets + OpenGL support)
- Provides visual node graph editing (planned: real-time 3D viewport)

## GPU Acceleration

The engine uses OpenGL compute shaders for real-time performance:
- Primitive generation achieves 10-100x speedups on large meshes
- BVH spatial acceleration provides 45x speedup for boolean operations
- Hardware utilization: Full RTX GPU support with 1024 work groups
- Context management handled by GLFW

## Current Development Focus

**Mentoring Mode**: The project is configured for learning. When helping:
- Guide through questions rather than providing complete implementations
- Explain design trade-offs and best practices
- Review code and suggest improvements with explanations
- Focus on understanding over speed

**Active Areas** (see `.github/copilot-instructions.md` TODO list):
- Attribute system for vertex/face/primitive data
- UV mapping and texture coordinate support
- Advanced deformation tools (bend, twist, taper)
- Real-time 3D viewport integration with the node editor
- Material system and rendering features

## File Structure

```
NodeFluxEngine/
├── CMakeLists.txt              - Main build configuration
├── conanfile.py                - Dependency management
├── include/nodeflux/           - Public headers
│   ├── core/                   - Core data structures
│   ├── geometry/               - Mesh generators and operations
│   ├── spatial/                - BVH and spatial structures
│   ├── sop/                    - SOP node system
│   ├── graph/                  - Node graph architecture
│   ├── nodes/                  - Primitive nodes
│   └── io/                     - Import/export
├── src/                        - Implementation files (mirrors include/)
├── tests/                      - Google Test unit tests
├── nodeflux_studio/            - Qt6 visual editor application
├── templates/                  - Example node graph JSON files
└── docs/                       - Architecture documentation
```

## Performance Guidelines

- Use Eigen for vectorized operations (avoid manual loops)
- Profile boolean operations - use BVH when working with large meshes
- Prefer move semantics for large mesh data
- Reserve container capacity when size is known: `vertices.reserve(count)`
- Consider memory layout for cache efficiency
- Use GPU compute shaders for operations on meshes > 10K triangles

## Important Notes

- **No C++23 features**: Project targets C++20 for broader compiler support
- **Thread safety**: Most operations are not thread-safe - avoid concurrent mesh modifications
- **Memory management**: Use smart pointers (`std::shared_ptr`, `std::unique_ptr`)
- **RAII**: Resources cleanup handled by destructors
- **Git workflow**: Main branch is `main`, use descriptive commit messages
