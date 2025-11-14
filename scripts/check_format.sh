#!/bin/bash
# Check if C++ files are properly formatted (for CI/CD)

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/.."

echo "Checking C++ formatting..."

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed."
    exit 1
fi

# Find all C++ files and check formatting
UNFORMATTED_FILES=()

while IFS= read -r -d '' file; do
    if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
        UNFORMATTED_FILES+=("$file")
    fi
done < <(find "$PROJECT_ROOT/nodo_core" \
              "$PROJECT_ROOT/nodo_studio" \
              "$PROJECT_ROOT/nodo_cli" \
              "$PROJECT_ROOT/tests" \
              \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) \
              -not -path "*/build/*" \
              -not -path "*/external/*" \
              -not -path "*/.cache/*" \
              -print0)

if [ ${#UNFORMATTED_FILES[@]} -eq 0 ]; then
    echo "✅ All files are properly formatted!"
    exit 0
else
    echo "❌ The following files need formatting:"
    printf '  %s\n' "${UNFORMATTED_FILES[@]}"
    echo ""
    echo "Run './scripts/format_all.sh' to format them automatically."
    exit 1
fi
