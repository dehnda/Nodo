/**
 * Nodo Performance Profiler
 * Simple profiling utilities for identifying bottlenecks
 */

#pragma once

#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace nodo::performance {

/**
 * @brief Simple RAII timer for profiling code sections
 */
class ScopedTimer {
public:
  ScopedTimer(const std::string& name, bool print_on_destroy = true)
      : name_(name),
        print_(print_on_destroy),
        start_(std::chrono::high_resolution_clock::now()) {}

  ~ScopedTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
    duration_us_ = duration.count();

    if (print_) {
      std::cout << "[TIMER] " << name_ << ": " << duration_us_ / 1000.0
                << " ms\n";
    }
  }

  long long get_duration_us() const { return duration_us_; }

private:
  std::string name_;
  bool print_;
  std::chrono::time_point<std::chrono::high_resolution_clock> start_;
  long long duration_us_ = 0;
};

/**
 * @brief Accumulating profiler for tracking multiple measurements
 */
class ProfilerSection {
public:
  void add_sample(double duration_ms) {
    samples_.push_back(duration_ms);
    total_time_ms_ += duration_ms;
    if (duration_ms < min_time_ms_ || min_time_ms_ < 0) {
      min_time_ms_ = duration_ms;
    }
    if (duration_ms > max_time_ms_) {
      max_time_ms_ = duration_ms;
    }
  }

  size_t sample_count() const { return samples_.size(); }
  double total_time() const { return total_time_ms_; }
  double average_time() const {
    return samples_.empty() ? 0.0 : total_time_ms_ / samples_.size();
  }
  double min_time() const { return min_time_ms_; }
  double max_time() const { return max_time_ms_; }

  const std::vector<double>& get_samples() const { return samples_; }

private:
  std::vector<double> samples_;
  double total_time_ms_ = 0.0;
  double min_time_ms_ = -1.0;
  double max_time_ms_ = 0.0;
};

/**
 * @brief Global profiler for collecting and reporting performance data
 */
class Profiler {
public:
  static Profiler& instance() {
    static Profiler inst;
    return inst;
  }

  void record(const std::string& section_name, double duration_ms) {
    sections_[section_name].add_sample(duration_ms);
  }

  void clear() { sections_.clear(); }

  void print_report() const {
    if (sections_.empty()) {
      std::cout << "No profiling data collected\n";
      return;
    }

    std::cout
        << "\n╔═══════════════════════════════════════════════════════════╗\n";
    std::cout
        << "║           NODO PERFORMANCE PROFILER REPORT                ║\n";
    std::cout
        << "╠═══════════════════════════════════════════════════════════╣\n";
    std::cout
        << "║ Section                  │  Avg  │  Min  │  Max  │ Calls ║\n";
    std::cout
        << "╠══════════════════════════╪═══════╪═══════╪═══════╪═══════╣\n";

    // Calculate total time
    double total_time = 0.0;
    for (const auto& [name, section] : sections_) {
      total_time += section.total_time();
    }

    // Sort by total time (descending)
    std::vector<std::pair<std::string, const ProfilerSection*>> sorted;
    for (const auto& [name, section] : sections_) {
      sorted.push_back({name, &section});
    }
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
      return a.second->total_time() > b.second->total_time();
    });

    // Print each section
    for (const auto& [name, section] : sorted) {
      // double percent = (section->total_time() / total_time) * 100.0;

      char line[200];
      snprintf(line, sizeof(line),
               "║ %-24s │ %5.1fms │ %5.1fms │ %5.1fms │ %5zu ║", name.c_str(),
               section->average_time(), section->min_time(),
               section->max_time(), section->sample_count());
      std::cout << line << "\n";
    }

    std::cout
        << "╠═══════════════════════════════════════════════════════════╣\n";
    char total_line[200];
    snprintf(total_line, sizeof(total_line),
             "║ TOTAL TIME: %8.2f ms                                   ║",
             total_time);
    std::cout << total_line << "\n";
    std::cout
        << "╚═══════════════════════════════════════════════════════════╝\n\n";
  }

  const std::map<std::string, ProfilerSection>& get_sections() const {
    return sections_;
  }

private:
  Profiler() = default;
  std::map<std::string, ProfilerSection> sections_;
};

/**
 * @brief RAII helper that automatically records to global profiler
 */
class AutoProfiler {
public:
  AutoProfiler(const std::string& section_name)
      : name_(section_name),
        start_(std::chrono::high_resolution_clock::now()) {}

  ~AutoProfiler() {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
    double duration_ms = duration.count() / 1000.0;
    Profiler::instance().record(name_, duration_ms);
  }

private:
  std::string name_;
  std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

// Convenience macro for profiling a code block
#define NODO_PROFILE(name)                                                     \
  nodo::performance::AutoProfiler _profiler_##__LINE__(name)

// Conditional profiling (only when enabled)
#ifdef NODO_ENABLE_PROFILING
  #define NODO_PROFILE_OPTIONAL(name) NODO_PROFILE(name)
#else
  #define NODO_PROFILE_OPTIONAL(name)
#endif

} // namespace nodo::performance
