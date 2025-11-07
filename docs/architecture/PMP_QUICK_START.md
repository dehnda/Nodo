# PMP Integration: Quick Start Guide

## Pre-Integration Checklist

Before starting, ensure:

- [ ] Current M3.3 work is complete and committed
- [ ] All existing tests are passing
- [ ] Team has reviewed and approved the strategy
- [ ] Branch created: `feature/pmp-integration`

---

## Step-by-Step Integration Guide

### Day 1: Setup & Dependencies

#### 1. Check PMP Availability

```bash
# Check if PMP is in Conan Center
conan search pmp* --remote=all

# Or check manually at:
# https://conan.io/center/
```

#### 2A. If PMP is in Conan (Easy Path)

```python
# conanfile.py
def requirements(self):
    # ... existing ...
    self.requires("pmp-library/3.0.0")  # Adjust version as needed
```

```bash
# Install dependencies
conan install . --build=missing

# Configure
cmake --preset=conan-debug

# Build
cmake --build --preset=conan-debug
```

#### 2B. If PMP Not in Conan (Build from Source)

**Option 1: CMake FetchContent**

Add to `nodo_core/CMakeLists.txt` before defining nodo_core target:

```cmake
# External: PMP Library
include(FetchContent)

message(STATUS "Fetching PMP Library...")

FetchContent_Declare(
    pmp
    GIT_REPOSITORY https://github.com/pmp-library/pmp-library.git
    GIT_TAG        3.0.0
    GIT_SHALLOW    TRUE
)

# Configure PMP options
set(PMP_BUILD_EXAMPLES OFF CACHE BOOL "")
set(PMP_BUILD_TESTS OFF CACHE BOOL "")
set(PMP_BUILD_DOCS OFF CACHE BOOL "")
set(PMP_BUILD_VIS OFF CACHE BOOL "")  # Don't build visualization

FetchContent_MakeAvailable(pmp)

message(STATUS "PMP Library ready")
```

