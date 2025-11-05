# Code Coverage Report

**Generated:** $(date)
**Total Tests:** 312 passing

## Overall Coverage

- **Lines:** 31.6% (9,042 of 28,598 lines)
- **Functions:** 11.3% (3,021 of 26,757 functions)

## Coverage by Module

### Expression System
- `ExpressionEvaluator.cpp`: **19.3%** (135 lines)
- `ExpressionEvaluator.h`: **66.7%** (3 lines)

### Graph System
- `node_graph.cpp`: **26.8%** (190 lines)
- `execution_engine.cpp`: **9.7%** (124 lines)
- `graph_serializer.cpp`: **2.9%** (278 lines)

### SOP Nodes (Selected)
- `merge_sop.cpp`: **18.2%** (11 lines)
- `normal_sop.cpp`: **16.1%** (31 lines)
- `bevel_sop.cpp`: **11.9%** (42 lines)
- `scatter_sop.cpp`: **10.3%** (29 lines)
- `wrangle_sop.cpp`: **10.5%** (171 lines)
- `array_sop.cpp`: **1.9%** (259 lines) ⚠️
- `boolean_sop.cpp`: **3.2%** (126 lines) ⚠️
- `polyextrude_sop.cpp`: **1.7%** (347 lines) ⚠️

### Core Systems
- `math.cpp`: **32.5%**
- `attribute_interpolation.cpp`: **28.8%**
- `attribute_set.cpp`: **20.8%**
- `element_topology.cpp`: **18.0%**

## Notes

- Low coverage in many SOP nodes indicates need for more integration tests
- ExpressionEvaluator has decent coverage but could be improved
- Graph system has moderate coverage through existing tests
- Array, Boolean, and PolyExtrude SOPs have very low coverage despite having unit tests

## Viewing the Report

Open the HTML report in your browser:
```bash
xdg-open coverage_report/index.html
```

Or view in VS Code:
```bash
code coverage_report/index.html
```

## Generating Coverage Again

```bash
# Reconfigure with coverage flags
cmake --preset=conan-debug -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage"

# Rebuild
cmake --build --preset=conan-debug --clean-first

# Run tests
./build/Debug/tests/nodo_tests

# Generate coverage report
lcov --capture --directory build/Debug/nodo_core --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/external/*' '*/tests/*' '*/build/*' -o coverage_filtered.info
genhtml coverage_filtered.info --output-directory coverage_report
```
