/**
 * @file benchmark_attributes.cpp
 * @brief Performance benchmarks for the attribute system
 *
 * This benchmark suite measures:
 * - Attribute creation and deletion
 * - Sequential and random access patterns
 * - Iteration methods (indexed, span, range-for)
 * - Memory usage and cache efficiency
 * - Comparison with old variant-based system (if available)
 *
 * Results are output to CSV for tracking over time.
 */

#include "nodeflux/core/geometry_container.hpp"
#include "nodeflux/core/standard_attributes.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

using namespace nodeflux::core;
namespace attrs = nodeflux::core::standard_attrs;

// ============================================================================
// Benchmark Utilities
// ============================================================================

class Timer {
public:
  void start() { start_ = std::chrono::high_resolution_clock::now(); }

  double elapsed_ms() const {
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start_).count();
  }

  double elapsed_us() const {
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::micro>(end - start_).count();
  }

private:
  std::chrono::high_resolution_clock::time_point start_;
};

struct BenchmarkResult {
  std::string name;
  size_t element_count;
  double time_ms;
  double throughput_million_ops_per_sec;
  size_t memory_bytes;

  void print() const {
    std::cout << std::left << std::setw(50) << name << std::right
              << std::setw(10) << element_count << " elements, " << std::setw(8)
              << std::fixed << std::setprecision(2) << time_ms << " ms, "
              << std::setw(8) << throughput_million_ops_per_sec << " Mops/s";

    if (memory_bytes > 0) {
      std::cout << ", " << std::setw(8) << (memory_bytes / 1024 / 1024)
                << " MB";
    }
    std::cout << "\n";
  }
};

std::vector<BenchmarkResult> g_results;

void record_result(const std::string &name, size_t count, double time_ms,
                   size_t memory_bytes = 0) {
  double throughput = (count / 1000000.0) / (time_ms / 1000.0);
  g_results.push_back({name, count, time_ms, throughput, memory_bytes});
  g_results.back().print();
}

// ============================================================================
// Benchmark 1: Attribute Creation
// ============================================================================

void benchmark_create_float_attribute() {
  std::cout << "\n=== Benchmark: Create Float Attribute ===\n";

  for (size_t count : {1000, 10000, 100000, 1000000}) {
    GeometryContainer geo;
    geo.set_point_count(count);

    Timer timer;
    timer.start();

    geo.add_point_attribute(attrs::P, AttributeType::FLOAT);

    double time = timer.elapsed_ms();
    size_t memory = count * sizeof(float);
    record_result("CreateFloatAttribute", count, time, memory);
  }
}

void benchmark_create_vector3_attribute() {
  std::cout << "\n=== Benchmark: Create Vector3 Attribute ===\n";

  for (size_t count : {1000, 10000, 100000, 1000000}) {
    GeometryContainer geo;
    geo.set_point_count(count);

    Timer timer;
    timer.start();

    geo.add_point_attribute(attrs::P, AttributeType::VEC3F);

    double time = timer.elapsed_ms();
    size_t memory = count * sizeof(Vec3f);
    record_result("CreateVector3Attribute", count, time, memory);
  }
}

void benchmark_create_multiple_attributes() {
  std::cout << "\n=== Benchmark: Create Multiple Attributes ===\n";

  for (size_t count : {1000, 10000, 100000, 1000000}) {
    GeometryContainer geo;
    geo.set_point_count(count);

    Timer timer;
    timer.start();

    geo.add_point_attribute(attrs::P, AttributeType::VEC3F);
    geo.add_point_attribute(attrs::N, AttributeType::VEC3F);
    geo.add_point_attribute(attrs::Cd, AttributeType::VEC3F);
    geo.add_point_attribute("pscale", AttributeType::FLOAT);

    double time = timer.elapsed_ms();
    size_t memory = count * (3 * sizeof(Vec3f) + sizeof(float));
    record_result("CreateMultipleAttributes", count, time, memory);
  }
}

// ============================================================================
// Benchmark 2: Sequential Access
// ============================================================================

void benchmark_sequential_write() {
  std::cout << "\n=== Benchmark: Sequential Write ===\n";

  for (size_t count : {1000, 10000, 100000, 1000000, 10000000}) {
    GeometryContainer geo;
    geo.set_point_count(count);
    geo.add_point_attribute(attrs::P, AttributeType::VEC3F);

    auto *positions = geo.positions();

    Timer timer;
    timer.start();

    for (size_t i = 0; i < count; ++i) {
      (*positions)[i] = Vec3f(static_cast<float>(i), static_cast<float>(i * 2),
                              static_cast<float>(i * 3));
    }

    double time = timer.elapsed_ms();
    record_result("SequentialWrite_Vec3f", count, time);
  }
}

