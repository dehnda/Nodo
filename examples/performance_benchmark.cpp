#include "nodeflux/benchmarks/performance_benchmark.hpp"
#include <iostream>
#include <string>

using namespace nodeflux::benchmarks;

void print_usage() {
    std::cout << "Usage: performance_benchmark [OPTIONS]\n";
    std::cout << "Options:\n";
    std::cout << "  --bvh              Run BVH construction benchmarks\n";
    std::cout << "  --comparison       Run BVH vs brute-force comparison\n";
    std::cout << "  --boolean          Run boolean operation benchmarks\n";
    std::cout << "  --parameters       Run BVH parameter optimization\n";
    std::cout << "  --all              Run all benchmarks\n";
    std::cout << "  --iterations N     Set number of iterations (default: 100)\n";
    std::cout << "  --output FILE      Export results to CSV file\n";
    std::cout << "  --help             Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::cout << "NodeFlux Engine Performance Benchmark Suite\n";
    std::cout << "==========================================\n\n";
    
    // Parse command line arguments
    bool run_bvh = false;
    bool run_comparison = false;
    bool run_boolean = false;
    bool run_parameters = false;
    bool run_all = false;
    size_t iterations = 100;
    std::string output_file;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--bvh") {
            run_bvh = true;
        } else if (arg == "--comparison") {
            run_comparison = true;
        } else if (arg == "--boolean") {
            run_boolean = true;
        } else if (arg == "--parameters") {
            run_parameters = true;
        } else if (arg == "--all") {
            run_all = true;
        } else if (arg == "--iterations" && i + 1 < argc) {
            iterations = std::stoul(argv[++i]);
        } else if (arg == "--output" && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg == "--help") {
            print_usage();
            return 0;
        } else {
            std::cout << "Unknown option: " << arg << "\n";
            print_usage();
            return 1;
        }
    }
    
    // If no specific benchmarks selected, run key ones
    if (!run_bvh && !run_comparison && !run_boolean && !run_parameters && !run_all) {
        run_bvh = true;
        run_comparison = true;
    }
    
    // Configure benchmark settings
    PerformanceBenchmark::BenchmarkConfig config;
    config.iterations = iterations;
    config.measure_memory = true;
    config.warm_up_runs = true;
    
    // For comprehensive testing, include VeryComplex level
    if (run_all) {
        config.complexity_levels = {
            PerformanceBenchmark::ComplexityLevel::Simple,
            PerformanceBenchmark::ComplexityLevel::Medium,
            PerformanceBenchmark::ComplexityLevel::Complex,
            PerformanceBenchmark::ComplexityLevel::VeryComplex
        };
    }
    
    PerformanceBenchmark benchmark(config);
    std::vector<PerformanceBenchmark::BenchmarkSuite> all_results;
    
    try {
        // Run BVH benchmarks
        if (run_bvh || run_all) {
            std::cout << "Running BVH construction and query benchmarks...\n";
            auto bvh_results = benchmark.run_bvh_benchmarks();
            std::cout << bvh_results.generate_report() << "\n";
            all_results.push_back(bvh_results);
        }
        
        // Run comparison benchmarks
        if (run_comparison || run_all) {
            std::cout << "Running BVH vs brute-force comparison benchmarks...\n";
            auto comp_results = benchmark.run_bvh_comparison_benchmarks();
            std::cout << comp_results.generate_report() << "\n";
            all_results.push_back(comp_results);
        }
        
        // Run boolean operation benchmarks
        if (run_boolean || run_all) {
            std::cout << "Running boolean operation benchmarks...\n";
            auto bool_results = benchmark.run_boolean_benchmarks();
            std::cout << bool_results.generate_report() << "\n";
            all_results.push_back(bool_results);
        }
        
        // Run parameter optimization benchmarks
        if (run_parameters || run_all) {
            std::cout << "Running BVH parameter optimization benchmarks...\n";
            auto param_results = benchmark.run_parameter_optimization_benchmarks();
            std::cout << param_results.generate_report() << "\n";
            all_results.push_back(param_results);
        }
        
        // Export to CSV if requested
        if (!output_file.empty() && !all_results.empty()) {
            std::cout << "Exporting results to " << output_file << "...\n";
            
            // Combine all results into one suite for export
            PerformanceBenchmark::BenchmarkSuite combined_suite;
            combined_suite.test_configuration = "Combined Performance Benchmarks";
            combined_suite.timestamp = std::chrono::system_clock::now();
            
            for (const auto& suite : all_results) {
                combined_suite.results.insert(combined_suite.results.end(),
                    suite.results.begin(), suite.results.end());
            }
            
            combined_suite.export_csv(output_file);
            std::cout << "Results exported successfully.\n";
        }
        
        std::cout << "\n=== Performance Benchmark Summary ===\n";
        std::cout << "Total benchmark suites run: " << all_results.size() << "\n";
        
        size_t total_tests = 0;
        for (const auto& suite : all_results) {
            total_tests += suite.results.size();
        }
        std::cout << "Total individual tests: " << total_tests << "\n";
        std::cout << "Iterations per test: " << iterations << "\n";
        
        // Find performance highlights
        double fastest_operation = std::numeric_limits<double>::max();
        double slowest_operation = 0.0;
        std::string fastest_name, slowest_name;
        
        for (const auto& suite : all_results) {
            for (const auto& result : suite.results) {
                if (result.average_time_ms < fastest_operation) {
                    fastest_operation = result.average_time_ms;
                    fastest_name = result.operation_name;
                }
                if (result.average_time_ms > slowest_operation) {
                    slowest_operation = result.average_time_ms;
                    slowest_name = result.operation_name;
                }
            }
        }
        
        std::cout << "Fastest operation: " << fastest_name 
                  << " (" << fastest_operation << " ms)\n";
        std::cout << "Slowest operation: " << slowest_name 
                  << " (" << slowest_operation << " ms)\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed with error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nBenchmarking completed successfully!\n";
    return 0;
}
