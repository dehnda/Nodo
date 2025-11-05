# Nodo CLI - Command-Line Interface

Headless execution tool for batch processing and automation of Nodo node graphs.

## Quick Start

### Installation
The CLI tool is built automatically with Nodo:

```bash
# Build Nodo (includes CLI)
cmake --preset=conan-debug
cmake --build --preset=conan-debug

# CLI is located at:
./build/Debug/nodo_cli/nodo_cli
```

### Basic Usage
```bash
./build/Debug/nodo_cli/nodo_cli input.nfg output.obj
```

## Command Reference

### Syntax
```bash
nodo_cli <input.nfg> <output.obj> [options]
```

### Arguments
- **`input.nfg`** - Path to your Nodo graph file (required)
- **`output.obj`** - Path for exported mesh file (required)

### Options
| Option | Short | Description |
|--------|-------|-------------|
| `--verbose` | `-v` | Show detailed execution progress |
| `--stats` | `-s` | Display execution statistics |
| `--help` | `-h` | Show help message |

## Examples

### 1. Simple Export
```bash
nodo_cli my_graph.nfg output.obj
```

### 2. With Statistics
```bash
nodo_cli my_graph.nfg output.obj --stats
```

**Output:**
```
Statistics:
-----------
Nodes:        4
Points:       800
Primitives:   600
Execution:    6 ms
Output size:  35101 bytes

✓ Successfully exported to: output.obj
```

### 3. Verbose Mode
```bash
nodo_cli my_graph.nfg output.obj --verbose
```

**Output:**
```
Nodo CLI - Headless Execution
==============================

Input:  my_graph.nfg
Output: output.obj
Mode:   Verbose

Loading graph...
Loaded 4 nodes

Executing graph...

Execution complete

Exporting to OBJ...

✓ Successfully exported to: output.obj
```

### 4. Full Output
```bash
nodo_cli my_graph.nfg output.obj -v -s
```

## Automation Examples

### Batch Processing (Bash)
```bash
#!/bin/bash
# Process all .nfg files in a directory

for graph in scenes/*.nfg; do
    output="output/$(basename "$graph" .nfg).obj"
    echo "Processing: $graph"
    ./nodo_cli "$graph" "$output" --stats

    if [ $? -eq 0 ]; then
        echo "✓ Success: $output"
    else
        echo "✗ Failed: $graph"
    fi
done
```

### Python Integration
```python
import subprocess
import os

def generate_mesh(graph_file, output_file, verbose=False):
    """Execute Nodo graph and export mesh."""

    cmd = ['./nodo_cli', graph_file, output_file]
    if verbose:
        cmd.append('--verbose')

    result = subprocess.run(
        cmd,
        capture_output=True,
        text=True
    )

    if result.returncode == 0:
        print(f"✓ Generated: {output_file}")
        return True
    else:
        print(f"✗ Error: {result.stderr}")
        return False

# Usage
generate_mesh('scene.nfg', 'output.obj', verbose=True)
```

### CI/CD Integration (GitHub Actions)
```yaml
name: Generate Assets

on: [push]

jobs:
  generate-meshes:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Setup Nodo
        run: |
          cmake --preset=conan-debug
          cmake --build --preset=conan-debug --target nodo_cli

      - name: Generate meshes
        run: |
          ./build/Debug/nodo_cli/nodo_cli scenes/level1.nfg assets/level1.obj
          ./build/Debug/nodo_cli/nodo_cli scenes/level2.nfg assets/level2.obj

      - name: Upload artifacts
        uses: actions/upload-artifact@v3
        with:
          name: generated-meshes
          path: assets/*.obj
```

### Makefile Integration
```makefile
# Generate all meshes from graphs
GRAPHS := $(wildcard scenes/*.nfg)
MESHES := $(patsubst scenes/%.nfg,output/%.obj,$(GRAPHS))

all: $(MESHES)

output/%.obj: scenes/%.nfg
	@mkdir -p output
	./nodo_cli $< $@ --stats

clean:
	rm -rf output/*.obj
```

## Use Cases

### 1. Asset Pipelines
Generate game assets in build process:
```bash
# Game asset generation
nodo_cli assets/player_weapon.nfg game/models/weapon.obj
nodo_cli assets/environment.nfg game/models/level.obj
```

### 2. Server-Side Generation
Dynamic mesh generation for web services:
```bash
# Web service endpoint handler
./nodo_cli user_graphs/$USER_ID.nfg /var/www/meshes/$REQUEST_ID.obj
```

### 3. Testing & Validation
Automated testing of node graphs:
```bash
# Regression testing
for test in tests/*.nfg; do
    ./nodo_cli "$test" "/tmp/test.obj" || exit 1
done
echo "All tests passed!"
```

### 4. Render Farms
Distribute processing across multiple machines:
```bash
# Render farm node
while true; do
    job=$(get_next_job)
    ./nodo_cli "$job.nfg" "$job.obj" --stats
    upload_result "$job.obj"
done
```

## Exit Codes

| Code | Meaning |
|------|---------|
| `0` | Success - mesh exported |
| `1` | Error - see stderr for details |

## Error Messages

### Common Errors

**File not found:**
```
Error: Input file not found: 'missing.nfg'
```
→ Check file path is correct

**Graph load failure:**
```
Error: Failed to load graph from 'corrupt.nfg'
```
→ Verify .nfg file is valid JSON

**Execution failure:**
```
Error: Graph execution failed
```
→ Check node parameters and connections

**Export failure:**
```
Error: Failed to export to 'output.obj'
```
→ Verify write permissions and disk space

## Performance Tips

1. **Use stats to monitor performance:**
   ```bash
   nodo_cli graph.nfg output.obj --stats
   ```

2. **Batch similar operations together:**
   ```bash
   # More efficient than running separately
   ./batch_process.sh scenes/*.nfg
   ```

3. **Profile large graphs:**
   ```bash
   time nodo_cli large_graph.nfg output.obj -v
   ```

## Limitations

- **Output Format:** Currently only exports to .obj (Wavefront)
  - Future: STL, FBX, GLTF support planned

- **Display Node:** Exports the display node or last executed node
  - Future: Custom node selection planned

- **Parameters:** Uses graph defaults (no CLI override yet)
  - Future: Parameter overrides via command-line

## Troubleshooting

### Build Issues
If CLI doesn't build:
```bash
# Ensure CLI option is enabled
cmake --preset=conan-debug -DNODO_BUILD_CLI=ON
cmake --build --preset=conan-debug --target nodo_cli
```

### Runtime Issues
If execution fails:
```bash
# Use verbose mode for debugging
nodo_cli graph.nfg output.obj --verbose
```

### Missing Dependencies
If libraries not found:
```bash
# Reinstall Conan dependencies
conan install . --build=missing
cmake --preset=conan-debug
```

## Feedback & Issues

Found a bug or have a feature request?
- Open an issue on GitHub
- Include the error message and graph file (if possible)
- Specify your OS and Nodo version

## See Also

- [M2.2 Implementation Summary](docs/M2.2_HEADLESS_EXECUTION_SUMMARY.md) - Technical details
- [Host Interface System](docs/M2.1_HOST_INTERFACE_SUMMARY.md) - Architecture
- [ROADMAP.md](ROADMAP.md) - Future enhancements
