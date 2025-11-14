# Contributing to Nodo

Thank you for your interest in contributing to Nodo! This document provides guidelines for contributing to the project.

## Code Style Guide

### General Principles

- **Consistency**: Follow the existing code style in the project
- **Readability**: Write code that is easy to understand and maintain
- **Documentation**: Document complex logic and public APIs
- **Testing**: Write tests for new features and bug fixes

### C++ Code Style

#### Formatting

We use **clang-format** for automatic code formatting. The configuration is in `.clang-format`.

**Key style rules:**
- **Indentation**: 2 spaces (no tabs)
- **Line length**: 80 characters maximum
- **Braces**: Same line (K&R style)
- **Pointer/Reference**: Align left (`int* ptr`, not `int *ptr`)

#### Naming Conventions

- **Classes/Structs/Enums**: `PascalCase`
  ```cpp
  class NodeGraph {};
  struct GeometryData {};
  enum class NodeType {};
  ```

- **Functions/Methods**: `snake_case`
  ```cpp
  void execute_node();
  bool is_valid() const;
  ```

- **Variables**: `snake_case`
  ```cpp
  int point_count = 0;
  std::string node_name;
  ```

- **Private Members**: `snake_case_` (trailing underscore)
  ```cpp
  class MyClass {
  private:
    int value_;
    std::string name_;
  };
  ```

- **Constants**: `UPPER_CASE`
  ```cpp
  constexpr int MAX_ITERATIONS = 100;
  const std::string DEFAULT_NAME = "Untitled";
  ```

- **Namespaces**: `snake_case`
  ```cpp
  namespace nodo::graph {}
  namespace nodo::sop {}
  ```

#### Header Guards

Use `#pragma once` instead of include guards:
```cpp
#pragma once

// Header content
```

#### Include Order

1. Related header (for .cpp files)
2. Project headers (`"nodo/..."`)
3. Other project headers
4. Qt headers (`<Q...>`)
5. Third-party library headers (`<Eigen/...>`, etc.)
6. Standard library headers (`<string>`, `<vector>`, etc.)

Example:
```cpp
#include "nodo/graph/node_graph.hpp"  // 1. Project headers
#include "NodeGraphWidget.h"           // 2. Other project headers

#include <QWidget>                     // 3. Qt headers
#include <QVBoxLayout>

#include <Eigen/Core>                  // 4. Third-party

#include <memory>                      // 5. Standard library
#include <string>
#include <vector>
```

#### Comments

- Use `//` for single-line comments
- Use `/* */` for multi-line comments
- Document public APIs with Doxygen-style comments:

```cpp
/**
 * @brief Brief description
 *
 * Detailed description if needed
 *
 * @param param1 Description of parameter
 * @param param2 Description of parameter
 * @return Description of return value
 */
int my_function(int param1, const std::string& param2);
```

#### Modern C++ Features

We use **C++20**. Prefer modern C++ idioms:

- **Use trailing return type syntax**: `auto function() -> ReturnType`
  ```cpp
  // Prefer this:
  auto get_node_count() -> size_t;
  auto process_geometry(const Mesh& mesh) -> std::optional<Result>;

  // Instead of:
  size_t get_node_count();
  std::optional<Result> process_geometry(const Mesh& mesh);
  ```
- Use `auto` when the type is obvious from context
- Use range-based for loops
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- Use `std::optional` instead of nullable pointers when appropriate
- Use `constexpr` for compile-time constants
- Use structured bindings: `auto [key, value] = map.find(...)`

### Python Code Style

For Python code (build scripts, tools):
- Follow [PEP 8](https://pep8.org/)
- Use 4 spaces for indentation
- Maximum line length: 88 characters (Black style)

### CMake Code Style

- Use 2 spaces for indentation
- Use lowercase for function names: `add_executable()`, `target_link_libraries()`
- Group related commands together

## Code Formatting Tools

### Automatic Formatting

Format all C++ files automatically:

```bash
# Format a single file
clang-format -i path/to/file.cpp

# Format all C++ files in the project
find nodo_core nodo_studio nodo_cli -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i
```

### Static Analysis

Run clang-tidy for code quality checks:

```bash
# Analyze a single file
clang-tidy path/to/file.cpp -- -Ibuild/Debug/_deps/

# Run on all files via CMake
cmake --build --preset=conan-debug --target tidy
```

### Editor Integration

#### VS Code

The project includes `.vscode/settings.json` with:
- Format on save enabled
- clang-format integration
- clang-tidy integration

Install the recommended extensions:
- C/C++ (ms-vscode.cpptools)
- CMake Tools (ms-vscode.cmake-tools)

#### Other Editors

- **CLion**: Built-in clang-format and clang-tidy support
- **Vim/Neovim**: Use plugins like `vim-clang-format`, `ale`, or `coc-clangd`
- **Emacs**: Use `clang-format.el` and `flycheck-clang-tidy`

## Git Workflow

1. **Fork** the repository
2. **Create** a feature branch: `git checkout -b feature/my-feature`
3. **Make** your changes following the style guide
4. **Format** your code: `clang-format -i` on modified files
5. **Test** your changes
6. **Commit** with clear, descriptive messages
7. **Push** to your fork
8. **Create** a Pull Request

### Commit Messages

Follow conventional commits:

```
type(scope): brief description

Longer description if needed

Fixes #123
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

## Building and Testing

See the main [README.md](README.md) for build instructions.

Always ensure:
- Code compiles without warnings
- Existing tests pass
- New tests are added for new features

### Build Commands

```bash
# Configure with conan-debug preset
cmake --preset=conan-debug

# Build
cmake --build --preset=conan-debug

# Run tests
ctest --preset=conan-debug
```

## Questions?

- Open an issue for bug reports or feature requests
- Start a discussion for questions
- Check existing documentation in the `docs/` folder

Thank you for contributing to Nodo! 🚀
