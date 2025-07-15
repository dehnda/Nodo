<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->

# NodeFluxEngine Copilot Instructions

This is a modern C++20 procedural mesh generation library project. Please follow these guidelines:

## Code Style & Standards
- Use C++20 features: concepts, ranges, std::optional, modules where appropriate
- **Avoid C++23 features**: Do not use std::expected - use std::optional<T> instead
- Follow RAII principles for resource management
- Use strong typing and avoid raw pointers
- Prefer value semantics over reference semantics where possible
- Use snake_case for variables and functions, PascalCase for types

## Architecture Guidelines
- **Core module**: Fundamental data structures (Mesh, Point, Vector)
- **Geometry module**: Mesh generation, transformations, boolean operations
- **IO module**: Import/export functionality (OBJ, STL, etc.)
- **Nodes module**: Node-based procedural system

## Error Handling
- Use std::optional<T> for operations that can fail (C++20 compatible)
- Use thread_local error storage for detailed error information
- Define specific error types for each module
- Provide clear error messages for debugging

## Dependencies
- Primary: Eigen3 (linear algebra), CGAL (computational geometry)
- Build: CMake with vcpkg for dependency management
- Testing: Use Google Test for unit tests

## Performance Considerations
- Use Eigen for vectorized operations
- Prefer move semantics for large mesh data
- Consider memory layout for cache efficiency
- Profile boolean operations for optimization opportunities

## Testing
- Write unit tests for all public APIs
- Include edge cases and error conditions
- Test with various mesh sizes and complexities
- Benchmark critical path operations

## Documentation
- Use Doxygen-style comments for public APIs
- Include usage examples in comments
- Document preconditions and postconditions
- Explain algorithm choices for complex operations

## TODO List - NodeFluxEngine Development Roadmap

### 🎯 High Priority (Next Sprint)
- [ ] **Mesh Validation Tools**: Implement manifold checking and mesh repair
- [ ] **Unit Testing Framework**: Enable Google Test and write comprehensive tests
- [ ] **STL File Format**: Add STL import/export support (binary and ASCII)
- [ ] **Performance Optimization**: Add spatial data structures (BVH/Octree)
- [ ] **Torus Generator**: Add torus primitive to complete basic geometry set

### 🔧 Medium Priority (Current Quarter)
- [ ] **Additional Primitives**: Torus, Cone, Rounded Box generators
- [ ] **Subdivision Surfaces**: Catmull-Clark and Loop subdivision algorithms
- [ ] **Advanced Transformations**: Extrude, Bevel, Inset operations
- [ ] **Material System**: Basic material/attribute support for meshes
- [ ] **Mesh Smoothing**: Laplacian and Taubin smoothing algorithms

### 🚀 Future Features (Next Quarter)
- [ ] **Array/Pattern Nodes**: Linear, radial, and grid array modifiers
- [ ] **PLY Format Support**: Import/export for point cloud data
- [ ] **glTF Export**: Modern 3D format for web and real-time applications
- [ ] **GPU Acceleration**: Compute shaders for mesh operations
- [ ] **Noise Functions**: Perlin, Simplex noise for procedural texturing
- [ ] **UV Unwrapping**: Automatic texture coordinate generation

### ✅ Completed Features
- [x] **Core Architecture**: C++20 modern design with std::optional error handling
- [x] **Boolean Operations**: Union, intersection, difference with CGAL
- [x] **Basic Primitives**: Box, Sphere (UV/Icosphere), Cylinder, Plane generators
- [x] **Complete Node System**: BoxNode, SphereNode, CylinderNode, PlaneNode with parameter modification
- [x] **OBJ Export**: Wavefront OBJ file format support
- [x] **Build System**: CMake with vcpkg + FetchContent hybrid approach
- [x] **Development Environment**: clangd IntelliSense, compile_commands.json
- [x] **Example Applications**: Basic union, primitive generators, boolean tests, complete node system demo

### 📋 Technical Debt & Improvements
- [ ] **Code Quality**: Address remaining linter warnings (magic numbers, short variable names)
- [ ] **Memory Management**: Profile and optimize memory usage for large meshes
- [ ] **Error Handling**: Expand error codes and improve error context
- [ ] **Documentation**: Add Doxygen generation and API documentation
- [ ] **CI/CD Pipeline**: GitHub Actions for automated testing and builds

### 🎨 UI & Visualization (Future)
- [ ] **Node Graph Editor**: Visual node-based editing interface
- [ ] **3D Viewport**: Real-time mesh preview and manipulation
- [ ] **Parameter Widgets**: UI controls for generator parameters
- [ ] **Export Dialog**: User-friendly file export interface

### 📊 Performance & Scalability
- [ ] **Benchmarking Suite**: Performance regression testing
- [ ] **Parallel Processing**: OpenMP for multi-threaded operations
- [ ] **LOD System**: Level-of-detail for large scene management
- [ ] **Streaming**: Support for meshes larger than available RAM

---
*Last Updated: July 16, 2025*
*Current Focus: Mesh validation tools and unit testing framework*
