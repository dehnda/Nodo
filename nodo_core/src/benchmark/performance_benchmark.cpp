/**
 * Nodo Performance Benchmark
 * Tests execution engine performance with various graph configurations
 */

#include "nodo/graph/execution_engine.hpp"
#include "nodo/graph/graph_serializer.hpp"
#include "nodo/graph/node_graph.hpp"
#include "nodo/performance/profiler.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace nodo;
using namespace nodo::graph;
using namespace nodo::performance;

// Benchmark configuration
struct BenchmarkConfig {
  std::string name;
  std::string graph_file;
  int iterations = 10;
  bool clear_cache_between_runs = false;
};

struct BenchmarkResult {
  std::string name;
  int iterations;
  double avg_time_ms;
  double min_time_ms;
  double max_time_ms;
  double std_dev_ms;
  size_t node_count;
  size_t point_count;
  size_t prim_count;
  bool used_cache;
};

class PerformanceBenchmark {
public:
  void run_benchmark(const BenchmarkConfig& config) {
    std::cout
        << "\n╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Running Benchmark: " << std::left << std::setw(32)
              << config.name << " ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n";

    // Load graph
    auto graph_opt = GraphSerializer::load_from_file(config.graph_file);
    if (!graph_opt) {
      std::cerr << "❌ Failed to load graph: " << config.graph_file << "\n";
      return;
    }

    NodeGraph& graph = *graph_opt;
    ExecutionEngine engine;

    std::vector<double> execution_times;
    execution_times.reserve(config.iterations);

    // Warmup run
    std::cout << "Warming up... ";
    engine.execute_graph(graph);
    std::cout << "done\n";

    // Benchmark runs
    std::cout << "Running " << config.iterations << " iterations...\n";

    for (int i = 0; i < config.iterations; ++i) {
      if (config.clear_cache_between_runs) {
        engine.clear_cache();
      }

      auto start = std::chrono::high_resolution_clock::now();
      bool success = engine.execute_graph(graph);
      auto end = std::chrono::high_resolution_clock::now();

      if (!success) {
        std::cerr << "❌ Execution failed at iteration " << i << "\n";
        continue;
      }

      auto duration =
          std::chrono::duration_cast<std::chrono::microseconds>(end - start);
      double time_ms = duration.count() / 1000.0;
      execution_times.push_back(time_ms);

      std::cout << "  Iteration " << (i + 1) << "/" << config.iterations << ": "
                << time_ms << " ms\n";
    }

    // Calculate statistics
    if (execution_times.empty()) {
      std::cerr << "❌ No successful executions\n";
      return;
    }

    double sum = 0.0;
    double min_time = execution_times[0];
    double max_time = execution_times[0];

    for (double time : execution_times) {
      sum += time;
      if (time < min_time)
        min_time = time;
      if (time > max_time)
        max_time = time;
    }

    double avg_time = sum / execution_times.size();

    // Calculate standard deviation
    double variance = 0.0;
    for (double time : execution_times) {
      double diff = time - avg_time;
      variance += diff * diff;
    }
    double std_dev = std::sqrt(variance / execution_times.size());

    // Get geometry stats from display node
    int display_node = graph.get_display_node();
    if (display_node < 0) {
      auto exec_order = graph.get_execution_order();
      if (!exec_order.empty()) {
        display_node = exec_order.back();
      }
    }

    size_t point_count = 0;
    size_t prim_count = 0;

    if (display_node >= 0) {
      auto geometry = engine.get_node_geometry(display_node);
      if (geometry) {
        point_count = geometry->topology().point_count();
        prim_count = geometry->topology().primitive_count();
      }
    }

    // Print results
    std::cout
        << "\n╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Results: " << std::left << std::setw(44) << config.name
              << " ║\n";
    std::cout << "╠════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Average:  " << std::setw(8) << avg_time << " ms"
              << std::string(29, ' ') << "║\n";
    std::cout << "║  Min:      " << std::setw(8) << min_time << " ms"
              << std::string(29, ' ') << "║\n";
    std::cout << "║  Max:      " << std::setw(8) << max_time << " ms"
              << std::string(29, ' ') << "║\n";
    std::cout << "║  Std Dev:  " << std::setw(8) << std_dev << " ms"
              << std::string(29, ' ') << "║\n";
    std::cout << "║  " << std::string(52, '-') << "║\n";
    std::cout << "║  Nodes:        " << std::setw(8) << graph.get_nodes().size()
              << std::string(29, ' ') << "║\n";
    std::cout << "║  Points:       " << std::setw(8) << point_count
              << std::string(29, ' ') << "║\n";
    std::cout << "║  Primitives:   " << std::setw(8) << prim_count
              << std::string(29, ' ') << "║\n";
    std::cout << "║  Cache Used:   " << std::setw(8)
              << (!config.clear_cache_between_runs ? "Yes" : "No")
              << std::string(29, ' ') << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n";

    // Print per-node cook times
    std::cout << "\nPer-Node Cook Times:\n";
    std::cout << "────────────────────\n";
    for (const auto& node_ptr : graph.get_nodes()) {
      std::cout << "  Node " << node_ptr->get_id() << " ("
                << node_ptr->get_name() << "): " << node_ptr->get_cook_time()
                << " ms\n";
    }
  }

  void run_cache_comparison(const std::string& graph_file) {
    std::cout
        << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║           CACHE PERFORMANCE COMPARISON                ║\n";
    std::cout
        << "╚═══════════════════════════════════════════════════════╝\n\n";

    // Test with cache enabled
    std::cout << "Testing WITH cache...\n";
    BenchmarkConfig with_cache;
    with_cache.name = "With Cache";
    with_cache.graph_file = graph_file;
    with_cache.iterations = 10;
    with_cache.clear_cache_between_runs = false;
    run_benchmark(with_cache);

    // Test without cache
    std::cout << "\nTesting WITHOUT cache...\n";
    BenchmarkConfig without_cache;
    without_cache.name = "Without Cache";
    without_cache.graph_file = graph_file;
    without_cache.iterations = 10;
    without_cache.clear_cache_between_runs = true;
    run_benchmark(without_cache);
  }
};

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "Nodo Performance Benchmark\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << argv[0] << " <graph.nfg> [--cache-compare]\n\n";
    std::cout << "Options:\n";
    std::cout
        << "  --cache-compare  Compare performance with/without caching\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << argv[0] << " projects/Simple_A.nfg\n";
    std::cout << "  " << argv[0]
              << " projects/copy_to_points.nfg --cache-compare\n";
    return 1;
  }

  std::string graph_file = argv[1];
  bool cache_compare = (argc > 2 && std::string(argv[2]) == "--cache-compare");

  PerformanceBenchmark benchmark;

  if (cache_compare) {
    benchmark.run_cache_comparison(graph_file);
  } else {
    BenchmarkConfig config;
    config.name = "Standard Benchmark";
    config.graph_file = graph_file;
    config.iterations = 10;
    config.clear_cache_between_runs = false;
    benchmark.run_benchmark(config);
  }

  return 0;
}