void benchmark_sequential_read() {
  std::cout << "\n=== Benchmark: Sequential Read ===\n";

  for (size_t count : {1000, 10000, 100000, 1000000, 10000000}) {
    GeometryContainer geo;
    geo.set_point_count(count);
    geo.add_point_attribute(attrs::P, AttributeType::VEC3F);

    auto *positions = geo.positions();
    for (size_t i = 0; i < count; ++i) {
      (*positions)[i] = Vec3f(static_cast<float>(i), static_cast<float>(i),
                              static_cast<float>(i));
    }

    Timer timer;
    timer.start();

    Vec3f sum(0.0F, 0.0F, 0.0F);
    for (size_t i = 0; i < count; ++i) {
      sum += (*positions)[i];
    }

    double time = timer.elapsed_ms();
    record_result("SequentialRead_Vec3f", count, time);

    // Prevent optimization
    if (sum.x() < 0) {
      std::cout << "sum: " << sum.x() << "\n";
    }
  }
}

// ============================================================================
// Benchmark 3: Random Access
// ============================================================================

void benchmark_random_write() {
  std::cout << "\n=== Benchmark: Random Write ===\n";

  std::mt19937 rng(42);

  for (size_t count : {1000, 10000, 100000, 1000000}) {
    GeometryContainer geo;
    geo.set_point_count(count);
    geo.add_point_attribute(attrs::P, AttributeType::VEC3F);

    auto *positions = geo.positions();

    // Generate random indices
    std::vector<size_t> indices(count);
    for (size_t i = 0; i < count; ++i) {
      indices[i] = i;
    }
    std::shuffle(indices.begin(), indices.end(), rng);

    Timer timer;
    timer.start();

    for (size_t i = 0; i < count; ++i) {
      size_t idx = indices[i];
      (*positions)[idx] =
          Vec3f(static_cast<float>(i), static_cast<float>(i * 2),
                static_cast<float>(i * 3));
    }

    double time = timer.elapsed_ms();
    record_result("RandomWrite_Vec3f", count, time);
  }
}

void benchmark_random_read() {
  std::cout << "\n=== Benchmark: Random Read ===\n";

  std::mt19937 rng(42);

  for (size_t count : {1000, 10000, 100000, 1000000}) {
    GeometryContainer geo;
    geo.set_point_count(count);
    geo.add_point_attribute(attrs::P, AttributeType::VEC3F);

    auto *positions = geo.positions();
    for (size_t i = 0; i < count; ++i) {
      (*positions)[i] = Vec3f(static_cast<float>(i), static_cast<float>(i),
                              static_cast<float>(i));
    }

    // Generate random indices
    std::vector<size_t> indices(count);
    for (size_t i = 0; i < count; ++i) {
      indices[i] = i;
    }
    std::shuffle(indices.begin(), indices.end(), rng);

    Timer timer;
    timer.start();

    Vec3f sum(0.0F, 0.0F, 0.0F);
    for (size_t i = 0; i < count; ++i) {
      sum += (*positions)[indices[i]];
    }

    double time = timer.elapsed_ms();
    record_result("RandomRead_Vec3f", count, time);

    // Prevent optimization
    if (sum.x() < 0) {
      std::cout << "sum: " << sum.x() << "\n";
    }
  }
}

// ============================================================================
// Benchmark 4: Iteration Methods
// ============================================================================

void benchmark_indexed_iteration() {
  std::cout << "\n=== Benchmark: Indexed Iteration ===\n";

  for (size_t count : {1000, 10000, 100000, 1000000, 10000000}) {
    GeometryContainer geo;
    geo.set_point_count(count);
    geo.add_point_attribute(attrs::P, AttributeType::VEC3F);

    auto *positions = geo.positions();

    Timer timer;
    timer.start();

    for (size_t i = 0; i < positions->size(); ++i) {
      (*positions)[i] = Vec3f(static_cast<float>(i), 0.0F, 0.0F);
    }

    double time = timer.elapsed_ms();
    record_result("IndexedIteration", count, time);
  }
}

void benchmark_span_iteration() {
  std::cout << "\n=== Benchmark: Span Iteration ===\n";

  for (size_t count : {1000, 10000, 100000, 1000000, 10000000}) {
    GeometryContainer geo;
    geo.set_point_count(count);
    geo.add_point_attribute(attrs::P, AttributeType::VEC3F);

    auto *positions = geo.positions();

    Timer timer;
    timer.start();

    auto span = positions->values_writable();
    float val = 0.0F;
    for (auto &pos : span) {
      pos = Vec3f(val, 0.0F, 0.0F);
      val += 1.0F;
    }

    double time = timer.elapsed_ms();
    record_result("SpanIteration", count, time);
  }
}

