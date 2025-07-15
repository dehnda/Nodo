# NodeFluxEngine

A modern C++20 procedural mesh generation library with clean architecture and direct CGAL integration.

## Features

- 🔧 **Modern C++20**: Uses concepts, ranges, and modern error handling
- 🎯 **Direct CGAL Integration**: Proven boolean operations without compatibility issues
- 📦 **vcpkg Dependencies**: Clean, reproducible dependency management
- 🏗️ **Modular Architecture**: Separated concerns for maintainability
- ⚡ **High Performance**: Efficient mesh operations and memory management
- 🧪 **Well Tested**: Comprehensive unit tests for all components

## Quick Start

```cpp
#include "nodeflux/geometry/boolean_ops.hpp"
#include "nodeflux/geometry/mesh_generator.hpp"
#include "nodeflux/io/mesh_io.hpp"

using namespace nodeflux;

int main() {
    // Generate two overlapping boxes
    auto box1 = geometry::MeshGenerator::box({0, 0, 0}, {2, 2, 2});
    auto box2 = geometry::MeshGenerator::box({1, 1, 1}, {3, 3, 3});
    
    // Perform clean union operation
    auto result = geometry::BooleanOps::union_meshes(box1, box2);
    
    if (result) {
        // Export the result
        io::MeshIO::export_obj("union_result.obj", *result);
        std::cout << "Success: " << result->vertex_count() << " vertices\\n";
    } else {
        std::cerr << "Error: " << geometry::BooleanOps::error_message(result.error()) << "\\n";
    }
    
    return 0;
}
```

## Build Requirements

- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.20+
- vcpkg package manager

## Setup

```bash
# Clone the project
git clone <your-repo-url>
cd NodeFluxEngine

# Bootstrap vcpkg (already included in project)
cd vcpkg && ./bootstrap-vcpkg.sh && cd ..

# Install dependencies
./vcpkg/vcpkg install

# Configure project with vcpkg
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build

# Run examples
./build/examples/basic_union
```

## Architecture

```
include/nodeflux/
├── core/           # Core data structures and interfaces
├── geometry/       # Mesh generation and boolean operations  
├── io/             # Import/export functionality
└── nodes/          # Node-based procedural system

src/
├── core/           # Core implementations
├── geometry/       # Geometry processing implementations
├── io/             # I/O implementations
└── nodes/          # Node system implementations

examples/           # Usage examples
tests/              # Unit tests
```

## License

MIT License
