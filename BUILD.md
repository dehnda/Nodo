# Building Nodo

This guide covers building Nodo on Linux, macOS, and Windows.

## Prerequisites

### All Platforms

- **CMake 3.20+**
- **C++20 compatible compiler**
- **Python 3.8+**
- **Conan 2.x** package manager

### Platform-Specific Requirements

#### Linux
- **GCC 11+** or **Clang 12+**
- Development packages: `libgl1-mesa-dev`, `libxcb-*-dev` (for Qt)

#### macOS
- **Xcode Command Line Tools** or **Clang 12+**
```bash
xcode-select --install
```

#### Windows
- **Visual Studio 2022** with "Desktop development with C++" workload
- Includes MSVC compiler, Windows SDK, and CMake tools

### Install Conan

```bash
# All platforms
pip install "conan>=2.0"

# Configure Conan profile (first time only)
conan profile detect
```

## Quick Start

```bash
# 1. Clone the repository
git clone https://github.com/dehnda/nodo.git
cd nodo

# 2. Install dependencies
conan install . --build=missing

# 3. Configure
cmake --preset conan-debug

# 4. Build
cmake --build --preset conan-debug --parallel

# 5. Run tests (optional)
./build/Debug/tests/nodo_tests              # Linux/macOS
.\build\Debug\tests\nodo_tests.exe          # Windows

# 6. Run the application
./build/Debug/nodo_studio/nodo_studio       # Linux/macOS
.\build\Debug\nodo_studio\nodo_studio.exe   # Windows (see note below)
```

### Windows: Running with Qt DLLs

On Windows, Qt DLLs must be on PATH. Use the Conan run environment:

```powershell
# Option 1: Use the run environment script
cmd /c "build\Debug\build\generators\conanrun.bat && build\Debug\nodo_studio\nodo_studio.exe"

# Option 2: Use the CMake target (if configured)
cmake --build --preset conan-debug --target run-studio
```

## Build Configurations

### Debug Build (Default)
- Debug symbols enabled
- No optimizations
- Easier debugging

```bash
conan install . --build=missing -s build_type=Debug
cmake --preset conan-debug
cmake --build --preset conan-debug --parallel
```

### Release Build
- Optimizations enabled
- Smaller binaries
- Faster runtime

```bash
conan install . --build=missing -s build_type=Release
cmake --preset conan-default
cmake --build --preset conan-default --parallel
```

## Building Specific Targets

```bash
# Build only the core library
cmake --build --preset conan-debug --target nodo_core

# Build only the Studio application
cmake --build --preset conan-debug --target nodo_studio

# Build only tests
cmake --build --preset conan-debug --target nodo_tests

# Build the CLI tool
cmake --build --preset conan-debug --target nodo_cli
```

## Enabling Tests

Tests are disabled by default to speed up dependency installation. To enable:

```bash
conan install . --build=missing -o nodo:with_tests=True
cmake --preset conan-debug
cmake --build --preset conan-debug --parallel
```

## Platform-Specific Instructions

### Windows: Using Visual Studio IDE

1. Open Visual Studio 2022
2. **File → Open → CMake...** → Select `CMakeLists.txt`
3. Visual Studio will detect CMake presets automatically
4. Select **conan-debug** or **conan-default** from the toolbar
5. **Build → Build All** (or press Ctrl+Shift+B)

### Windows: Release Packaging

For end-to-end release builds with testing and deployment:

```powershell
# Basic release build with tests
pwsh -File tools/buildscripts/windows/release.ps1

# Skip tests for faster builds
pwsh -File tools/buildscripts/windows/release.ps1 -SkipTests

# Deploy Qt DLLs next to executable and run
pwsh -File tools/buildscripts/windows/release.ps1 -Deploy -Run

# Create distributable package (zip)
pwsh -File tools/buildscripts/windows/release.ps1 -Deploy -Package

# Debug configuration instead of Release
pwsh -File tools/buildscripts/windows/release.ps1 -Configuration Debug
```

### Windows: Pre-release Verification

Run tests before building:

