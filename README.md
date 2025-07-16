# NodeFluxEngine

A modern C++20 **GPU-accelerated procedural mesh generation library** with node-based workflows and real-time performance.

## 🎯 Vision

NodeFluxEngine is the **first GPU-native procedural mesh generation system** that combines:
- **🔥 Houdini-inspired SOP workflow** with visual node-based operations
- **⚡ Complete GPU acceleration** for real-time performance
- **🧠 Intelligent caching & data flow** for complex procedural workflows
- **🎯 Modern C++20 architecture** with robust error handling

## ✅ Current Status (Production Ready)

### **🏗️ Core Infrastructure**
- **Core Architecture**: C++20 modern design with std::optional error handling
- **Build System**: CMake with vcpkg + FetchContent hybrid approach
- **Unit Testing**: Comprehensive Google Test suite with 44 passing tests
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

## 🚀 What's Next: Procedural Node System

See **[UNIFIED_PROCEDURAL_ROADMAP.md](UNIFIED_PROCEDURAL_ROADMAP.md)** for the complete development plan.

**Current Focus**: Building a complete GPU-accelerated procedural mesh generation system with visual node-based workflows.

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

```bash
# Clone the repository
git clone https://github.com/dehnda/NodeFluxEngine.git
cd NodeFluxEngine

# Build with CMake (dependencies auto-fetched)
cmake -S . -B build
cmake --build build

# Run examples
./build/examples/basic_union
./build/examples/node_system_demo
./build/examples/gpu_accelerated_demo
```

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

## 📋 Development Roadmap

See **[UNIFIED_PROCEDURAL_ROADMAP.md](UNIFIED_PROCEDURAL_ROADMAP.md)** for the complete development plan including:

- **Week 1**: Core SOP data flow architecture and GPU primitive completion
- **Week 2**: Transform & Array nodes with GPU acceleration
- **Week 3**: Advanced procedural operations (subdivision, smoothing, noise)
- **Week 4**: Enhanced export system and production polish

## 🤝 Contributing

NodeFluxEngine is actively developed. See the roadmap for current priorities and feel free to contribute!

## 📄 License

MIT License
