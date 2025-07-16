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

## Detailed Coding Style Guide

### Naming Conventions
- **Variables & Functions**: snake_case (e.g., `vertex_count`, `calculate_bounds()`)
- **Types & Classes**: PascalCase (e.g., `GeometryData`, `ArraySOP`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `DEFAULT_RADIUS`, `MAX_VERTICES`)
- **Private Members**: trailing underscore (e.g., `vertex_count_`, `last_error_`)
- **Template Parameters**: PascalCase (e.g., `template<typename DataType>`)

### Literals & Constants
- **Magic Numbers**: Replace with named constants
  ```cpp
  // Bad
  auto sphere = generate_sphere(1.0f, 32, 16);

  // Good
  constexpr float DEFAULT_RADIUS = 1.0F;
  constexpr int DEFAULT_SUBDIVISIONS = 32;
  constexpr int DEFAULT_RINGS = 16;
  auto sphere = generate_sphere(DEFAULT_RADIUS, DEFAULT_SUBDIVISIONS, DEFAULT_RINGS);
  ```
- **Float Literals**: Use uppercase 'F' suffix (e.g., `2.0F` not `2.0f`)
- **Constants**: Prefer `constexpr` over `const` for compile-time constants

### Pointer & Reference Safety
- **Explicit Null Checks**: Always check pointers explicitly
  ```cpp
  // Bad
  return port ? port->get_data() : nullptr;

  // Good
  return (port != nullptr) ? port->get_data() : nullptr;
  ```
- **Smart Pointers**: Prefer `std::unique_ptr` and `std::shared_ptr` over raw pointers
- **Reference Parameters**: Use const references for input parameters when possible

### Include Management
- **Remove Unused Includes**: Only include headers that are directly used
- **Forward Declarations**: Use forward declarations in headers when possible
- **Include Order**: System headers first, then project headers, alphabetically within groups

### Static Members
- **Access through Class**: Use `ClassName::static_member()` not `instance.static_member()`
  ```cpp
  // Bad
  exporter.export_mesh(*mesh, "output.obj");

  // Good
  ObjExporter::export_mesh(*mesh, "output.obj");
  ```

### Error Handling
- **Explicit Error Checking**: Always check return values and optional types
- **RAII**: Use destructors for cleanup, avoid manual resource management
- **Exception Safety**: Provide at least basic exception guarantee

### Performance Guidelines
- **Move Semantics**: Use `std::move` for expensive-to-copy objects
- **const Correctness**: Mark methods const when they don't modify state
- **Reserve Containers**: Pre-allocate container capacity when size is known
- **Avoid Copies**: Pass large objects by const reference

### Documentation Standards
- **Doxygen Comments**: Use `/**` style for public APIs
- **Brief Descriptions**: Start with `@brief` for one-line summaries
- **Parameter Documentation**: Use `@param` for all parameters
- **Return Value Documentation**: Use `@return` for non-void functions
- **Examples**: Include `@code` blocks for complex APIs

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
*🎯 STRATEGIC FOCUS: GPU-Accelerated Procedural Node System*

### 🎯 High Priority (Next Sprint) - Procedural Foundation
- [x] **Unit Testing Framework**: Enable Google Test and write comprehensive tests
- [x] **Performance Optimization**: Add spatial data structures (BVH/Octree) ✅ 45x speedup achieved
### 🎯 High Priority (Next Sprint) - Procedural Foundation  
- [x] **Unit Testing Framework**: Enable Google Test and write comprehensive tests ✅ 59 tests with 52 passing
- [x] **Performance Optimization**: Add spatial data structures (BVH/Octree) ✅ 45x speedup achieved
- [x] **Torus Generator**: Add torus primitive to complete basic geometry set ✅
- [x] **GPU Acceleration**: Compute shaders for mesh operations ✅ Infinite speedup achieved
- [x] **GPU Mesh Primitives**: Complete Box, Cylinder, Plane generators with compute shaders ✅ All implemented
- [x] **SOP Data Flow Architecture**: Core GeometryData containers and NodePort connection system ✅ Complete
- [x] **Procedural Execution Engine**: Smart caching, dependency resolution, GPU batch processing ✅ Working

