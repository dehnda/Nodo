#!/bin/bash
# Generate code coverage report for Nodo project

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_DIR"

echo "=== Generating Code Coverage Report ==="
echo ""

# Step 1: Configure with coverage flags
echo "Step 1: Configuring CMake with coverage flags..."
cmake --preset=conan-debug \
    -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
    -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
    > /dev/null

# Step 2: Clean and rebuild
echo "Step 2: Rebuilding with coverage instrumentation..."
cmake --build --preset=conan-debug --clean-first 2>&1 | grep -E "(Built target|Linking)" | tail -5

# Step 3: Run tests
echo ""
echo "Step 3: Running tests..."
./build/Debug/tests/nodo_tests --gtest_brief=1 2>&1 | tail -3

# Step 4: Generate coverage data
echo ""
echo "Step 4: Capturing coverage data..."
lcov --capture \
    --directory build/Debug/nodo_core \
    --output-file coverage.info \
    2>&1 | grep -E "(Processing|Finished)" | tail -3

# Step 5: Filter coverage data (only nodo_core)
echo ""
echo "Step 5: Filtering coverage data (nodo_core only)..."
lcov --remove coverage.info \
    '/usr/*' \
    '*/external/*' \
    '*/tests/*' \
    '*/build/*' \
    '*/nodo_studio/*' \
    '*/nodo_cli/*' \
    '*.conan2*' \
    -o coverage_filtered.info \
    2>&1 | tail -3

# Step 6: Generate HTML report
echo ""
echo "Step 6: Generating HTML report..."
genhtml coverage_filtered.info \
    --output-directory coverage_report \
    2>&1 | grep -E "Overall" | head -2

echo ""
echo "=== Coverage Report Complete ==="
echo ""
echo "Summary:"
lcov --summary coverage_filtered.info 2>&1 | grep -E "(lines|functions)"
echo ""
echo "View the report:"
echo "  HTML: file://$PROJECT_DIR/coverage_report/index.html"
echo "  Or run: xdg-open coverage_report/index.html"
echo ""