**Option 2: Git Submodule** (if FetchContent doesn't work)

```bash
# Add PMP as submodule
cd external/
git submodule add https://github.com/pmp-library/pmp-library.git pmp
cd pmp
git checkout 3.0.0
cd ../..
git add .gitmodules external/pmp
git commit -m "Add PMP library as submodule"
```

Then in `CMakeLists.txt`:

```cmake
# Add PMP subdirectory
add_subdirectory(external/pmp EXCLUDE_FROM_ALL)
```

#### 3. Link PMP to nodo_core

```cmake
# nodo_core/CMakeLists.txt

# Link libraries
target_link_libraries(nodo_core
    PUBLIC
        Eigen3::Eigen
        fmt::fmt
        manifold::manifold
        nlohmann_json::nlohmann_json
        exprtk::exprtk
    PRIVATE
        pmp  # Add this (or pmp::pmp if using Conan)
)
```

#### 4. Test Build

```bash
# Clean build
rm -rf build/
cmake --preset=conan-debug
cmake --build --preset=conan-debug

# Should compile without errors
```

---

### Day 2-3: Converter Implementation

#### 1. Create Directory Structure

```bash
mkdir -p nodo_core/include/nodo/processing
mkdir -p nodo_core/src/processing
mkdir -p tests/processing
```

#### 2. Copy Template Files

Use the implementations from `PMP_IMPLEMENTATION_PLAN.md`:

1. `nodo_core/include/nodo/processing/pmp_converter.hpp`
2. `nodo_core/src/processing/pmp_converter.cpp`

#### 3. Update CMakeLists.txt

```cmake
# nodo_core/CMakeLists.txt

set(NODO_CORE_SOURCES
    # ... existing sources ...
    
    # Processing (PMP integration)
    src/processing/pmp_converter.cpp
)
```

#### 4. Create and Run Tests

1. Create `tests/test_pmp_converter.cpp` (from implementation plan)
2. Add to tests CMakeLists.txt:

```cmake
# tests/CMakeLists.txt

add_executable(nodo_tests
    # ... existing tests ...
    test_pmp_converter.cpp
)
```

3. Run tests:

```bash
cmake --build --preset=conan-debug
./build/Debug/tests/nodo_tests --gtest_filter="PMPConverter*"
```

Expected output:
```
[==========] Running 7 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 7 tests from PMPConverterTest
[ RUN      ] PMPConverterTest.MeshToPMP
[       OK ] PMPConverterTest.MeshToPMP (5 ms)
...
[----------] 7 tests from PMPConverterTest (42 ms total)
[==========] 7 tests from 1 test suite ran. (43 ms total)
[  PASSED  ] 7 tests.
```

---

### Day 4-6: Decimation Implementation

#### 1. Create Files

1. `nodo_core/include/nodo/processing/decimation.hpp` (from plan)
2. `nodo_core/src/processing/decimation.cpp` (from plan)
3. `nodo_core/include/nodo/sop/decimation_sop.hpp` (from plan)
4. `nodo_core/src/sop/decimation_sop.cpp` (from plan)

#### 2. Update CMakeLists.txt

```cmake
set(NODO_CORE_SOURCES
    # ... existing ...
    
    # Processing
    src/processing/pmp_converter.cpp
    src/processing/decimation.cpp  # Add this
    
    # ... existing SOPs ...
    src/sop/decimation_sop.cpp  # Add this
)
```

#### 3. Register in Factory

Edit `nodo_core/src/sop/sop_factory.cpp`:

```cpp
#include "nodo/sop/decimation_sop.hpp"

std::unique_ptr<SOPNode> SOPFactory::create_node(
    const std::string& type,
    const std::string& name) {
    
    // ... existing nodes ...
    
    if (type == "decimation") {
        return std::make_unique<DecimationSOP>(name);
    }
    
    // ... rest ...
}
```

#### 4. Add Node to UI

Edit `nodo_studio/src/node_library_panel.cpp`:

```cpp
// In the Generator category or new Processing category
addNodeButton("Decimation", "decimation", 
              "Reduce mesh triangle count");
```

#### 5. Create Tests

Create `tests/test_decimation.cpp` (from plan) and run:

```bash
cmake --build --preset=conan-debug
./build/Debug/tests/nodo_tests --gtest_filter="Decimation*"
```

#### 6. Test in UI

```bash
# Run nodo_studio
./build/Debug/nodo_studio/nodo_studio
```

1. Create Sphere node
2. Create Decimation node
3. Connect Sphere → Decimation
4. Adjust parameters
5. Verify mesh simplifies correctly

---

### Day 7-8: Documentation

#### 1. Create Node Documentation

Create `docs/nodes/processing/decimation.md`:

```markdown
# Decimation

**Category:** Processing

## Description

Reduces the triangle count of a mesh while preserving shape quality.

## Inputs

**Geometry** (0)
- Input mesh to decimate
- Must be a triangle mesh

## Parameters

### Target

**Target Mode** (`int`)
- Options: `Percentage`, `Vertex Count`
- Default: `Percentage`

**Target Percentage** (`float`)
- Range: 0.01 to 1.0
- Default: 0.5
- Percentage of original vertices to keep

... (continue with all parameters)

## Example Usage

```
Sphere (subdivisions=5)
  ↓
Decimation (50%)
  ↓
Output (reduced mesh)
```

## Tips

- Use "Quality" preset for best results
- Enable "Preserve Features" for hard surface models
- Adjust "Normal Deviation" to control smoothness
```

#### 2. Update Main Index

Add to `docs/nodes/index.md`:

```markdown
## Processing (New!)

Mesh processing and optimization operations.

- **[Decimation](processing/decimation.md)** ✅ - Reduce triangle count
```

---

## Verification Checklist

Before considering Phase 1 complete:

### Code Quality
- [ ] All files compile without warnings
- [ ] No memory leaks (run with valgrind)
- [ ] Code follows Nodo style guidelines
- [ ] Comments and documentation complete

### Testing
- [ ] Unit tests pass (>80% coverage)
- [ ] Integration tests pass
- [ ] Manual UI testing successful
- [ ] Edge cases handled (empty mesh, etc.)

### Integration
- [ ] Node appears in UI
- [ ] Parameters work correctly
- [ ] Connects properly in node graph
- [ ] Output geometry valid

### Documentation
- [ ] API documentation complete
- [ ] User-facing docs written
- [ ] Examples provided
- [ ] Code comments clear

---

## Common Issues & Solutions

### Issue: PMP headers not found

**Solution:**
```cmake
# Explicitly add include directories
target_include_directories(nodo_core
    PRIVATE
        ${CMAKE_SOURCE_DIR}/external/pmp/src
)
```

### Issue: Linker errors with PMP

**Solution:**
```cmake
# Make sure to link the library correctly
target_link_libraries(nodo_core PRIVATE pmp)

# If using FetchContent, might need:
target_link_libraries(nodo_core PRIVATE pmp_static)
```

### Issue: Conversion doesn't preserve normals

**Solution:**
```cpp
// In pmp_converter.cpp, after converting mesh:
if (!pmp_mesh.has_vertex_property("v:normal")) {
    pmp::vertex_normals(pmp_mesh);
}
```

### Issue: Tests fail with "invalid mesh"

**Solution:**
```cpp
// Ensure test meshes are triangulated
auto sphere = SphereGenerator::generate_icosphere(1.0, 2);
// Icosphere is already triangulated, but UV sphere might not be
```

### Issue: Decimation produces invalid output

**Solution:**
```cpp
// Check if input is manifold and closed
// PMP requires valid input meshes
auto validation_error = PMPConverter::validate_for_pmp(input);
if (!validation_error.empty()) {
    // Handle error
}
```

---

## Performance Benchmarks

Run these to ensure performance is acceptable:

```cpp
// tests/benchmark_decimation.cpp
TEST(DecimationBenchmark, LargeMesh) {
    auto sphere = SphereGenerator::generate_icosphere(1.0, 6);
    // ~10k vertices
    
    auto start = std::chrono::high_resolution_clock::now();
    
    DecimationParams params;
    params.target_percentage = 0.1f;
    auto result = Decimation::decimate(*sphere, params);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    std::cout << "Decimation took: " << duration << "ms" << std::endl;
    EXPECT_LT(duration, 1000);  // Should be under 1 second
}
```

Expected performance:
- 1K vertices: < 50ms
- 10K vertices: < 500ms
- 100K vertices: < 5s

---

## Git Workflow

### Committing Progress

```bash
# After Day 1 (setup)
git add conanfile.py nodo_core/CMakeLists.txt
git commit -m "feat: Add PMP library dependency"

# After Day 2-3 (converter)
git add nodo_core/include/nodo/processing/pmp_converter.hpp
git add nodo_core/src/processing/pmp_converter.cpp
git add tests/test_pmp_converter.cpp
git commit -m "feat: Add PMP converter infrastructure"

# After Day 4-6 (decimation)
git add nodo_core/include/nodo/processing/decimation.hpp
git add nodo_core/src/processing/decimation.cpp
git add nodo_core/include/nodo/sop/decimation_sop.hpp
git add nodo_core/src/sop/decimation_sop.cpp
git add nodo_core/src/sop/sop_factory.cpp
git add tests/test_decimation.cpp
git commit -m "feat: Implement decimation SOP with PMP backend"

# After Day 7-8 (docs)
git add docs/nodes/processing/decimation.md
git add docs/nodes/index.md
git commit -m "docs: Add decimation node documentation"
```

### Creating Pull Request

```bash
# Push branch
git push origin feature/pmp-integration

# Create PR with description:
# - What: PMP library integration Phase 1 (Decimation)
# - Why: Add production-quality mesh decimation
# - How: Wrapped PMP library, added DecimationSOP
# - Tests: All passing, benchmarked
```

---

## Next Steps After Phase 1

Once decimation is working:

1. **Week 2-3:** Remeshing (similar pattern)
2. **Week 4:** Smoothing (enhance LaplacianSOP)
3. **Week 5:** Subdivision (enhance SubdivisionSOP)
4. **Week 6:** Hole filling

Each phase follows the same pattern:
1. Create wrapper class
2. Create/enhance SOP
3. Add tests
4. Document
5. Commit

---

## Support Resources

### PMP Documentation
- Website: https://www.pmp-library.org/
- API Docs: https://www.pmp-library.org/modules.html
- Tutorial: https://www.pmp-library.org/tutorial.html
- GitHub: https://github.com/pmp-library/pmp-library

### Nodo Resources
- Architecture docs: `docs/architecture/`
- Existing SOPs: `nodo_core/src/sop/`
- Test patterns: `tests/`

### Getting Help
- PMP Issues: https://github.com/pmp-library/pmp-library/issues
- PMP Discussions: https://github.com/pmp-library/pmp-library/discussions

---

## Success Criteria

Phase 1 is complete when:

✅ PMP library building correctly  
✅ PMPConverter tests pass  
✅ Decimation wrapper tests pass  
✅ DecimationSOP works in UI  
✅ Documentation complete  
✅ No regressions in existing tests  
✅ Code reviewed and merged  

**Estimated Time: 8 days of focused work**

Good luck! 🚀
