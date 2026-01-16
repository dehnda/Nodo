/**
 * @file benchmark_geometry.cpp
 * @brief Benchmark for GeometryContainer operations
 */

#include "nodo/core/geometry_container.hpp"
#include "nodo/core/standard_attributes.hpp"
#include "nodo/geometry/boolean_ops.hpp"
#include "nodo/geometry/box_generator.hpp"
#include "nodo/geometry/sphere_generator.hpp"
#include "nodo/processing/pmp_converter.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

using namespace nodo;
using namespace std::chrono;

struct BenchmarkResult {
  std::string operation;
  size_t iterations;
  double min_ms;
  double max_ms;
  double avg_ms;
  double median_ms;
};

std::vector<double> time_function(std::function<void()> func, size_t iterations) {
  std::vector<double> timings;
  timings.reserve(iterations);

  for (size_t i = 0; i < iterations; ++i) {
    auto start = high_resolution_clock::now();
    func();
    auto end = high_resolution_clock::now();
    timings.push_back(duration<double, std::milli>(end - start).count());
  }

  return timings;
}

BenchmarkResult calculate_stats(const std::string& name, const std::vector<double>& timings) {
  BenchmarkResult result;
  result.operation = name;
  result.iterations = timings.size();

  auto sorted = timings;
  std::sort(sorted.begin(), sorted.end());

  result.min_ms = sorted.front();
  result.max_ms = sorted.back();
  result.avg_ms = std::accumulate(sorted.begin(), sorted.end(), 0.0) / sorted.size();
  result.median_ms = sorted[sorted.size() / 2];

  return result;
}

void print_result(const BenchmarkResult& result) {
  std::cout << std::left << std::setw(40) << result.operation << " | " << std::right << std::setw(6)
            << result.iterations << " | " << std::setw(10) << std::fixed << std::setprecision(3) << result.min_ms
            << " | " << std::setw(10) << result.max_ms << " | " << std::setw(10) << result.avg_ms << " | "
            << std::setw(10) << result.median_ms << "\n";
}

int main() {
  std::cout << "\n=== GeometryContainer Benchmarks ===\n\n";

  const size_t iterations = 100;

  // Header
  std::cout << std::left << std::setw(40) << "Operation"
            << " | " << std::right << std::setw(6) << "Iters"
            << " | " << std::setw(10) << "Min (ms)"
            << " | " << std::setw(10) << "Max (ms)"
            << " | " << std::setw(10) << "Avg (ms)"
            << " | " << std::setw(10) << "Median\n";
  std::cout << std::string(100, '-') << "\n";

  // Benchmark 1: Box generation
  {
    auto timings = time_function([] { geometry::BoxGenerator::generate(1.0, 1.0, 1.0); }, iterations);
    print_result(calculate_stats("Box Generation (1x1x1)", timings));
  }

  // Benchmark 2: Sphere generation
  {
    auto timings = time_function([] { geometry::SphereGenerator::generate_uv_sphere(1.0, 32, 16); }, iterations);
    print_result(calculate_stats("Sphere Generation (UV, 32x16)", timings));
  }

  // Benchmark 3: GeometryContainer cloning
  {
    auto box = geometry::BoxGenerator::generate(2.0, 2.0, 2.0).value();
    auto timings = time_function([&box] { auto copy = box.clone(); }, iterations);
    print_result(calculate_stats("GeometryContainer Clone", timings));
  }

  // Benchmark 4: Attribute access
  {
    auto sphere = geometry::SphereGenerator::generate_uv_sphere(1.0, 64, 32).value();
    auto timings = time_function(
        [&sphere] {
          auto* pos = sphere.get_point_attribute_typed<core::Vec3f>(core::standard_attrs::P);
          if (pos) {
            auto span = pos->values();
            volatile float sum = 0.0f;
            for (const auto& p : span) {
              sum += p.x() + p.y() + p.z();
            }
          }
        },
        iterations);
    print_result(calculate_stats("Attribute Read (positions)", timings));
  }

  // Benchmark 5: Boolean union
  {
    auto box1 = geometry::BoxGenerator::generate(1.0, 1.0, 1.0).value();
    auto box2 = geometry::BoxGenerator::generate(1.0, 1.0, 1.0).value();
    auto timings =
        time_function([&box1, &box2] { geometry::BooleanOps::union_geometries(box1, box2); }, iterations / 10);
    print_result(calculate_stats("Boolean Union (boxes)", timings));
  }

  // Benchmark 6: Boolean difference
  {
    auto large = geometry::BoxGenerator::generate(2.0, 2.0, 2.0).value();
    auto small = geometry::BoxGenerator::generate(1.0, 1.0, 1.0).value();
    auto timings =
        time_function([&large, &small] { geometry::BooleanOps::difference_geometries(large, small); }, iterations / 10);
    print_result(calculate_stats("Boolean Difference (large-small)", timings));
  }

  // Benchmark 7: PMP conversion (to PMP)
  {
    auto sphere = geometry::SphereGenerator::generate_uv_sphere(1.0, 32, 16).value();
    auto timings = time_function([&sphere] { processing::detail::PMPConverter::to_pmp(sphere); }, iterations);
    print_result(calculate_stats("Convert to PMP", timings));
  }

  // Benchmark 8: PMP conversion (from PMP)
  {
    auto sphere = geometry::SphereGenerator::generate_uv_sphere(1.0, 32, 16).value();
    auto pmp_mesh = processing::detail::PMPConverter::to_pmp(sphere);
    auto timings = time_function([pmp_mesh] { processing::detail::PMPConverter::from_pmp(pmp_mesh); }, iterations);
    print_result(calculate_stats("Convert from PMP", timings));
  }

  // Benchmark 9: Complex sphere generation
  {
    auto timings = time_function([] { geometry::SphereGenerator::generate_uv_sphere(1.0, 128, 64); }, iterations / 10);
    print_result(calculate_stats("Sphere Generation (UV, 128x64)", timings));
  }

  // Benchmark 10: Boolean with complex geometry
  {
    auto sphere1 = geometry::SphereGenerator::generate_uv_sphere(1.0, 64, 32).value();
    auto sphere2 = geometry::SphereGenerator::generate_uv_sphere(1.0, 64, 32).value();

    // Offset sphere2
    auto* pos2 = sphere2.get_point_attribute_typed<core::Vec3f>(core::standard_attrs::P);
    if (pos2) {
      auto pos_span = pos2->values_writable();
      for (auto& p : pos_span) {
        p.x() += 0.5f;
      }
    }

    auto timings = time_function([&sphere1, &sphere2] { geometry::BooleanOps::union_geometries(sphere1, sphere2); },
                                 iterations / 10);
    print_result(calculate_stats("Boolean Union (spheres, 64x32)", timings));
  }

  std::cout << "\n=== Benchmark Complete ===\n";

  return 0;
}