void benchmark_raw_pointer_iteration() {
  std::cout << "\n=== Benchmark: Raw Pointer Iteration ===\n";

  for (size_t count : {1000, 10000, 100000, 1000000, 10000000}) {
    GeometryContainer geo;
    geo.set_point_count(count);
    geo.add_point_attribute(attrs::P, AttributeType::VEC3F);

    auto *positions = geo.positions();

    Timer timer;
    timer.start();

    Vec3f *data = positions->get_vector_writable().data();
    for (size_t i = 0; i < count; ++i) {
      data[i] = Vec3f(static_cast<float>(i), 0.0F, 0.0F);
    }

    double time = timer.elapsed_ms();
    record_result("RawPointerIteration", count, time);
  }
}

// ============================================================================
// Benchmark 5: Memory Operations
// ============================================================================

void benchmark_attribute_resize() {
  std::cout << "\n=== Benchmark: Attribute Resize ===\n";

  for (size_t count : {1000, 10000, 100000, 1000000}) {
    GeometryContainer geo;
    geo.set_point_count(10); // Start small
    geo.add_point_attribute(attrs::P, AttributeType::VEC3F);

    Timer timer;
    timer.start();

    geo.set_point_count(count);

    double time = timer.elapsed_ms();
    record_result("AttributeResize", count, time);
  }
}

void benchmark_attribute_clone() {
  std::cout << "\n=== Benchmark: Attribute Clone ===\n";

  for (size_t count : {1000, 10000, 100000, 1000000}) {
    GeometryContainer geo;
    geo.set_point_count(count);
    geo.add_point_attribute(attrs::P, AttributeType::VEC3F);
    geo.add_point_attribute(attrs::N, AttributeType::VEC3F);
    geo.add_point_attribute(attrs::Cd, AttributeType::VEC3F);

    auto *positions = geo.positions();
    for (size_t i = 0; i < count; ++i) {
      (*positions)[i] = Vec3f(static_cast<float>(i), 0.0F, 0.0F);
    }

    Timer timer;
    timer.start();

    GeometryContainer clone = geo.clone();

    double time = timer.elapsed_ms();
    size_t memory = count * 3 * sizeof(Vec3f);
    record_result("AttributeClone", count, time, memory);
  }
}

// ============================================================================
// Benchmark 6: Complex Operations
// ============================================================================

void benchmark_transform_positions() {
  std::cout << "\n=== Benchmark: Transform Positions (Scale) ===\n";

  for (size_t count : {1000, 10000, 100000, 1000000, 10000000}) {
    GeometryContainer geo;
    geo.set_point_count(count);
    geo.add_point_attribute(attrs::P, AttributeType::VEC3F);

    auto *positions = geo.positions();
    for (size_t i = 0; i < count; ++i) {
      (*positions)[i] = Vec3f(static_cast<float>(i), static_cast<float>(i),
                              static_cast<float>(i));
    }

    Vec3f scale(2.0F, 2.0F, 2.0F);

    Timer timer;
    timer.start();

    for (size_t i = 0; i < positions->size(); ++i) {
      (*positions)[i] = (*positions)[i].cwiseProduct(scale);
    }

    double time = timer.elapsed_ms();
    record_result("TransformScale", count, time);
  }
}

void benchmark_compute_normals() {
  std::cout << "\n=== Benchmark: Compute Centroid ===\n";

  for (size_t count : {1000, 10000, 100000, 1000000, 10000000}) {
    GeometryContainer geo;
    geo.set_point_count(count);
    geo.add_point_attribute(attrs::P, AttributeType::VEC3F);

    auto *positions = geo.positions();
    for (size_t i = 0; i < count; ++i) {
      (*positions)[i] = Vec3f(static_cast<float>(i), static_cast<float>(i),
                              static_cast<float>(i));
    }

    Timer timer;
    timer.start();

    Vec3f centroid(0.0F, 0.0F, 0.0F);
    for (size_t i = 0; i < positions->size(); ++i) {
      centroid += (*positions)[i];
    }
    centroid /= static_cast<float>(positions->size());

    double time = timer.elapsed_ms();
    record_result("ComputeCentroid", count, time);

    // Prevent optimization
    if (centroid.x() < 0) {
      std::cout << "centroid: " << centroid.x() << "\n";
    }
  }
}

