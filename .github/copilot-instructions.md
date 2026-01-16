# Copilot Instructions for Nodo Project

## Build Configuration

### CMake Presets
- **Always use the `conan-debug` preset** for building the project
- Configure command: `cmake --preset=conan-debug`
- Build command: `cmake --build --preset=conan-debug`

### Build Process
1. First run: `conan install . --build=missing` (if dependencies need to be installed)
2. Configure: `cmake --preset=conan-debug`
3. Build: `cmake --build --preset=conan-debug`

### Quick Build Task
Use the VS Code task "CMake Configure & Build" but note it uses the default preset. For development work, prefer using the `conan-debug` preset directly.

## Project Structure
- `nodo_core/`: Core library with geometry operations (SOPs)
- `nodo_studio/`: Qt-based GUI application
- `tests/`: Unit tests
- `external/`: External dependencies (iconoir, xatlas)

## Dependencies
- Qt6
- Manifold (geometry library)
- Eigen3
- fmt
- Clipper2
- nlohmann_json
- exprtk
- GTest (for tests)

## Coding Standards

### Language & Style
- **C++20** standard
- Project uses **clang-format**, **clang-tidy**, and **clangd** for code quality
- All style rules are defined in `.clang-format` and `.clang-tidy` at project root

### Naming Conventions (enforced by clang-tidy)
- **Classes/Structs/Enums**: `PascalCase` (e.g., `GeometryContainer`, `NodeGraph`)
- **Methods/Functions**: `camelBack` (e.g., `setupShaders()`, `computeNormals()`)
- **Variables/Parameters**: `snake_case` (e.g., `vertex_count`, `node_id`)
- **Member Variables**: `snake_case` with trailing underscore (e.g., `camera_distance_`, `frame_count_`)
- **Constants**: `UPPER_CASE` (e.g., `MAX_VERTICES`, `NODE_VERSION`)
- **Namespaces**: `snake_case` (e.g., `nodo::core`, `nodo::sop`)

### Formatting Rules (clang-format)
- **Indentation**: 2 spaces (no tabs)
- **Line Length**: 120 characters max
- **Braces**: K&R style (opening brace on same line)
- **Pointer/Reference**: Left-aligned (e.g., `int* ptr`, `const std::string& str`)
- **Spacing**: Space after keywords, before `{`, in control statements

### Include Order (clang-format enforced)
1. Project headers: `"nodo/..."`
2. Other local headers: `"..."`
3. Qt headers: `<Q...>`
4. Eigen headers: `<Eigen/...>`
5. Standard library: `<vector>`, `<string>`, etc.
6. Other system headers

### Best Practices
- Use `auto` for complex types, explicit types for clarity
- Prefer `const` correctness
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) over raw pointers
- Document public APIs with Doxygen-style comments (`/** @brief ... */`)
- Write unit tests for new features (GTest framework)
