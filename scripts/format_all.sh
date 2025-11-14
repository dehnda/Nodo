#!/bin/bash
# Format all C++ source files in the Nodo project using clang-format

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/.."

echo "Formatting all C++ files in the Nodo project..."

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed."
    echo "Install it with: sudo apt install clang-format"
    exit 1
fi

# Find and format all C++ files
find "$PROJECT_ROOT/nodo_core" \
     "$PROJECT_ROOT/nodo_studio" \
     "$PROJECT_ROOT/nodo_cli" \
     "$PROJECT_ROOT/tests" \
     \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) \
     -not -path "*/build/*" \
     -not -path "*/external/*" \
     -not -path "*/.cache/*" \
     -exec clang-format -i {} +

echo "✅ Formatting complete!"
echo ""
echo "Formatted files in:"
echo "  - nodo_core/"
echo "  - nodo_studio/"
echo "  - nodo_cli/"
echo "  - tests/"
