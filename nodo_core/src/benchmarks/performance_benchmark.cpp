#include "nodo/benchmarks/performance_benchmark.hpp"

#include "nodo/geometry/boolean_ops.hpp"
#include "nodo/geometry/mesh_generator.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <random>
#include <sstream>

#ifdef __linux__
  #include <sys/resource.h>
  #include <unistd.h>
#endif

namespace nodo::benchmarks {

// PerformanceBenchmark Implementation
PerformanceBenchmark::PerformanceBenchmark(const BenchmarkConfig& config) : config_(config) {}

const PerformanceBenchmark::BenchmarkResult*
PerformanceBenchmark::BenchmarkSuite::find_result(const std::string& name) const {
  auto it = std::find_if(results.begin(), results.end(),
                         [&name](const BenchmarkResult& result) { return result.operation_name == name; });
  return (it != results.end()) ? &(*it) : nullptr;
}

std::string PerformanceBenchmark::BenchmarkSuite::generate_report() const {
  std::ostringstream report;
  report << std::fixed << std::setprecision(3);

  report << "=== Nodo Performance Benchmark Report ===\n";
  auto timestamp_time_t = std::chrono::system_clock::to_time_t(timestamp);
  report << "Timestamp: " << std::put_time(std::localtime(&timestamp_time_t), "%Y-%m-%d %H:%M:%S") << "\n";
  report << "Configuration: " << test_configuration << "\n\n";

  report << "Results Summary:\n";
  report << std::setw(30) << "Operation" << std::setw(12) << "Avg (ms)" << std::setw(12) << "Min (ms)" << std::setw(12)
         << "Max (ms)" << std::setw(12) << "Std Dev" << std::setw(10) << "Iters" << std::setw(12) << "Memory (KB)"
         << "\n";
  report << std::string(100, '-') << "\n";

  for (const auto& result : results) {
    report << std::setw(30) << result.operation_name << std::setw(12) << result.average_time_ms << std::setw(12)
           << result.min_time_ms << std::setw(12) << result.max_time_ms << std::setw(12) << result.std_dev_ms
           << std::setw(10) << result.iterations << std::setw(12) << (result.memory_usage_bytes / 1024) << "\n";

    if (!result.additional_info.empty()) {
      report << "  Info: " << result.additional_info << "\n";
    }
  }

  return report.str();
}

void PerformanceBenchmark::BenchmarkSuite::export_csv(const std::string& filename) const {
  std::ofstream file(filename);
  if (!file.is_open())
    return;

  // CSV header
  file << "Operation,Average_ms,Min_ms,Max_ms,StdDev_ms,Iterations,Memory_"
          "bytes,Additional_Info\n";

  // Data rows
  for (const auto& result : results) {
    file << result.operation_name << "," << result.average_time_ms << "," << result.min_time_ms << ","
         << result.max_time_ms << "," << result.std_dev_ms << "," << result.iterations << ","
         << result.memory_usage_bytes << "," << "\"" << result.additional_info << "\"\n";
  }
}

PerformanceBenchmark::BenchmarkSuite PerformanceBenchmark::run_bvh_benchmarks() {
  BenchmarkSuite suite;
  suite.test_configuration = "BVH Construction and Query Performance";
  suite.timestamp = std::chrono::system_clock::now();

  for (auto complexity : config_.complexity_levels) {
    auto test_mesh = create_test_mesh(complexity);
    std::string complexity_name;

    switch (complexity) {
      case ComplexityLevel::Simple:
        complexity_name = "Simple";
        break;
      case ComplexityLevel::Medium:
        complexity_name = "Medium";
        break;
      case ComplexityLevel::Complex:
        complexity_name = "Complex";
        break;
      case ComplexityLevel::VeryComplex:
        complexity_name = "VeryComplex";
        break;
    }

    // Benchmark BVH construction
    spatial::BVH bvh;
    spatial::BVH::BuildParams params;

    auto build_timings = time_function([&]() { bvh.build(test_mesh, params); }, config_.iterations);

    auto build_result = calculate_statistics(build_timings, "BVH_Build_" + complexity_name);
    build_result.additional_info = "Triangles: " + std::to_string(test_mesh.faces().rows());

    if (config_.measure_memory) {
      build_result.memory_usage_bytes = estimate_memory_usage(bvh);
    }

    suite.results.push_back(build_result);

    // Benchmark ray intersection
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> pos_dist(-2.0, 2.0);
    std::uniform_real_distribution<double> dir_dist(-1.0, 1.0);

    Eigen::Vector3d ray_origin(pos_dist(rng), pos_dist(rng), pos_dist(rng));
    Eigen::Vector3d ray_direction(dir_dist(rng), dir_dist(rng), dir_dist(rng));
    spatial::BVH::Ray test_ray(ray_origin, ray_direction);
    test_ray.t_min = 0.0;
    test_ray.t_max = 1000.0;

    auto ray_timings = time_function([&]() { bvh.intersect_ray(test_ray); }, config_.iterations);

    auto ray_result = calculate_statistics(ray_timings, "BVH_RayIntersect_" + complexity_name);
    suite.results.push_back(ray_result);

    // Benchmark AABB query
    spatial::AABB query_aabb(Eigen::Vector3d(-0.5, -0.5, -0.5), Eigen::Vector3d(0.5, 0.5, 0.5));

    auto aabb_timings = time_function([&]() { bvh.query_aabb(query_aabb); }, config_.iterations);

    auto aabb_result = calculate_statistics(aabb_timings, "BVH_AABBQuery_" + complexity_name);
    suite.results.push_back(aabb_result);
  }

  return suite;
}

PerformanceBenchmark::BenchmarkSuite PerformanceBenchmark::run_bvh_comparison_benchmarks() {
  BenchmarkSuite suite;
  suite.test_configuration = "BVH vs Brute Force Comparison";
  suite.timestamp = std::chrono::system_clock::now();

  for (auto complexity : config_.complexity_levels) {
    auto test_mesh = create_test_mesh(complexity);
    std::string complexity_name;

    switch (complexity) {
      case ComplexityLevel::Simple:
        complexity_name = "Simple";
        break;
      case ComplexityLevel::Medium:
        complexity_name = "Medium";
        break;
      case ComplexityLevel::Complex:
        complexity_name = "Complex";
        break;
      case ComplexityLevel::VeryComplex:
        complexity_name = "VeryComplex";
        break;
    }

    // Build BVH once
    spatial::BVH bvh;
    spatial::BVH::BuildParams params;
    bvh.build(test_mesh, params);

    // Setup test ray
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> pos_dist(-2.0, 2.0);
    std::uniform_real_distribution<double> dir_dist(-1.0, 1.0);

    Eigen::Vector3d ray_origin(pos_dist(rng), pos_dist(rng), pos_dist(rng));
    Eigen::Vector3d ray_direction(dir_dist(rng), dir_dist(rng), dir_dist(rng));
    spatial::BVH::Ray test_ray(ray_origin, ray_direction);
    test_ray.t_min = 0.0;
    test_ray.t_max = 1000.0;

    // Benchmark BVH ray intersection
    auto bvh_ray_timings = time_function([&]() { bvh.intersect_ray(test_ray); }, config_.iterations);

    auto bvh_ray_result = calculate_statistics(bvh_ray_timings, "BVH_RayIntersect_" + complexity_name);
    suite.results.push_back(bvh_ray_result);

    // Benchmark brute force ray intersection
    auto brute_ray_timings = time_function([&]() { brute_force_ray_intersect(test_mesh, test_ray); },
                                           std::min(config_.iterations,
                                                    static_cast<size_t>(10))); // Fewer iterations for brute force

    auto brute_ray_result = calculate_statistics(brute_ray_timings, "BruteForce_RayIntersect_" + complexity_name);
    suite.results.push_back(brute_ray_result);

    // Calculate speedup
    if (brute_ray_result.average_time_ms > 0) {
      double speedup = brute_ray_result.average_time_ms / bvh_ray_result.average_time_ms;
      bvh_ray_result.additional_info = "Speedup: " + std::to_string(speedup) + "x";
    }

    // Setup AABB query
    spatial::AABB query_aabb(Eigen::Vector3d(-0.5, -0.5, -0.5), Eigen::Vector3d(0.5, 0.5, 0.5));

    // Benchmark BVH AABB query
    auto bvh_aabb_timings = time_function([&]() { bvh.query_aabb(query_aabb); }, config_.iterations);

    auto bvh_aabb_result = calculate_statistics(bvh_aabb_timings, "BVH_AABBQuery_" + complexity_name);
    suite.results.push_back(bvh_aabb_result);

    // Benchmark brute force AABB query
    auto brute_aabb_timings = time_function([&]() { brute_force_aabb_query(test_mesh, query_aabb); },
                                            std::min(config_.iterations, static_cast<size_t>(10)));

    auto brute_aabb_result = calculate_statistics(brute_aabb_timings, "BruteForce_AABBQuery_" + complexity_name);
    suite.results.push_back(brute_aabb_result);

    // Calculate speedup
    if (brute_aabb_result.average_time_ms > 0) {
      double speedup = brute_aabb_result.average_time_ms / bvh_aabb_result.average_time_ms;
      bvh_aabb_result.additional_info = "Speedup: " + std::to_string(speedup) + "x";
    }
  }

  return suite;
}

// Boolean benchmarks temporarily disabled during Mesh deprecation
/*
PerformanceBenchmark::BenchmarkSuite PerformanceBenchmark::run_boolean_benchmarks() {
  BenchmarkSuite suite;
  suite.test_configuration = "Boolean Operation Performance";
  suite.timestamp = std::chrono::system_clock::now();

  for (auto complexity : config_.complexity_levels) {
    auto mesh_a = create_test_mesh(complexity);
    auto mesh_b = create_test_mesh(complexity);

    std::string complexity_name;
    switch (complexity) {
      case ComplexityLevel::Simple:
        complexity_name = "Simple";
        break;
      case ComplexityLevel::Medium:
        complexity_name = "Medium";
        break;
      case ComplexityLevel::Complex:
        complexity_name = "Complex";
        break;
      case ComplexityLevel::VeryComplex:
        complexity_name = "VeryComplex";
        break;
    }

    // Benchmark boolean operations (using Manifold internally)
    auto union_timings = time_function([&]() { geometry::BooleanOps::union_meshes(mesh_a, mesh_b); },
                                       std::min(config_.iterations / 10,
                                                static_cast<size_t>(5))); // Boolean ops are expensive

    auto union_result = calculate_statistics(union_timings, "Union_" + complexity_name);
    union_result.additional_info = "Manifold CSG";
    suite.results.push_back(union_result);
  }

  return suite;
}
*/

PerformanceBenchmark::BenchmarkSuite PerformanceBenchmark::run_parameter_optimization_benchmarks() {
  BenchmarkSuite suite;
  suite.test_configuration = "BVH Parameter Optimization";
  suite.timestamp = std::chrono::system_clock::now();

  auto test_mesh = create_test_mesh(ComplexityLevel::Medium);

  // Test different max_triangles_per_leaf values
  std::vector<int> leaf_sizes = {1, 4, 8, 16, 32, 64};
  for (int leaf_size : leaf_sizes) {
    spatial::BVH bvh;
    spatial::BVH::BuildParams params;
    params.max_triangles_per_leaf = leaf_size;
    params.use_sah = true;

    auto build_timings = time_function([&]() { bvh.build(test_mesh, params); }, config_.iterations / 2);

    auto result = calculate_statistics(build_timings, "BVH_Build_Leaf" + std::to_string(leaf_size));
    result.additional_info = "Max triangles per leaf: " + std::to_string(leaf_size);
    suite.results.push_back(result);
  }

  // Test SAH vs midpoint splitting
  spatial::BVH bvh_sah, bvh_midpoint;
  spatial::BVH::BuildParams sah_params, midpoint_params;
  sah_params.use_sah = true;
  midpoint_params.use_sah = false;

  auto sah_timings = time_function([&]() { bvh_sah.build(test_mesh, sah_params); }, config_.iterations / 2);

  auto sah_result = calculate_statistics(sah_timings, "BVH_Build_SAH");
  sah_result.additional_info = "Surface Area Heuristic";
  suite.results.push_back(sah_result);

  auto midpoint_timings =
      time_function([&]() { bvh_midpoint.build(test_mesh, midpoint_params); }, config_.iterations / 2);

  auto midpoint_result = calculate_statistics(midpoint_timings, "BVH_Build_Midpoint");
  midpoint_result.additional_info = "Midpoint splitting";
  suite.results.push_back(midpoint_result);

  return suite;
}

core::Mesh PerformanceBenchmark::create_test_mesh(ComplexityLevel level) {
  switch (level) {
    case ComplexityLevel::Simple:
      return create_test_mesh(100);
    case ComplexityLevel::Medium:
      return create_test_mesh(1000);
    case ComplexityLevel::Complex:
      return create_test_mesh(10000);
    case ComplexityLevel::VeryComplex:
      return create_test_mesh(100000);
    default:
      return create_test_mesh(1000);
  }
}

core::Mesh PerformanceBenchmark::create_test_mesh(size_t triangle_count) {
  // Create a sphere with appropriate subdivision level to reach target triangle
  // count
  int subdivisions = 0;
  size_t current_triangles = 8; // Base icosahedron

  while (current_triangles < triangle_count && subdivisions < 10) {
    subdivisions++;
    current_triangles *= 4; // Each subdivision quadruples triangle count
  }

  auto sphere_opt = geometry::MeshGenerator::sphere(Eigen::Vector3d::Zero(), 1.0, subdivisions);

  if (sphere_opt) {
    return *sphere_opt;
  }

  // Fallback to box if sphere generation fails
  return geometry::MeshGenerator::box(Eigen::Vector3d(-1, -1, -1), Eigen::Vector3d(1, 1, 1));
}

std::vector<double> PerformanceBenchmark::time_function(std::function<void()> func, size_t iterations) {
  std::vector<double> timings;
  timings.reserve(iterations);

  // Warm-up runs
  if (config_.warm_up_runs) {
    for (size_t i = 0; i < config_.warm_up_iterations; ++i) {
      func();
    }
  }

  // Actual timing runs
  for (size_t i = 0; i < iterations; ++i) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration<double, std::milli>(end - start);
    timings.push_back(duration.count());
  }

