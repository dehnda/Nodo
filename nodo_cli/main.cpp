/**
 * Nodo CLI - Headless execution tool
 *
 * Command-line interface for executing Nodo node graphs without GUI.
 * Useful for:
 * - Batch processing
 * - CI/CD pipelines
 * - Render farms
 * - Server-side processing
 * - Automated testing
 *
 * Usage:
 *   nodo_cli input.nfg output.obj [--verbose] [--stats]
 *
 * M2.2: Headless Execution
 */

#include "nodo/core/IHostInterface.h"
#include "nodo/graph/execution_engine.hpp"
#include "nodo/graph/graph_serializer.hpp"
#include "nodo/graph/node_graph.hpp"
#include "nodo/io/obj_exporter.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

/**
 * CLI-specific host interface with progress bars and statistics
 */
class CLIHostInterface : public nodo::IHostInterface {
public:
  explicit CLIHostInterface(bool verbose) : verbose_(verbose) {}

  bool report_progress(int current, int total, const std::string& message) override {
    if (verbose_) {
      // Calculate percentage
      float percent = (static_cast<float>(current) / static_cast<float>(total)) * 100.0f;

      // Print progress bar
      std::cout << "\r[";
      int bar_width = 50;
      int filled = static_cast<int>(percent / 100.0f * bar_width);

      for (int i = 0; i < bar_width; ++i) {
        if (i < filled) {
          std::cout << "=";
        } else if (i == filled) {
          std::cout << ">";
        } else {
          std::cout << " ";
        }
      }

      std::cout << "] " << static_cast<int>(percent) << "% - " << message << std::flush;
    }

    return true; // Never cancel
  }

  void log(const std::string& level, const std::string& message) override {
    if (verbose_ || level == "error" || level == "warning") {
      std::cout << "\n[" << level << "] " << message;
    }
  }

  std::string get_host_info() const override { return "Nodo CLI v1.0"; }

private:
  bool verbose_;
};

/**
 * Print usage information
 */
void print_usage(const char* program_name) {
  std::cout << "Nodo CLI - Headless Node Graph Execution\n\n";
  std::cout << "Usage:\n";
  std::cout << "  " << program_name << " <input.nfg> <output.obj> [options]\n\n";
  std::cout << "Arguments:\n";
  std::cout << "  <input.nfg>    Input node graph file (.nfg format)\n";
  std::cout << "  <output.obj>   Output mesh file (.obj format)\n\n";
  std::cout << "Options:\n";
  std::cout << "  --verbose, -v  Show detailed progress and statistics\n";
  std::cout << "  --stats, -s    Show execution statistics\n";
  std::cout << "  --help, -h     Show this help message\n\n";
  std::cout << "Examples:\n";
  std::cout << "  " << program_name << " scene.nfg output.obj\n";
  std::cout << "  " << program_name << " scene.nfg output.obj --verbose --stats\n";
}

/**
 * Main CLI entry point
 */
int main(int argc, char* argv[]) {
  // Parse command line arguments
  if (argc < 3) {
    print_usage(argv[0]);
    return 1;
  }

  std::string input_file;
  std::string output_file;
  bool verbose = false;
  bool show_stats = false;

  // Parse arguments
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      return 0;
    } else if (arg == "--verbose" || arg == "-v") {
      verbose = true;
    } else if (arg == "--stats" || arg == "-s") {
      show_stats = true;
    } else if (input_file.empty()) {
      input_file = arg;
    } else if (output_file.empty()) {
      output_file = arg;
    } else {
      std::cerr << "Error: Unknown argument '" << arg << "'\n";
      return 1;
    }
  }

  // Validate arguments
  if (input_file.empty() || output_file.empty()) {
    std::cerr << "Error: Both input and output files are required\n";
    print_usage(argv[0]);
    return 1;
  }

  // Check input file exists
  if (!fs::exists(input_file)) {
    std::cerr << "Error: Input file '" << input_file << "' not found\n";
    return 1;
  }

  // Start execution
  std::cout << "Nodo CLI - Headless Execution\n";
  std::cout << "==============================\n\n";
  std::cout << "Input:  " << input_file << "\n";
  std::cout << "Output: " << output_file << "\n";
  std::cout << "Mode:   " << (verbose ? "Verbose" : "Quiet") << "\n\n";

  auto start_time = std::chrono::high_resolution_clock::now();

  // Load node graph
  if (verbose) {
    std::cout << "Loading graph...\n";
  }

  auto graph_opt = nodo::graph::GraphSerializer::load_from_file(input_file);

  if (!graph_opt) {
    std::cerr << "Error: Failed to load graph from '" << input_file << "'\n";
    return 1;
  }

  nodo::graph::NodeGraph& graph = *graph_opt;

  if (verbose) {
    std::cout << "Loaded " << graph.get_nodes().size() << " nodes\n\n";
  }

  // Execute graph
  if (verbose) {
    std::cout << "Executing graph...\n";
  }

  nodo::graph::ExecutionEngine engine;
  CLIHostInterface host(verbose);
  engine.set_host_interface(&host);

  bool success = engine.execute_graph(graph);

  if (verbose) {
    std::cout << "\n"; // New line after progress bar
  }

  if (!success) {
    std::cerr << "Error: Graph execution failed\n";
    return 1;
  }

  if (verbose) {
    std::cout << "Execution complete\n\n";
  }

  // Export result
  if (verbose) {
    std::cout << "Exporting to OBJ...\n";
  }

  // Get display node or last executed node
  int display_node = graph.get_display_node();
  if (display_node < 0) {
    // No display node, use last node
    auto execution_order = graph.get_execution_order();
    if (execution_order.empty()) {
      std::cerr << "Error: No nodes to export\n";
      return 1;
    }
    display_node = execution_order.back();
  }

  auto geometry = engine.get_node_geometry(display_node);
  if (!geometry) {
    std::cerr << "Error: No geometry to export\n";
    return 1;
  }

  if (!nodo::io::ObjExporter::export_geometry(*geometry, output_file)) {
    std::cerr << "Error: Failed to export to '" << output_file << "'\n";
    return 1;
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Show statistics
  if (show_stats) {
    std::cout << "\nStatistics:\n";
    std::cout << "-----------\n";
    std::cout << "Nodes:        " << graph.get_nodes().size() << "\n";
    std::cout << "Points:       " << geometry->topology().point_count() << "\n";
    std::cout << "Primitives:   " << geometry->topology().primitive_count() << "\n";
    std::cout << "Execution:    " << duration.count() << " ms\n";
    std::cout << "Output size:  " << fs::file_size(output_file) << " bytes\n";
  }

  std::cout << "\n✓ Successfully exported to: " << output_file << "\n";

  if (!verbose && !show_stats) {
    std::cout << "  (Use --verbose or --stats for more information)\n";
  }

  return 0;
}