```powershell
pwsh -File tools/buildscripts/windows/verify-release.ps1
```

This will:
- Install dependencies with tests enabled
- Configure CMake
- Build and run tests
- Build nodo_studio

### Linux/macOS: Using System Qt

If you have Qt 6.7+ installed system-wide:

```bash
conan install . --build=missing -o nodo:system_qt=True
cmake --preset conan-debug
cmake --build --preset conan-debug --parallel
```

## VS Code Integration

The project includes pre-configured tasks accessible via **Ctrl+Shift+P → "Tasks: Run Task"**:

- **CMake Configure & Build** - Default build (Ctrl+Shift+B)
- **Run Tests** - Execute test suite
- **Conan Install Dependencies** - Refresh dependencies
- **Full Rebuild** - Clean and rebuild

Recommended: Use `cmake --preset conan-debug` for development work instead of the default preset.

## Troubleshooting

### "conan: command not found"

Ensure Python's Scripts directory is in your PATH:

```bash
# Linux/macOS
export PATH="$HOME/.local/bin:$PATH"

# Windows (PowerShell)
$env:PATH += ";$env:USERPROFILE\AppData\Local\Programs\Python\Python312\Scripts"
```

Or reinstall Python with "Add to PATH" option checked.

### "CMakePresets.json not found"

Re-run Conan to regenerate presets:

```bash
conan install . --build=missing
```

### Dependency Build Failures

Clean and rebuild dependencies:

```bash
rm -rf build CMakeUserPresets.json
conan install . --build=missing
cmake --preset conan-debug
```

### Windows: "CMake could not find MSVC compiler"

Ensure Visual Studio 2022 is installed with the "Desktop development with C++" workload. The project automatically configures MSVC without requiring vcvarsall.bat.

### Windows: Qt DLL Errors

The application cannot find Qt DLLs. Use one of these solutions:

1. Run through Conan environment (recommended):
   ```powershell
   cmd /c "build\Debug\build\generators\conanrun.bat && build\Debug\nodo_studio\nodo_studio.exe"
   ```

2. Use deployment target:
   ```powershell
   cmake --build --preset conan-debug --target deploy-windows
   # Executable with DLLs in: build/Debug/nodo_studio/Debug/deploy/Debug
   ```

### Slow Builds

- **Use Ninja**: Install via package manager and Conan will auto-detect it
  ```bash
  # Linux
  sudo apt install ninja-build
  
  # macOS
  brew install ninja
  
  # Windows
  winget install Ninja-build.Ninja
  ```

- **Enable parallel builds**: Use `--parallel` flag (already shown in examples)

- **Build Release configuration**: Smaller binaries compile faster

### Clean Rebuild

```bash
# Remove all build artifacts
rm -rf build CMakeUserPresets.json

# Reinstall dependencies
conan install . --build=missing

# Reconfigure and build
cmake --preset conan-debug
cmake --build --preset conan-debug --parallel
```

## Dependencies

All dependencies are automatically managed by Conan:

- **Eigen 3.4.0** - Linear algebra
- **Qt 6.7.0** - GUI framework
- **fmt 10.2.1** - String formatting
- **nlohmann_json 3.11.3** - JSON parsing
- **Manifold 3.2.1** - Boolean operations
- **PMP 3.0.0** - Mesh processing
- **exprtk 0.0.2** - Expression evaluation
- **Google Test 1.14.0** - Testing framework

## Next Steps

After building successfully:

- See [CONTRIBUTING.md](CONTRIBUTING.md) for development workflow
- See [ROADMAP.md](ROADMAP.md) for project status and plans
- See [README.md](README.md) for usage examples and API overview

## Project Structure

```
build/
  Debug/              # Debug build artifacts (Linux/macOS)
  Release/            # Release build artifacts (Linux/macOS)
  Debug/              # Debug build artifacts (Windows)
  Release/            # Release build artifacts (Windows)
  
CMakeUserPresets.json # Generated by Conan (git-ignored)
```

Keep build directories separate if building for multiple platforms on the same machine.
