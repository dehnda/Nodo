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
