<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->

# NodeFluxEngine Copilot Instructions

This is a modern C++20 procedural mesh generation library project. Please follow these guidelines:

## 🎯 **MENTORING MODE - PRIMARY DIRECTIVE**
**Act as a coding mentor and guide, NOT an implementer. Focus on:**
- **Teaching through questions**: Ask guiding questions to help me discover solutions
- **Code review & feedback**: Analyze my code and suggest improvements with explanations
- **Architecture guidance**: Help me think through design decisions and trade-offs
- **Best practices coaching**: Explain WHY certain approaches are better
- **Problem-solving methodology**: Guide me through debugging and optimization strategies
- **Learning resources**: Suggest documentation, patterns, and techniques to study

**DO NOT:**
- Write complete implementations for me
- Directly solve problems without letting me work through them
- Make file changes unless specifically requested for setup/configuration
- Rush to solutions - prioritize understanding over speed

**INSTEAD:**
- Point me toward the right direction with hints and questions
- Help me break down complex problems into manageable steps  
- Review my code and explain improvements
- Suggest what to research or study next
- Help me understand error messages and debugging approaches

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
*🎯 STRATEGIC FOCUS: Complete Procedural Modeling System with Real-Time Visualization*

### 🔥 **PHASE 1: Core Procedural Data Architecture (Week 4)**
*Foundation for Professional Procedural Modeling*

#### **🎯 1.1 Attribute System - Most Critical**
- [ ] **GeometryAttributes Class**: Core attribute storage and management
  - [ ] **Per-vertex attributes**: Position, normal, color, UV, custom data
  - [ ] **Per-face attributes**: Material ID, group ID, custom face data
  - [ ] **Per-primitive attributes**: Object-level metadata
  - [ ] **Attribute promotion/demotion**: Convert between vertex/face/primitive levels
  - [ ] **Attribute transfer**: Copy attributes between geometries
  - [ ] **Attribute interpolation**: Smooth attribute transitions during operations

#### **🎯 1.2 Enhanced Data Types**
- [ ] **Extend NodePin::DataType**: Add UV, Color, Vector, Float, Integer types
- [ ] **Multi-output nodes**: Support nodes with multiple data streams
- [ ] **Data type validation**: Ensure compatible connections
- [ ] **Automatic type conversion**: Smart casting between compatible types

#### **🎯 1.3 Selection & Grouping System**
- [ ] **Selection API**: Point, edge, face, primitive selection
- [ ] **Group management**: Named groups with add/remove operations
- [ ] **Pattern-based selection**: Select by attribute values, patterns
- [ ] **Selection visualization**: Highlight selected components in viewport

### 🎨 **PHASE 2: Essential Procedural Operations (Week 5)**
*Core Modeling Tools for Professional Workflows*

#### **🎨 2.1 UV Mapping & Texture Coordinates**
- [ ] **UV Generator Nodes**: Planar, cylindrical, spherical projection
- [ ] **UV Layout Tools**: Automatic unwrapping and seam detection
- [ ] **UV Attribute Integration**: Store UV coordinates in attribute system
- [ ] **Multi-layer UV support**: Multiple UV sets per geometry

#### **🎨 2.2 Advanced Deformation Tools**
- [ ] **Bend SOP**: Bend geometry along axis with falloff
- [ ] **Twist SOP**: Twist geometry around axis
- [ ] **Taper SOP**: Scale geometry along axis
- [ ] **Lattice SOP**: Lattice-based deformation
- [ ] **Path Deform SOP**: Deform along curve path

#### **🎨 2.3 Curve & Surface Foundation**
- [ ] **Curve primitive**: NURBS/Bezier curve support
- [ ] **Curve generators**: Line, circle, arc, spiral
- [ ] **Sweep SOP**: Sweep profile along curve
- [ ] **Extrude SOP**: Extrude face along normal/direction
- [ ] **Revolve SOP**: Revolve profile around axis

### 🔧 **PHASE 3: Advanced Mesh Operations (Week 6)**
*Professional Topology & Mesh Editing*

#### **🔧 3.1 Topology Operations**
- [ ] **Edge operations**: Split, collapse, flip edges
- [ ] **Face operations**: Inset, outset, bridge faces
- [ ] **Vertex operations**: Merge, split, slide vertices
- [ ] **Topology validation**: Manifold checking and repair

#### **🔧 3.2 Measurement & Analysis**
- [ ] **Measure SOP**: Distance, area, volume calculations
- [ ] **Connectivity analysis**: Connected components, boundaries
- [ ] **Geometric analysis**: Curvature, normal computation
- [ ] **Quality metrics**: Aspect ratio, skewness analysis

