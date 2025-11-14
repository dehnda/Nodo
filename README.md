# Nodo

![Nodo Logo](nodo_studio/resources/logos/nodo_logo_horizontal.svg)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![CI](https://github.com/dehnda/Nodo/workflows/CI/badge.svg)](https://github.com/dehnda/Nodo/actions)
[![codecov](https://codecov.io/gh/dehnda/Nodo/branch/main/graph/badge.svg)](https://codecov.io/gh/dehnda/Nodo)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey)](https://github.com/dehnda/Nodo)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Qt](https://img.shields.io/badge/Qt-6.7-green.svg)](https://www.qt.io/)
[![CMake](https://img.shields.io/badge/CMake-3.20%2B-064F8C.svg)](https://cmake.org/)
[![Conan](https://img.shields.io/badge/Conan-2.x-blue.svg)](https://conan.io/)

A professional **node-based procedural 3D modeling tool** with real-time workflows and parametric control.

> **⚠️ Early Development**: Nodo is actively being developed. Features and APIs may change. Use for experimentation and provide feedback!

![Nodo Studio Interface](website/Nodo_Studio_Screenshot.png)

## 🎯 Vision

Nodo brings **Houdini-inspired procedural modeling** to everyone:
- **🔥 Visual node-based workflow** - Connect nodes to build complex geometry
- **⚡ Real-time updates** - See changes instantly as you work
- **🧠 Expression system** - Parameter-driven workflows with math expressions
- **🎯 Production-ready** - 44+ nodes for complete modeling workflows

## ✅ What's Included

### **🎨 44+ Production Nodes**
- **Generators**: Box, Sphere, Cylinder, Torus, Grid, Line
- **Modifiers**: Extrude, Subdivide, Smooth, Noise, Bevel, Mirror, Bend, Twist
- **Transform**: Transform, Array, Copy to Points, Scatter, Align
- **Boolean**: Union, Intersection, Difference, Merge, Split
- **Attributes**: Wrangle (expressions), Color, Normal, UV Unwrap
- **Groups**: Create, Delete, Combine, Promote, Expand, Transfer
- **Advanced**: Geodesic, Remesh, Parameterize, Curvature, Repair
- **Utility**: Switch, Null, Output, File I/O

### **⚡ Expression System**
- **Parameter expressions**: `$F * 0.1`, `ch("../box/size")`
- **Math functions**: `sin()`, `cos()`, `rand()`, `noise()`
- **Channel references**: Link parameters across nodes
- **Graph parameters**: Expose controls for entire graphs

### **🔧 Professional Features**
- **Real-time preview**: See changes as you modify parameters
- **Undo/Redo**: Full history with Ctrl+Z/Ctrl+Y
- **Keyboard shortcuts**: Professional workflow navigation
- **File format**: OBJ import/export with full attribute support
- **CLI tool**: Headless batch processing (nodo_cli)

## 🚀 Development Status

Nodo is in **active development** with a stable core feature set.

See **[ROADMAP.md](ROADMAP.md)** for upcoming features and long-term vision.

## Quick Start

```cpp
#include "nodo/nodes/sphere_node.hpp"
#include "nodo/nodes/box_node.hpp"
#include "nodo/geometry/boolean_ops.hpp"
#include "nodo/io/obj_exporter.hpp"

using namespace nodo;

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

### Quick Start

```bash
# 1. Install Conan
pip install "conan>=2.0"

# 2. Clone and build
git clone https://github.com/dehnda/nodo.git
cd nodo
conan install . --build=missing
cmake --preset conan-debug
cmake --build --preset conan-debug --parallel

# 3. Run the application
./build/Debug/nodo_studio/nodo_studio
```

### Full Build Documentation

See **[BUILD.md](BUILD.md)** for complete instructions including:
- Platform-specific setup (Linux, macOS, Windows)
- Debug vs Release configurations
- Enabling tests
- Troubleshooting common issues
- VS Code integration
- Windows release packaging

### VS Code Integration

For VS Code users, the project includes pre-configured tasks:

- **Ctrl+Shift+P → "Tasks: Run Task"** to access all build tasks
- **"Nodo Build (Debug)"** - Default build task (Ctrl+Shift+B)
- **"Run Tests"** - Execute the full test suite
- **"Conan Install Dependencies"** - Refresh dependencies
- **"Full Rebuild"** - Clean and rebuild everything
- **"Run GPU Accelerated Demo"** - Launch GPU examples

The tasks handle the complete Conan workflow automatically.

## 📁 Architecture

```
include/nodo/
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

- **[📖 Online Documentation](https://dehnda.github.io/Nodo/)** - Complete user guide and API reference
- **[ROADMAP.md](ROADMAP.md)** - Development roadmap and future plans
- **[BUILD.md](BUILD.md)** - Build instructions for all platforms

## 📄 License

MIT License - Copyright (c) 2025 Daniel Dehne

This project is open source and free to use. See [LICENSE](LICENSE) for full details.
Third-party components are used under their respective licenses (see [THIRD_PARTY_LICENSES.txt](THIRD_PARTY_LICENSES.txt)).
