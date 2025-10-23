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

#### 1. Open a shell (no special VS prompt required)

The project now uses the Visual Studio CMake generator on Windows, so you don't need to run vcvarsall.bat. Just use a normal PowerShell/CMD.

#### 2. Navigate to the project directory

```powershell
cd C:\path\to\NodeFluxEngine
```

#### 3. Install dependencies with Conan

**For Debug build:**
```powershell
conan install . --output-folder=build/windows --build=missing -s build_type=Debug
```

**For Release build:**
```powershell
conan install . --output-folder=build/windows --build=missing -s build_type=Release
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

**OR** build a specific target:

```powershell
# Build only the Studio application
cmake --build --preset conan-release --target nodeflux_studio --parallel
```

### 6. Run the application (fix Qt6 DLLs)

On Windows, Qt DLLs (e.g., Qt6Widgets.dll) must be on PATH when you launch the app. The easiest way is to use the Conan Virtual Run Environment.

Option A — use the generated run env scripts (recommended):

```powershell
# Release build
cmd /c "build\windows\build\generators\conanrun.bat && build\windows\build\nodeflux_studio\nodeflux_studio.exe"

# Debug build
cmd /c "build\windows\build\generators\conanrunenv-debug-x86_64.bat && build\windows\build\nodeflux_studio\nodeflux_studio.exe"
```

If the run env scripts are missing, (re)generate them:

```powershell
# Release
conan install . --output-folder=build/windows -s build_type=Release -g VirtualRunEnv

# Debug
conan install . --output-folder=build/windows -s build_type=Debug -g VirtualRunEnv
```

Option B — deploy Qt next to the exe (distribution):

- Use Qt's windeployqt (provided by the Conan Qt package) or CMake's Qt deploy helpers to copy the required Qt DLLs into the app folder for shipping. This is typically used for packaging/installer workflows, not for local dev.

### 7. CMake convenience targets (Windows)

- Bundle Qt next to the exe (self-contained folder):
	```powershell
	cmake --build --preset conan-release --target deploy-windows
	```
	Output: `build/windows/build/nodeflux_studio/Release/deploy/Release`

- Run the Studio via Conan RunEnv (detached, for dev):
	```powershell
	cmake --build --preset conan-release --target run-studio
	```

- Run the deployed Studio (detached):
	```powershell
	cmake --build --preset conan-release --target run-studio-deploy
	```

### 8. Pre-release verification (run tests, then build)

If you want to gate your release build on tests, use one of these options:

- One-liner script (recommended):

```powershell
pwsh -File tools/buildscripts/windows/verify-release.ps1
```

This will:
- Install dependencies with tests enabled
- Configure CMake
- Build and run tests
- Build the Studio (and optionally deploy/run with `-WithDeploy`)

- Or, do it manually via CMake target (after enabling tests):

```powershell
conan install . --output-folder=build/windows -s build_type=Release -o nodefluxengine:with_tests=True --build=missing
cmake --preset conan-default
cmake --build --preset conan-release --target verify-release
```

If tests are disabled, `verify-release` will print a hint on how to enable them.

### 9. Top-level release script

For an end-to-end release flow (tests → build → optional deploy → optional run → optional zip package), use the top-level script:

```powershell
# Default: Release build, tests enabled, no deploy
pwsh -File .\tools\buildscripts\windows\release.ps1

# Skip tests
pwsh -File .\tools\buildscripts\windows\release.ps1 -SkipTests

# Deploy Qt next to the exe (windeployqt) and run the deployed app
pwsh -File .\tools\buildscripts\windows\release.ps1 -Deploy -Run

# Deploy and package the deploy folder as a zip (to ./out)
pwsh -File .\tools\buildscripts\windows\release.ps1 -Deploy -Package

# Use Debug instead of Release
pwsh -File .\tools\buildscripts\windows\release.ps1 -Configuration Debug
```

The script handles:
- Conan install (enabling tests only when not using -SkipTests)
- CMake configure (conan-default)
- Test run via the `verify-release` target
- Building `nodeflux_studio`
- Optional deploy/run/package steps

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

### Qt DLL or plugin errors when running

Run the app through the Conan Run Environment as shown above. It prepends all dependency bin directories (Qt, etc.) to PATH for the duration of the process.

## Compilers on Windows (MSVC, Clang-CL, MinGW)

- Default: MSVC via the Visual Studio generator (no vcvars required).
- Clang-CL (MSVC ABI compatible):
	- Keep the Qt package as-is (MSVC-built).
	- In your Conan profile, use MSVC settings but set the CMake toolset to ClangCL:
		- `tools.cmake.cmaketoolchain:generator=Visual Studio 17 2022`
		- `tools.cmake.cmaketoolchain:toolset=ClangCL`
	- Then re-run `conan install` and CMake.
- MinGW GCC:
	- You need Qt built for MinGW. Mixing MSVC-built Qt with MinGW-compiled binaries will fail.
	- Unless you have a MinGW Qt package in your remotes, this path requires building Qt yourself or switching to a distro that provides it (e.g., MSYS2).

Note: If you explicitly want Ninja with MSVC, either run from a Visual Studio Developer Prompt or use the Conan-generated `conanvcvars.bat` (located in `build/windows/build/generators`) to initialize the environment before configuring/building.

### Slow builds

- Use Ninja instead of MSBuild (install via `winget install Ninja-build.Ninja`)
- Use `/MP` flag for parallel compilation with MSVC
- Build in Release mode: smaller binaries, faster compilation

### Clean rebuild

```powershell
# Remove build directory
Remove-Item -Recurse -Force build/windows
Remove-Item -Recurse -Force out

# Remove generated presets
Remove-Item CMakePresets.json -ErrorAction SilentlyContinue

# Start fresh
conan install . --output-folder=build/windows --build=missing -s build_type=Release
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
conan install . --output-folder=build/windows --build=missing -s build_type=Debug
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
- Package the application with dependencies (see PACKAGING.md - TODO)
- Create an installer (see INSTALLER.md - TODO)

## Tips

1. **Keep WSL and Windows builds separate**: Use `build/` for Linux, `build/windows/` for Windows
2. **Use Release builds for distribution**: They're optimized and smaller
3. **Qt deployment**: Use `windeployqt` to package Qt dependencies with your executable

> Optional: If you ever need unit tests, enable them explicitly:
> - Conan: `-o nodefluxengine:with_tests=True`
> - CMake: tests are auto-toggled by the Conan toolchain; or set `-D NODEFLUX_BUILD_TESTS=ON` manually.