#### **🔧 3.3 Instancing & Copying**
- [ ] **Copy to Points SOP**: Instance geometry at point locations
- [ ] **Scatter SOP**: Randomly distribute points on surfaces
- [ ] **Instance management**: Efficient instancing system
- [ ] **Packed geometry**: Memory-efficient instance storage

### 🚀 **PHASE 4: Production Features (Month 2)**
*Complete Professional Toolset*

#### **🚀 4.1 Material & Rendering**
- [ ] **Material attributes**: Diffuse, specular, roughness, metallic
- [ ] **Shader assignment**: Per-face material assignment
- [ ] **Texture mapping**: Image texture support with UV coordinates
- [ ] **Material editor**: Node-based material creation

#### **🚀 4.2 Import/Export Pipeline**
- [ ] **Enhanced OBJ**: Material (MTL) support
- [ ] **glTF export**: PBR material support
- [ ] **STL export**: 3D printing format
- [ ] **PLY format**: Point cloud and mesh data
- [ ] **Alembic support**: Animation cache format

#### **🚀 4.3 Advanced Procedural Features**
- [ ] **Procedural animation**: Time-based parameter animation
- [ ] **Scripting interface**: Python/VEX-like expression system
- [ ] **Volume operations**: Voxel-based modeling
- [ ] **Particle systems**: Point cloud operations
- [ ] **Simulation integration**: Physics-based modeling

### 🏗️ **ARCHITECTURE DESIGN PRINCIPLES**

#### **🎯 Modular Node System**
```cpp
// Example extensible node architecture
class ProceduralNode {
    virtual std::vector<NodePin> get_input_pins() const = 0;
    virtual std::vector<NodePin> get_output_pins() const = 0;
    virtual void execute(const NodeContext& context) = 0;
    virtual NodeMetadata get_metadata() const = 0;
};

// Specialized node types
class AttributeNode : public ProceduralNode { /* ... */ };
class DeformationNode : public ProceduralNode { /* ... */ };
class GeneratorNode : public ProceduralNode { /* ... */ };
```

#### **🎯 Extensible Data System**
```cpp
// Flexible attribute system
class GeometryAttributes {
    template<typename T>
    void add_attribute(const std::string& name, AttributeClass class_type);
    
    template<typename T>
    std::optional<T> get_attribute(const std::string& name, int index) const;
    
    void transfer_attributes(const GeometryAttributes& source, const std::vector<int>& mapping);
};
```

#### **🎯 Plugin Architecture**
```cpp
// Extensible node registration
class NodeRegistry {
    static void register_node<T>(const std::string& category, const std::string& name);
    static std::unique_ptr<ProceduralNode> create_node(const std::string& type);
    static std::vector<NodeMetadata> get_available_nodes();
};
```

### 🎯 High Priority (Week 3) - Modern Architecture Foundation
- [ ] **Clean Architecture Implementation**: Implement new data model separation
  - [ ] **NodeGraph Core**: Pure data model with serialization support (nodeflux::graph::NodeGraph)
  - [ ] **ExecutionEngine**: Separate graph execution logic with dependency resolution
  - [ ] **GraphSerializer**: JSON-based save/load with version compatibility
  - [ ] **ModernNodeGraphEditor**: UI layer cleanly separated from data model
  - [ ] **Migration Path**: Migrate existing NodeGraphEditor to new architecture

### 🎨 Week 4 - Real-Time Visualization System
- [ ] **3D Renderer Architecture**: Real-time mesh visualization and manipulation
  - [ ] **ViewportRenderer**: OpenGL-based 3D viewport with camera controls
  - [ ] **MeshRenderCache**: GPU buffer management for dynamic mesh updates
  - [ ] **Scene Management**: Multi-mesh scene with selection and highlighting
  - [ ] **Camera System**: Orbit/pan/zoom controls with smooth transitions
  - [ ] **Viewport Integration**: Embed 3D viewport in node editor layout
- [ ] **Real-Time Updates**: Live mesh preview as node graph changes
  - [ ] **Change Notifications**: Automatic viewport updates on graph execution
  - [ ] **Selective Updates**: Only re-render changed nodes for performance
  - [ ] **Preview Modes**: Wireframe, solid, normal visualization
  - [ ] **Performance Optimization**: LOD and frustum culling for large scenes

