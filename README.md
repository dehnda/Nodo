# Nodo

![Nodo Logo](nodo_studio/resources/logos/nodo_logo_horizontal.svg)

A modern C++20 **GPU-accelerated procedural mesh generation library** with node-based workflows and real-time performance.

## 🎯 Vision

Nodo is the **first GPU-native procedural mesh generation system** that combines:
- **🔥 Houdini-inspired SOP workflow** with visual node-based operations
- **⚡ Complete GPU acceleration** for real-time performance
- **🧠 Intelligent caching & data flow** for complex procedural workflows
- **🎯 Modern C++20 architecture** with robust error handling

## ✅ Current Status (Production Ready)

### **🏗️ Core Infrastructure**
- **Core Architecture**: C++20 modern design with std::optional error handling
- **Build System**: CMake with Conan 2.x unified dependency management
- **Unit Testing**: Comprehensive Google Test suite with 59 passing tests
- **GPU Framework**: Complete OpenGL compute shader system

### **🎨 Mesh Generation Engine**
- **All Primitives**: Box, Sphere (UV/Icosphere), Cylinder, Plane, Torus generators
- **Complete Node System**: Parameter-driven node architecture for all primitives
- **Mesh Validation**: Comprehensive validation and repair system
- **Export System**: Wavefront OBJ file format support

### **🚀 GPU Acceleration**
- **GPU Mesh Generation**: All primitives working with 10-100x speedups for large meshes
- **BVH Spatial Acceleration**: 45x speedup over brute-force boolean operations
- **Hardware Utilization**: Full RTX 5070 Ti support with 1024 work groups

### **🔧 Boolean & Spatial Operations**
- **Boolean Operations**: Union, intersection, difference with CGAL integration
- **Advanced Algorithms**: Ready for procedural workflows

## 🚀 Development Status & Roadmap

See **[ROADMAP.md](ROADMAP.md)** for the complete development plan including:
- Strategic goals and timeline (Q4 2025 - 2027+)
- Current phase: Property panel system implementation (Phase 1)
- Future engine integration plans (Godot/Unity/Unreal)
- Decision points and success metrics

See **[CONTRIBUTING.md](CONTRIBUTING.md)** for technical documentation:
- Code style guide and formatting
- How to add new SOP nodes
- Build instructions and testing

**Current Focus (October 2025)**: Backend parameter system + property panel UI for all 44 nodes.

## Quick Start

```cpp
#include "nodeflux/nodes/sphere_node.hpp"
#include "nodeflux/nodes/box_node.hpp"
#include "nodeflux/geometry/boolean_ops.hpp"
#include "nodeflux/io/obj_exporter.hpp"

using namespace nodeflux;

int main() {
    // Create procedural nodes
    auto sphere = std::make_unique<nodes::SphereNode>(1.0, 32, 16);
    auto box = std::make_unique<nodes::BoxNode>(2.0, 2.0, 2.0);

    // Generate meshes
    auto sphere_mesh = sphere->generate();
    auto box_mesh = box->generate();

    // Boolean operation with BVH acceleration
    if (sphere_mesh && box_mesh) {
        auto result = geometry::BooleanOperations::union_meshes(*sphere_mesh, *box_mesh);
        if (result) {
            io::ObjExporter::export_mesh(*result, "union_result.obj");
        }
    }

    return 0;
}
```

## 🛠️ Build Instructions

### Prerequisites
- **CMake 3.20+**
- **C++20 compatible compiler** (GCC 11+, Clang 12+, MSVC 2022+)
- **Conan 2.x** for dependency management
- **Python 3.8+** (for Conan)

### Setup & Build

```bash
# 1. Install Conan (if not already installed)
pip install "conan>=2.0"

# 2. Clone the repository
git clone https://github.com/dehnda/nodo.git
cd nodo

# 3. Install dependencies with Conan
conan install . --output-folder=build --build=missing

# 4. Configure and build with CMake
cmake --preset conan-debug
cmake --build build --parallel

# 5. Run tests to verify installation
./build/tests/nodeflux_tests

# 6. Run examples
./build/examples/basic_union
./build/examples/complete_node_system
./build/examples/gpu_accelerated_demo
```

### Dependencies (Managed by Conan)
- **Eigen3 3.4.0** - Linear algebra operations
- **CGAL 5.6.1** - Computational geometry algorithms
- **GLFW 3.4** - OpenGL window management
- **GLEW 2.2.0** - OpenGL extension loading
- **Google Test 1.14.0** - Testing framework
- **fmt 10.2.1** - String formatting
- **GMP/MPFR** - Multiple precision arithmetic

All dependencies are automatically resolved and built by Conan.

### Troubleshooting

**CMakePresets.json not found:**
```bash
# Re-run Conan install to regenerate presets
conan install . --output-folder=build --build=missing
```

**Dependency conflicts:**
```bash
# Clean and rebuild dependencies
rm -rf build
conan install . --output-folder=build --build=missing
```

**GPU examples require OpenGL:**
- Ensure graphics drivers are installed
- For headless systems, use virtual displays or Mesa software rendering

### VS Code Integration

For VS Code users, the project includes pre-configured tasks:

- **Ctrl+Shift+P → "Tasks: Run Task"** to access all build tasks
- **"NodeFlux Build (Debug)"** - Default build task (Ctrl+Shift+B)
- **"Run Tests"** - Execute the full test suite
- **"Conan Install Dependencies"** - Refresh dependencies
- **"Full Rebuild"** - Clean and rebuild everything
- **"Run GPU Accelerated Demo"** - Launch GPU examples

The tasks handle the complete Conan workflow automatically.

## 📁 Architecture

```
include/nodeflux/
├── core/           # Core data structures (Mesh, Point, Vector)
├── geometry/       # Mesh generation and boolean operations
├── gpu/            # GPU acceleration (OpenGL compute shaders)
├── nodes/          # Node-based procedural system
└── io/             # Import/export functionality (OBJ, future: STL, glTF)

src/
├── core/           # Core implementations
├── geometry/       # Geometry processing implementations
├── gpu/            # GPU compute implementations
├── nodes/          # Node system implementations
└── io/             # I/O implementations

examples/           # Usage examples and demos
tests/              # Unit tests (44 passing tests)
```

## 📋 Project Documentation

Comprehensive documentation is available:

- **[CONTRIBUTING.md](CONTRIBUTING.md)** - Code style, architecture, and how to contribute
- **[ROADMAP.md](ROADMAP.md)** - Development roadmap and future plans
- **[docs/](docs/)** - Technical documentation and guides

## 🤝 Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for:
- Code style guidelines
- Development workflow
- How to submit pull requests
- Testing requirements

For bug reports and feature requests, please use [GitHub Issues](https://github.com/dehnda/Nodo/issues).

## 📄 License

MIT License - Copyright (c) 2025 Daniel Dehne

This project is open source and free to use. See [LICENSE](LICENSE) for full details.
Third-party components are used under their respective licenses (see [THIRD_PARTY_LICENSES.txt](THIRD_PARTY_LICENSES.txt)).
