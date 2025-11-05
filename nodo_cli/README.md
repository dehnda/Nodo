# Nodo CLI - Headless Execution Tool

Command-line interface for batch processing and automation of Nodo node graphs.

## Quick Start

### Build
```bash
cmake --preset=conan-debug
cmake --build --preset=conan-debug --target nodo_cli
```

### Run
```bash
./build/Debug/nodo_cli/nodo_cli input.nfg output.obj
```

## Features

- **Headless Execution** - No Qt or GUI dependencies
- **Progress Reporting** - Terminal progress bars
- **Statistics** - Execution time, geometry counts, file size
- **Verbose Mode** - Detailed logging for debugging
- **Error Handling** - Clear error messages

## Usage

```bash
nodo_cli <input.nfg> <output.obj> [options]

Options:
  --verbose, -v  Show detailed progress
  --stats, -s    Display execution statistics
  --help, -h     Show help message
```

## Examples

### Basic Export
```bash
./nodo_cli scene.nfg output.obj
```

### With Statistics
```bash
./nodo_cli scene.nfg output.obj --stats
```

### Verbose Mode
```bash
./nodo_cli scene.nfg output.obj --verbose --stats
```

## Architecture

```
nodo_cli
├── main.cpp               (CLI implementation)
├── CMakeLists.txt         (Build configuration)
└── README.md              (this file)

Dependencies:
├── nodo_core              (Core engine - ONLY dependency)
└── C++ Standard Library   (C++20)
```

### CLIHostInterface

Custom implementation of `IHostInterface` for terminal output:

```cpp
class CLIHostInterface : public nodo::IHostInterface {
    // Progress bar: [===>      ] 50% - Processing...
    bool report_progress(int current, int total, const std::string& message) override;

    // Console logging with timestamps
    void log(const std::string& level, const std::string& message) override;

    // Returns "Nodo CLI v1.0"
    std::string get_host_info() const override;
};
```

## Documentation

- **User Guide:** [docs/CLI_USER_GUIDE.md](../docs/CLI_USER_GUIDE.md)
- **Implementation:** [docs/M2.2_HEADLESS_EXECUTION_SUMMARY.md](../docs/M2.2_HEADLESS_EXECUTION_SUMMARY.md)
- **Completion Report:** [docs/M2.2_COMPLETION_REPORT.md](../docs/M2.2_COMPLETION_REPORT.md)

## Testing

Run the automated test suite:

```bash
./scripts/test_cli.sh
```

Manual testing:

```bash
# Test with example projects
./nodo_cli projects/Simple_A.nfg /tmp/test.obj --verbose --stats
./nodo_cli projects/copy_to_points.nfg /tmp/test2.obj --stats
```

## Performance

Typical execution times:
- Small graphs (1-2 nodes): 3-5ms
- Medium graphs (4 nodes): 5-6ms
- Large graphs: TBD (M2.3 optimization)

## Automation

### Bash Script
```bash
for graph in scenes/*.nfg; do
    ./nodo_cli "$graph" "output/$(basename "$graph" .nfg).obj"
done
```

### Python
```python
import subprocess

subprocess.run([
    './nodo_cli', 'scene.nfg', 'output.obj', '--stats'
])
```

### Makefile
```makefile
%.obj: %.nfg
	./nodo_cli $< $@
```

## Exit Codes

- `0` - Success
- `1` - Error (see stderr)

## Limitations

- Output format: .obj only (STL, FBX, GLTF planned)
- Exports display node or last executed node
- No parameter overrides via CLI yet

See roadmap for planned enhancements (M2.2.1+).

## Troubleshooting

### Build Issues
```bash
# Ensure CLI is enabled
cmake -DNODO_BUILD_CLI=ON --preset=conan-debug
```

### Runtime Issues
```bash
# Use verbose mode
./nodo_cli graph.nfg output.obj --verbose
```

## Support

- GitHub Issues: Report bugs and request features
- Documentation: See `/docs` directory
- Examples: See `/projects` directory