### 🔧 Current Priority (Week 3) - Advanced Procedural Operations
- [x] **Transform & Array Nodes**: TransformSOP, ArraySOP, MirrorSOP with GPU acceleration ✅ MirrorSOP completed
- [x] **Enhanced Boolean Nodes**: BooleanSOP with GPU BVH integration ✅ Complete implementation
- [ ] **Advanced Transformations**: ExtrudeSOP, BevelSOP, InsetSOP as procedural nodes
- [ ] **Subdivision Surfaces**: SubdivisionSOP (Catmull-Clark and Loop) enhancement
- [ ] **Material System**: Basic material/attribute support for procedural meshes
- [ ] **Mesh Smoothing**: LaplacianSOP, TaubinSOP as modifier nodes

### 🚀 Future Features (Month 2) - Production-Ready System
- [x] **Noise Functions**: NoiseDisplacementSOP (Perlin, Simplex) ✅ Basic implementation complete
- [ ] **Advanced Rendering Features**:
  - [ ] **Material Editor**: Node-based material/shader editor
  - [ ] **Texture Support**: UV mapping and texture coordinate generation
  - [ ] **Lighting System**: Real-time lighting with shadows
  - [ ] **Post-Processing**: Ambient occlusion, anti-aliasing, tone mapping
- [ ] **Production Pipeline**:
  - [ ] **Asset Browser**: File management and asset preview
  - [ ] **Export Pipeline**: Multi-format export (OBJ, glTF, STL, PLY)
  - [ ] **Batch Processing**: Command-line tools for automation
  - [ ] **Plugin Architecture**: Extensible node system

### 🎯 High Priority (Completed) - Procedural Foundation  
- [x] **Unit Testing Framework**: Enable Google Test and write comprehensive tests ✅ 59 tests with 52 passing
- [x] **Performance Optimization**: Add spatial data structures (BVH/Octree) ✅ 45x speedup achieved
- [x] **Torus Generator**: Add torus primitive to complete basic geometry set ✅
- [x] **GPU Acceleration**: Compute shaders for mesh operations ✅ Infinite speedup achieved
- [x] **GPU Mesh Primitives**: Complete Box, Cylinder, Plane generators with compute shaders ✅ All implemented
- [x] **SOP Data Flow Architecture**: Core GeometryData containers and NodePort connection system ✅ Complete
- [x] **Procedural Execution Engine**: Smart caching, dependency resolution, GPU batch processing ✅ Working

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
- [x] **Visual Node Editor**: Working ImNodes-based editor with fixed initialization ✅

### 📋 Technical Debt & Improvements
- [ ] **Code Quality**: Address remaining linter warnings (magic numbers, short variable names)
- [ ] **Memory Management**: Profile and optimize memory usage for large meshes
- [ ] **Error Handling**: Expand error codes and improve error context
- [ ] **Documentation**: Add Doxygen generation and API documentation
- [ ] **CI/CD Pipeline**: GitHub Actions for automated testing and builds

### 🎨 UI & Visualization Roadmap - Complete Visual Pipeline
- [x] **Node Graph Editor**: Visual SOP-based editing interface for procedural workflows ✅ Working
- [ ] **3D Viewport Integration**: Real-time mesh preview within node editor
  - [ ] **ViewportWidget**: Embeddable 3D viewport using OpenGL/ImGui
  - [ ] **Camera Controls**: Mouse-based orbit, pan, zoom with smooth interpolation
  - [ ] **Mesh Rendering**: Vertex buffer objects with dynamic updates
  - [ ] **Selection System**: Click-to-select meshes with visual highlighting
  - [ ] **Gizmos**: Transform handles for direct mesh manipulation
- [ ] **Advanced Visualization**:
  - [ ] **Multi-Viewport**: Multiple camera angles and split-screen views
  - [ ] **Render Modes**: Wireframe, solid, normal maps, UV coordinates
  - [ ] **Animation Support**: Keyframe animation and parameter interpolation
  - [ ] **Performance Profiler**: Real-time performance metrics and bottleneck analysis

### 📊 Performance & Scalability - Production Ready
- [ ] **Benchmarking Suite**: Performance regression testing for procedural workflows
- [ ] **Parallel Processing**: OpenMP for multi-threaded SOP operations
- [ ] **LOD System**: Level-of-detail for large procedural scene management
- [ ] **Streaming**: Support for procedural meshes larger than available RAM
- [ ] **GPU Memory Management**: Efficient GPU buffer pooling and reuse
- [ ] **Viewport Optimization**: Frustum culling, occlusion culling, instanced rendering

---
*Last Updated: July 17, 2025 - Architecture Planning Phase*
*Current Focus: Clean architecture foundation → Real-time visualization system*
