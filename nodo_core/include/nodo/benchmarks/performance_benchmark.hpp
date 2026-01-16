#pragma once

#include "../core/mesh.hpp"
#include "../spatial/bvh.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace nodo::benchmarks {

/// @brief Performance measurement utilities
class PerformanceBenchmark {
public:
  /// @brief Benchmark result for a single operation
  struct BenchmarkResult {
    std::string operation_name;
    double average_time_ms;
    double min_time_ms;
    double max_time_ms;
    double std_dev_ms;
    size_t iterations;
    size_t memory_usage_bytes;
    std::string additional_info;

    BenchmarkResult()
        : average_time_ms(0), min_time_ms(0), max_time_ms(0), std_dev_ms(0), iterations(0), memory_usage_bytes(0) {}
  };

  /// @brief Comprehensive benchmark suite results
  struct BenchmarkSuite {
    std::vector<BenchmarkResult> results;
    std::string test_configuration;
    std::chrono::system_clock::time_point timestamp;

    /// @brief Get result by operation name
    const BenchmarkResult* find_result(const std::string& name) const;

    /// @brief Generate summary report
    std::string generate_report() const;

    /// @brief Export results to CSV
    void export_csv(const std::string& filename) const;
  };

  /// @brief Mesh complexity levels for testing
  enum class ComplexityLevel {
    Simple,     // ~100 triangles
    Medium,     // ~1K triangles
    Complex,    // ~10K triangles
    VeryComplex // ~100K triangles
  };

  /// @brief Benchmark configuration
  struct BenchmarkConfig {
    size_t iterations;
    bool measure_memory;
    bool warm_up_runs;
    size_t warm_up_iterations;
    std::vector<ComplexityLevel> complexity_levels;

    BenchmarkConfig()
        : iterations(100),
          measure_memory(true),
          warm_up_runs(true),
          warm_up_iterations(10),
          complexity_levels{ComplexityLevel::Simple, ComplexityLevel::Medium, ComplexityLevel::Complex} {}
  };

public:
  /// @brief Constructor
  explicit PerformanceBenchmark(const BenchmarkConfig& config = BenchmarkConfig{});

  /// @brief Run comprehensive BVH performance benchmarks
  /// @return Complete benchmark results
  BenchmarkSuite run_bvh_benchmarks();

  /// @brief Run boolean operation benchmarks
  /// @return Boolean operation benchmark results
  BenchmarkSuite run_boolean_benchmarks();

  /// @brief Run mesh generation benchmarks
  /// @return Mesh generation benchmark results
  BenchmarkSuite run_generation_benchmarks();

  /// @brief Run spatial query benchmarks
  /// @return Spatial query benchmark results
  BenchmarkSuite run_spatial_query_benchmarks();

  /// @brief Compare BVH vs brute-force operations
  /// @return Comparison benchmark results
  BenchmarkSuite run_bvh_comparison_benchmarks();

  /// @brief Benchmark BVH build parameter optimization
  /// @return Parameter optimization results
  BenchmarkSuite run_parameter_optimization_benchmarks();

  /// @brief Create test mesh of specified complexity
  /// @param level Complexity level
  /// @return Generated test mesh
  static core::Mesh create_test_mesh(ComplexityLevel level);

  /// @brief Create test mesh with specific triangle count
  /// @param triangle_count Target number of triangles
  /// @return Generated test mesh
  static core::Mesh create_test_mesh(size_t triangle_count);

private:
  BenchmarkConfig config_;

  /// @brief Time a function execution
  /// @param func Function to benchmark
  /// @param iterations Number of iterations
  /// @return Timing measurements
  std::vector<double> time_function(std::function<void()> func, size_t iterations);

  /// @brief Calculate statistics from timing data
  /// @param timings Vector of timing measurements
  /// @return Statistical summary
  BenchmarkResult calculate_statistics(const std::vector<double>& timings, const std::string& operation_name);

  /// @brief Estimate memory usage of an object
  /// @param obj Object to measure
  /// @return Estimated memory usage in bytes
  template <typename T>
  size_t estimate_memory_usage(const T& obj);

  /// @brief Generate mesh with specified subdivision level
  /// @param base_mesh Base mesh to subdivide
  /// @param subdivision_levels Number of subdivision iterations
  /// @return Subdivided mesh
  static core::Mesh subdivide_mesh(const core::Mesh& base_mesh, int subdivision_levels);

  /// @brief Brute force ray intersection (for comparison)
  /// @param mesh Mesh to test
  /// @param ray Ray to intersect
  /// @return Intersection result
  static std::optional<spatial::BVH::RayHit> brute_force_ray_intersect(const core::Mesh& mesh,
                                                                       const spatial::BVH::Ray& ray);

  /// @brief Brute force AABB query (for comparison)
  /// @param mesh Mesh to query
  /// @param aabb Query bounding box
  /// @return Triangle indices
  static std::vector<int> brute_force_aabb_query(const core::Mesh& mesh, const spatial::AABB& aabb);
};

/// @brief Memory usage tracker for performance analysis
class MemoryTracker {
public:
  /// @brief Memory usage snapshot
  struct MemorySnapshot {
    size_t resident_memory_kb;
    size_t virtual_memory_kb;
    size_t peak_memory_kb;
    std::chrono::system_clock::time_point timestamp;
  };

  /// @brief Start memory tracking
  static void start_tracking();

  /// @brief Take memory snapshot
  /// @return Current memory usage
  static MemorySnapshot take_snapshot();

  /// @brief Stop tracking and get peak usage
  /// @return Peak memory usage during tracking period
  static MemorySnapshot stop_tracking();

private:
  static bool tracking_active_;
  static MemorySnapshot baseline_snapshot_;
  static MemorySnapshot peak_snapshot_;
};

} // namespace nodo::benchmarks
