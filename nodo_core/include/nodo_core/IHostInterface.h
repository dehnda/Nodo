#pragma once

#include <string>

namespace nodo {

/**
 * @brief Abstract interface for host application integration
 *
 * Allows nodo_core to communicate with host applications (standalone, engine
 * plugins, etc.) without direct dependencies. All methods are optional - null
 * checks ensure zero overhead when not implemented.
 *
 * Use cases:
 * - Progress reporting for long operations
 * - Cancellation support for interactive applications
 * - Logging integration with host application
 * - Path resolution for assets in engine contexts
 *
 * @since M2.1
 */
class IHostInterface {
public:
  virtual ~IHostInterface() = default;

  /**
   * @brief Report progress of a long-running operation
   * @param current Current step/value (e.g., node index)
   * @param total Total steps/value (e.g., total nodes)
   * @param message Optional descriptive message
   * @return true to continue, false to cancel
   *
   * Called during graph execution, mesh processing, etc.
   * Host can display progress bar, update UI, or cancel operation.
   */
  virtual bool report_progress(int current, int total,
                               const std::string& message = "") {
    return true; // Default: continue without displaying progress
  }

  /**
   * @brief Check if the current operation should be cancelled
   * @return true if operation should stop, false to continue
   *
   * Called frequently during expensive operations.
   * Host can check user input, timeout, or other cancellation conditions.
   */
  virtual bool is_cancelled() const {
    return false; // Default: never cancel
  }

  /**
   * @brief Log a message to the host application
   * @param level Severity: "info", "warning", "error", "debug"
   * @param message Log message
   *
   * Host can display in console, file, or custom logging system.
   */
  virtual void log(const std::string& level, const std::string& message) {
    // Default: no-op (silent)
  }

  /**
   * @brief Resolve a relative path to an absolute path
   * @param relative_path Path relative to project/asset directory
   * @return Absolute path in host's filesystem
   *
   * Useful for loading assets (textures, OBJ files) in engine contexts
   * where project paths differ from filesystem paths.
   */
  virtual std::string resolve_path(const std::string& relative_path) const {
    return relative_path; // Default: pass-through (no resolution)
  }

  /**
   * @brief Get host application information
   * @return String identifying host (e.g., "Nodo Studio 1.0", "Godot Plugin
   * 0.1")
   *
   * Useful for debugging, analytics, or version-specific behavior.
   */
  virtual std::string get_host_info() const {
    return "Unknown Host"; // Default: generic
  }
};

/**
 * @brief Default implementation for standalone mode
 *
 * Provides basic console logging and no-op implementations for other methods.
 * Used by Nodo Studio when no custom host interface is provided.
 */
class DefaultHostInterface : public IHostInterface {
public:
  void log(const std::string& level, const std::string& message) override;
  std::string get_host_info() const override;
};

} // namespace nodo