void benchmark_attribute_blending() {
  std::cout << "\n=== Benchmark: Attribute Blending (Lerp) ===\n";

  for (size_t count : {1000, 10000, 100000, 1000000}) {
    GeometryContainer geo;
    geo.set_point_count(count);
    geo.add_point_attribute("attr_a", AttributeType::VEC3F);
    geo.add_point_attribute("attr_b", AttributeType::VEC3F);
    geo.add_point_attribute("result", AttributeType::VEC3F);

    auto *attr_a = geo.get_point_attribute_typed<Vec3f>("attr_a");
    auto *attr_b = geo.get_point_attribute_typed<Vec3f>("attr_b");
    auto *result = geo.get_point_attribute_typed<Vec3f>("result");

    for (size_t i = 0; i < count; ++i) {
      (*attr_a)[i] = Vec3f(0.0F, 0.0F, 0.0F);
      (*attr_b)[i] = Vec3f(1.0F, 1.0F, 1.0F);
    }

    float t = 0.5F;

    Timer timer;
    timer.start();

    for (size_t i = 0; i < count; ++i) {
      (*result)[i] = (*attr_a)[i] * (1.0F - t) + (*attr_b)[i] * t;
    }

    double time = timer.elapsed_ms();
    record_result("AttributeBlending", count, time);
  }
}

// ============================================================================
// Benchmark 7: Memory Usage
// ============================================================================

void benchmark_memory_usage() {
  std::cout << "\n=== Benchmark: Memory Usage ===\n";

  struct Config {
    std::string name;
    size_t count;
    std::vector<std::pair<std::string, AttributeType>> attributes;
  };

  std::vector<Config> configs = {
      {"1M_Points_PositionOnly",
       1000000,
       {{std::string(attrs::P), AttributeType::VEC3F}}},
      {"1M_Points_PosNorm",
       1000000,
       {{std::string(attrs::P), AttributeType::VEC3F},
        {std::string(attrs::N), AttributeType::VEC3F}}},
      {"1M_Points_PosNormCol",
       1000000,
       {{std::string(attrs::P), AttributeType::VEC3F},
        {std::string(attrs::N), AttributeType::VEC3F},
        {std::string(attrs::Cd), AttributeType::VEC3F}}},
      {"1M_Points_Full",
       1000000,
       {{std::string(attrs::P), AttributeType::VEC3F},
        {std::string(attrs::N), AttributeType::VEC3F},
        {std::string(attrs::Cd), AttributeType::VEC3F},
        {std::string(attrs::uv), AttributeType::VEC2F},
        {"pscale", AttributeType::FLOAT},
        {"id", AttributeType::INT}}},
  };

  for (const auto &config : configs) {
    GeometryContainer geo;
    geo.set_point_count(config.count);

    size_t expected_memory = 0;

    for (const auto &[name, type] : config.attributes) {
      geo.add_point_attribute(name, type);

      switch (type) {
      case AttributeType::FLOAT:
      case AttributeType::INT:
        expected_memory += config.count * 4;
        break;
      case AttributeType::VEC2F:
        expected_memory += config.count * 8;
        break;
      case AttributeType::VEC3F:
        expected_memory += config.count * 12;
        break;
      case AttributeType::VEC4F:
        expected_memory += config.count * 16;
        break;
      default:
        break;
      }
    }

    std::cout << std::left << std::setw(50) << config.name << std::right
              << std::setw(10) << config.count << " elements, " << std::setw(8)
              << (expected_memory / 1024 / 1024) << " MB"
              << " (estimated)\n";
  }
}

// ============================================================================
// Export Results to CSV
// ============================================================================

void export_results_to_csv(const std::string &filename) {
  std::ofstream csv(filename);

  csv << "Benchmark,ElementCount,TimeMS,ThroughputMopsPerSec,MemoryBytes\n";

  for (const auto &result : g_results) {
    csv << result.name << "," << result.element_count << "," << std::fixed
        << std::setprecision(3) << result.time_ms << ","
        << result.throughput_million_ops_per_sec << "," << result.memory_bytes
        << "\n";
  }

  csv.close();
  std::cout << "\n✅ Results exported to: " << filename << "\n";
}

// ============================================================================
// Main
// ============================================================================

int main() {
  std::cout << "╔════════════════════════════════════════════════════════╗\n";
  std::cout << "║      Attribute System Performance Benchmarks          ║\n";
  std::cout << "╚════════════════════════════════════════════════════════╝\n";

  // Run all benchmarks
  benchmark_create_float_attribute();
  benchmark_create_vector3_attribute();
  benchmark_create_multiple_attributes();

  benchmark_sequential_write();
  benchmark_sequential_read();

  benchmark_random_write();
  benchmark_random_read();

  benchmark_indexed_iteration();
  benchmark_span_iteration();
  benchmark_raw_pointer_iteration();

  benchmark_attribute_resize();
  benchmark_attribute_clone();

  benchmark_transform_positions();
  benchmark_compute_normals();
  benchmark_attribute_blending();

  benchmark_memory_usage();

  // Export to CSV
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << "benchmark_results_"
     << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".csv";

  export_results_to_csv(ss.str());

  std::cout << "\n╔════════════════════════════════════════════════════════╗\n";
  std::cout << "║            Benchmarks Complete!                        ║\n";
  std::cout << "╚════════════════════════════════════════════════════════╝\n";

  return 0;
}
