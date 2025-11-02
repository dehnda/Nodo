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
- C++20
- Follow existing code style in the project