  return timings;
}

PerformanceBenchmark::BenchmarkResult PerformanceBenchmark::calculate_statistics(const std::vector<double>& timings,
                                                                                 const std::string& operation_name) {
  BenchmarkResult result;
  result.operation_name = operation_name;
  result.iterations = timings.size();

  if (timings.empty())
    return result;

  // Calculate statistics
  result.min_time_ms = *std::min_element(timings.begin(), timings.end());
  result.max_time_ms = *std::max_element(timings.begin(), timings.end());
  result.average_time_ms = std::accumulate(timings.begin(), timings.end(), 0.0) / timings.size();

  // Calculate standard deviation
  double variance = 0.0;
  for (double time : timings) {
    variance += (time - result.average_time_ms) * (time - result.average_time_ms);
  }
  variance /= timings.size();
  result.std_dev_ms = std::sqrt(variance);

  return result;
}

std::optional<spatial::BVH::RayHit> PerformanceBenchmark::brute_force_ray_intersect(const core::Mesh& mesh,
                                                                                    const spatial::BVH::Ray& ray) {
  spatial::BVH::RayHit closest_hit;
  closest_hit.t = ray.t_max;
  bool found_hit = false;

  for (int tri_idx = 0; tri_idx < mesh.faces().rows(); ++tri_idx) {
    const auto& face = mesh.faces().row(tri_idx);
    Eigen::Vector3d v0 = mesh.vertices().row(face[0]);
    Eigen::Vector3d v1 = mesh.vertices().row(face[1]);
    Eigen::Vector3d v2 = mesh.vertices().row(face[2]);

    // Möller-Trumbore algorithm (same as BVH implementation)
    Eigen::Vector3d edge1 = v1 - v0;
    Eigen::Vector3d edge2 = v2 - v0;
    Eigen::Vector3d h = ray.direction.cross(edge2);
    double det = edge1.dot(h);

    const double epsilon = 1e-9;
    if (std::abs(det) < epsilon)
      continue;

    double inv_det = 1.0 / det;
    Eigen::Vector3d distance = ray.origin - v0;
    double u_coord = distance.dot(h) * inv_det;

    if (u_coord < 0.0 || u_coord > 1.0)
      continue;

    Eigen::Vector3d q_vec = distance.cross(edge1);
    double v_coord = ray.direction.dot(q_vec) * inv_det;

    if (v_coord < 0.0 || u_coord + v_coord > 1.0)
      continue;

    double t_val = edge2.dot(q_vec) * inv_det;

    if (t_val > epsilon && t_val < closest_hit.t) {
      closest_hit.t = t_val;
      closest_hit.triangle_index = tri_idx;
      closest_hit.point = ray.origin + t_val * ray.direction;
      closest_hit.normal = edge1.cross(edge2).normalized();
      closest_hit.barycentric = Eigen::Vector2d(u_coord, v_coord);
      found_hit = true;
    }
  }

  return found_hit ? std::optional<spatial::BVH::RayHit>(closest_hit) : std::nullopt;
}