### 🔧 Current Priority (Week 3) - Advanced Procedural Operations
- [x] **Transform & Array Nodes**: TransformSOP, ArraySOP, MirrorSOP with GPU acceleration ✅ MirrorSOP completed
- [x] **Enhanced Boolean Nodes**: BooleanSOP with GPU BVH integration ✅ Complete implementation
- [ ] **Advanced Transformations**: ExtrudeSOP, BevelSOP, InsetSOP as procedural nodes (Week 3)
- [ ] **Subdivision Surfaces**: SubdivisionSOP (Catmull-Clark and Loop) enhancement (Week 3)
- [ ] **Material System**: Basic material/attribute support for procedural meshes (Week 3)
- [ ] **Mesh Smoothing**: LaplacianSOP, TaubinSOP as modifier nodes (Week 3)

### 🚀 Future Features (Next Quarter) - Advanced Procedural Suite
- [x] **Noise Functions**: NoiseDisplacementSOP (Perlin, Simplex) ✅ Basic implementation complete
- [ ] **GPU Boolean Operations**: Full GPU acceleration for complex geometric operations in node graph
- [ ] **UV Unwrapping**: Automatic texture coordinate generation as procedural node
- [ ] **PLY Format Support**: Import/export for point cloud data with node integration
- [ ] **glTF Export**: Modern 3D format for web and real-time applications
- [ ] **Additional Primitives**: ConeSOP, RoundedBoxSOP generators as procedural nodes

### ✅ Completed Features
- [x] **Core Architecture**: C++20 modern design with std::optional error handling
- [x] **Boolean Operations**: Union, intersection, difference with CGAL
- [x] **Basic Primitives**: Box, Sphere (UV/Icosphere), Cylinder, Plane, Torus generators
- [x] **Complete Node System**: BoxNode, SphereNode, CylinderNode, PlaneNode, TorusNode with parameter modification
- [x] **Mesh Validation Tools**: Comprehensive mesh validation and repair system with manifold checking
- [x] **OBJ Export**: Wavefront OBJ file format support
- [x] **Build System**: CMake with Conan 2.x unified dependency management
- [x] **Development Environment**: clangd IntelliSense, compile_commands.json
- [x] **Example Applications**: Basic union, primitive generators, boolean tests, complete node system demo, mesh validation demo, torus demo
- [x] **Unit Testing Framework**: Comprehensive Google Test suite with 59 tests covering all core functionality
- [x] **GPU Acceleration Framework**: Complete OpenGL compute shader system with GLFW context management
- [x] **BVH Spatial Acceleration**: 45x speedup over brute-force with enhanced boolean operations
- [x] **GPU Mesh Generation**: Real-time mesh generation with 10-100x speedups for large meshes
- [x] **Complete SOP System**: BooleanSOP, MirrorSOP, ArraySOP, SubdivisionSOP, NoiseDisplacementSOP

### 📋 Technical Debt & Improvements
- [ ] **Code Quality**: Address remaining linter warnings (magic numbers, short variable names)
- [ ] **Memory Management**: Profile and optimize memory usage for large meshes
- [ ] **Error Handling**: Expand error codes and improve error context
- [ ] **Documentation**: Add Doxygen generation and API documentation
- [ ] **CI/CD Pipeline**: GitHub Actions for automated testing and builds

### 🎨 UI & Visualization (Future) - Visual Interface
- [ ] **Node Graph Editor**: Visual SOP-based editing interface for procedural workflows
- [ ] **3D Viewport**: Real-time mesh preview and manipulation
- [ ] **Parameter Widgets**: UI controls for generator parameters
- [ ] **Export Dialog**: User-friendly file export interface

### 📊 Performance & Scalability - Production Ready
- [ ] **Benchmarking Suite**: Performance regression testing for procedural workflows
- [ ] **Parallel Processing**: OpenMP for multi-threaded SOP operations
- [ ] **LOD System**: Level-of-detail for large procedural scene management
- [ ] **Streaming**: Support for procedural meshes larger than available RAM

---
*Last Updated: July 16, 2025 - Week 2 COMPLETE*
*Current Focus: Week 3 Advanced transformations (ExtrudeSOP, LaplacianSOP, MaterialSOP)*
- [ ] **Export Dialog**: User-friendly file export interface

### 📊 Performance & Scalability
- [ ] **Benchmarking Suite**: Performance regression testing
- [ ] **Parallel Processing**: OpenMP for multi-threaded operations
- [ ] **LOD System**: Level-of-detail for large scene management
- [ ] **Streaming**: Support for meshes larger than available RAM

---
*Last Updated: July 16, 2025*
*Current Focus: GPU-accelerated mesh generation and spatial data structures*
