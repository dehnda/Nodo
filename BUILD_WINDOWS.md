# Building NodeFluxEngine on Windows

This guide explains how to build NodeFluxEngine natively on Windows using Visual Studio 2022 and Conan.

## Prerequisites

### 1. Install Visual Studio 2022

Download and install [Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/downloads/) (free).

During installation, select the **"Desktop development with C++"** workload, which includes:
- MSVC C++ compiler
- Windows SDK
- CMake tools (optional, but convenient)

### 2. Install CMake

**Option A: Use CMake bundled with Visual Studio** (already installed if you selected CMake tools)

**Option B: Install standalone CMake**
```powershell
# Using winget (Windows Package Manager)
winget install Kitware.CMake

# Or download from https://cmake.org/download/
```

Verify installation:
```powershell
cmake --version
```

### 3. Install Python and Conan

**Install Python 3.8 or later:**
```powershell
# Using winget
winget install Python.Python.3.12

# Or download from https://www.python.org/downloads/
```

**Install Conan:**
```powershell
pip install conan
```

Verify installation:
```powershell
conan --version
```

**Configure Conan profile (first time only):**
```powershell
conan profile detect
```

### 4. Install Ninja (Optional but Recommended)

Ninja provides faster builds than MSBuild:
```powershell
winget install Ninja-build.Ninja
```

## Building the Project

### Accessing the Project from Windows

If your project is in WSL, you can access it from Windows:

```powershell
# Navigate to WSL path from Windows
cd \\wsl.localhost\Ubuntu\home\daniel\projects\NodeFluxEngine
```

**OR** copy the project to a Windows directory for better performance:

```powershell
# Copy to Windows (one-time)
Copy-Item -Recurse \\wsl.localhost\Ubuntu\home\daniel\projects\NodeFluxEngine C:\Dev\NodeFluxEngine
cd C:\Dev\NodeFluxEngine
```

### Build Steps

#### 1. Open "Developer Command Prompt for VS 2022" or "Developer PowerShell for VS 2022"

You can find these in the Start Menu under "Visual Studio 2022" folder. This ensures the MSVC compiler is in your PATH.

#### 2. Navigate to the project directory

```powershell
cd C:\path\to\NodeFluxEngine
```

#### 3. Install dependencies with Conan

**For Debug build:**
```powershell
conan install . --output-folder=build-windows --build=missing -s build_type=Debug
```

**For Release build:**
```powershell
conan install . --output-folder=build-windows --build=missing -s build_type=Release
```

This will download and build all dependencies (Eigen, Qt, CGAL, etc.). **This may take 15-30 minutes the first time.**

#### 4. Configure CMake

```powershell
# For Debug
cmake --preset conan-debug

# For Release
cmake --preset conan-release
```

#### 5. Build the project

```powershell
# For Debug
cmake --build --preset conan-debug --parallel

# For Release
cmake --build --preset conan-release --parallel
```

**OR** build specific targets:

```powershell
# Build only the Studio application
cmake --build --preset conan-release --target nodeflux_studio --parallel

# Build only tests
cmake --build --preset conan-release --target nodeflux_tests --parallel
```

### 6. Run the application

```powershell
# Debug build
.\out\build\conan-debug\nodeflux_studio\nodeflux_studio.exe

# Release build
.\out\build\conan-release\nodeflux_studio\nodeflux_studio.exe
```

## Alternative: Using Visual Studio IDE

You can also open the project directly in Visual Studio 2022:

1. Open Visual Studio 2022
2. File → Open → CMake... → Select `CMakeLists.txt`
3. Visual Studio will automatically detect the CMake presets
4. Select the desired preset from the toolbar (conan-debug or conan-release)
5. Build → Build All (or press Ctrl+Shift+B)

## Troubleshooting

### "conan: command not found"

Make sure Python's Scripts directory is in your PATH:
```powershell
$env:PATH += ";$env:USERPROFILE\AppData\Local\Programs\Python\Python312\Scripts"
```

Or reinstall Python with "Add to PATH" option checked.

### "CMake could not find MSVC compiler"

Make sure you're using "Developer Command Prompt for VS 2022" or "Developer PowerShell for VS 2022", not the regular command prompt.

### Qt plugin errors when running

Make sure the Qt DLLs are in your PATH:
```powershell
$env:PATH += ";C:\Users\YourUser\.conan2\p\b\qt<hash>\p\bin"
```

Or copy the Qt DLLs next to your executable.

### Slow builds

- Use Ninja instead of MSBuild (install via `winget install Ninja-build.Ninja`)
- Use `/MP` flag for parallel compilation with MSVC
- Build in Release mode: smaller binaries, faster compilation

### Clean rebuild

```powershell
# Remove build directory
Remove-Item -Recurse -Force build-windows
Remove-Item -Recurse -Force out

# Remove generated presets
Remove-Item CMakePresets.json -ErrorAction SilentlyContinue

# Start fresh
conan install . --output-folder=build-windows --build=missing -s build_type=Release
cmake --preset conan-release
cmake --build --preset conan-release --parallel
```

## Build Configurations

### Debug Build
- Includes debug symbols
- No optimizations
- Easier debugging
- Larger binary size
- Slower runtime performance

```powershell
conan install . --output-folder=build-windows --build=missing -s build_type=Debug
cmake --preset conan-debug
cmake --build --preset conan-debug --parallel
```

### Release Build
- Optimizations enabled
- No debug symbols (unless configured)
- Smaller binary size
- Faster runtime performance
- Harder to debug

```powershell
conan install . --output-folder=build-windows --build=missing -s build_type=Release
cmake --preset conan-release
cmake --build --preset conan-release --parallel
```

## Next Steps

After building:
- Run tests: `.\out\build\conan-release\tests\nodeflux_tests.exe`
- Package the application with dependencies (see PACKAGING.md - TODO)
- Create an installer (see INSTALLER.md - TODO)

## Tips

1. **Keep WSL and Windows builds separate**: Use `build/` for Linux, `build-windows/` for Windows
2. **Use Release builds for distribution**: They're optimized and smaller
3. **Run tests before distributing**: `.\out\build\conan-release\tests\nodeflux_tests.exe`
4. **Qt deployment**: Use `windeployqt` to package Qt dependencies with your executable