std::vector<int> PerformanceBenchmark::brute_force_aabb_query(const core::Mesh& mesh, const spatial::AABB& aabb) {
  std::vector<int> results;

  for (int tri_idx = 0; tri_idx < mesh.faces().rows(); ++tri_idx) {
    const auto& face = mesh.faces().row(tri_idx);

    // Check if any vertex of the triangle is inside the AABB
    bool triangle_intersects = false;
    for (int vertex_idx = 0; vertex_idx < 3; ++vertex_idx) {
      Eigen::Vector3d vertex = mesh.vertices().row(face[vertex_idx]);
      if (aabb.contains(vertex)) {
        triangle_intersects = true;
        break;
      }
    }

    if (triangle_intersects) {
      results.push_back(tri_idx);
    }
  }

  return results;
}

template <typename T>
size_t PerformanceBenchmark::estimate_memory_usage(const T& obj) {
  // This is a simplified estimation - in a real implementation,
  // you'd want more sophisticated memory tracking
  return sizeof(T);
}

// MemoryTracker Implementation
bool MemoryTracker::tracking_active_ = false;
MemoryTracker::MemorySnapshot MemoryTracker::baseline_snapshot_;
MemoryTracker::MemorySnapshot MemoryTracker::peak_snapshot_;

void MemoryTracker::start_tracking() {
  baseline_snapshot_ = take_snapshot();
  peak_snapshot_ = baseline_snapshot_;
  tracking_active_ = true;
}

MemoryTracker::MemorySnapshot MemoryTracker::take_snapshot() {
  MemorySnapshot snapshot;
  snapshot.timestamp = std::chrono::system_clock::now();

#ifdef __linux__
  // Get memory usage on Linux
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) == 0) {
    snapshot.resident_memory_kb = usage.ru_maxrss; // Already in KB on Linux
    snapshot.peak_memory_kb = usage.ru_maxrss;
  }

  // Get virtual memory from /proc/self/status
  std::ifstream status_file("/proc/self/status");
  std::string line;
  while (std::getline(status_file, line)) {
    if (line.substr(0, 7) == "VmSize:") {
      std::istringstream iss(line.substr(7));
      iss >> snapshot.virtual_memory_kb;
      break;
    }
  }
#else
  // Placeholder for other platforms
  snapshot.resident_memory_kb = 0;
  snapshot.virtual_memory_kb = 0;
  snapshot.peak_memory_kb = 0;
#endif

  return snapshot;
}

MemoryTracker::MemorySnapshot MemoryTracker::stop_tracking() {
  tracking_active_ = false;
  return peak_snapshot_;
}

} // namespace nodo::benchmarks
