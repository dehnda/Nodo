#include "nodo/core/IHostInterface.h"
#include "nodo/graph/execution_engine.hpp"
#include "nodo/graph/node_graph.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace nodo;
using namespace nodo::graph;

/**
 * Example host interface implementation for testing
 * Shows how a host application (engine, CLI tool, etc.) can integrate with
 * nodo_core
 */
class ExampleHostInterface : public IHostInterface {
public:
  bool report_progress(int current, int total, const std::string& message) override {
    // Track progress calls
    progress_calls_++;
    last_progress_message_ = message;
    last_current_ = current;
    last_total_ = total;

    // Check if user wants to cancel
    return !should_cancel_;
  }

  bool is_cancelled() const override { return should_cancel_; }

  void log(const std::string& level, const std::string& message) override {
    // Store log messages for verification
    log_messages_.push_back("[" + level + "] " + message);
  }

  std::string resolve_path(const std::string& relative_path) const override {
    // Convert project-relative paths to absolute paths
    return project_root_ + "/" + relative_path;
  }

  std::string get_host_info() const override { return "Example Host Application v1.0"; }

  // Test helper methods
  void set_project_root(const std::string& root) { project_root_ = root; }

  void cancel() { should_cancel_ = true; }

  int get_progress_calls() const { return progress_calls_; }
  const std::vector<std::string>& get_log_messages() const { return log_messages_; }
  int get_last_current() const { return last_current_; }
  int get_last_total() const { return last_total_; }

private:
  std::string project_root_ = "/home/user/project";
  bool should_cancel_ = false;
  int progress_calls_ = 0;
  int last_current_ = 0;
  int last_total_ = 0;
  std::string last_progress_message_;
  std::vector<std::string> log_messages_;
};

// Test: Default host interface
TEST(HostInterfaceTest, DefaultHostInterfaceWorks) {
  DefaultHostInterface default_host;

  // Test host info
  EXPECT_EQ(default_host.get_host_info(), "Nodo Studio (Standalone)");

  // Test default implementations (should not crash)
  EXPECT_TRUE(default_host.report_progress(5, 10, "test"));
  EXPECT_FALSE(default_host.is_cancelled());
  EXPECT_EQ(default_host.resolve_path("test.obj"), "test.obj");

  // Log should work (writes to console)
  default_host.log("info", "Test message");
}

// Test: Custom host interface
TEST(HostInterfaceTest, CustomHostInterfaceWorks) {
  ExampleHostInterface host;
  host.set_project_root("/my/project");

  // Test host info
  EXPECT_EQ(host.get_host_info(), "Example Host Application v1.0");

  // Test path resolution
  EXPECT_EQ(host.resolve_path("assets/model.obj"), "/my/project/assets/model.obj");

  // Test progress reporting
  EXPECT_TRUE(host.report_progress(5, 10, "Processing"));
  EXPECT_EQ(host.get_progress_calls(), 1);
  EXPECT_EQ(host.get_last_current(), 5);
  EXPECT_EQ(host.get_last_total(), 10);

  // Test cancellation
  EXPECT_FALSE(host.is_cancelled());
  host.cancel();
  EXPECT_TRUE(host.is_cancelled());
  EXPECT_FALSE(host.report_progress(6, 10, "Should cancel"));

  // Test logging
  host.log("info", "Test info message");
  host.log("error", "Test error message");
  EXPECT_EQ(host.get_log_messages().size(), 2);
  EXPECT_EQ(host.get_log_messages()[0], "[info] Test info message");
  EXPECT_EQ(host.get_log_messages()[1], "[error] Test error message");
}

// Test: Execution engine integration
TEST(HostInterfaceTest, ExecutionEngineIntegration) {
  NodeGraph graph;
  ExecutionEngine engine;

  // Test 1: Standalone mode (no host interface)
  EXPECT_EQ(engine.get_host_interface(), nullptr);
  bool result = engine.execute_graph(graph);
  EXPECT_TRUE(result);

  // Test 2: With custom host interface
  ExampleHostInterface host;
  engine.set_host_interface(&host);
  EXPECT_NE(engine.get_host_interface(), nullptr);
  EXPECT_EQ(engine.get_host_interface()->get_host_info(), "Example Host Application v1.0");

  result = engine.execute_graph(graph);
  EXPECT_TRUE(result);

  // Test 3: Switch to default host interface
  DefaultHostInterface default_host;
  engine.set_host_interface(&default_host);
  EXPECT_EQ(engine.get_host_interface()->get_host_info(), "Nodo Studio (Standalone)");

  result = engine.execute_graph(graph);
  EXPECT_TRUE(result);

  // Test 4: Remove host interface (back to nullptr)
  engine.set_host_interface(nullptr);
  EXPECT_EQ(engine.get_host_interface(), nullptr);

  result = engine.execute_graph(graph);
  EXPECT_TRUE(result);
}

// Test: Zero overhead when not used
TEST(HostInterfaceTest, ZeroOverheadWhenNull) {
  NodeGraph graph;
  ExecutionEngine engine;

  // Execute without host interface - should work normally
  engine.set_host_interface(nullptr);

  bool result = engine.execute_graph(graph);
  EXPECT_TRUE(result);

  // No crashes, no side effects
  EXPECT_EQ(engine.get_host_interface(), nullptr);
}
