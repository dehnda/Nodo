#pragma once

#include <string>
#include <system_error>

namespace nodeflux::core {

/**
 * @brief Error categories for different subsystems
 */
enum class ErrorCategory {
  Geometry,   ///< Geometric operations (boolean, transformations)
  IO,         ///< Input/output operations
  Validation, ///< Mesh validation errors
  Memory,     ///< Memory allocation errors
  GPU,        ///< GPU/compute shader errors
  System,     ///< System-level errors
  Unknown     ///< Unknown or unclassified errors
};

/**
 * @brief Specific error codes within each category
 */
enum class ErrorCode {
  // Geometry errors
  InvalidMesh,
  NonManifoldMesh,
  EmptyMesh,
  BooleanOperationFailed,

  // IO errors
  FileNotFound,
  InvalidFormat,
  ReadError,
  WriteError,

  // Validation errors
  DegenerateFaces,
  DuplicateVertices,
  UnreferencedVertices,
  NonClosedMesh,

  // GPU/System errors
  InitializationFailed,
  CompilationFailed,
  UnsupportedOperation,
  RuntimeError,

  // General
  OutOfMemory,
  Unknown
};

/**
 * @brief Error information with context
 */
struct Error {
  ErrorCategory category;
  ErrorCode code;
  std::string message;
  std::string context; ///< Additional context information

  Error(ErrorCategory cat, ErrorCode c, std::string msg, std::string ctx = "")
      : category(cat), code(c), message(std::move(msg)),
        context(std::move(ctx)) {}

  /// Get human-readable error description
  [[nodiscard]] std::string description() const;

  /// Check if this is a recoverable error
  [[nodiscard]] bool is_recoverable() const noexcept;
};

/**
 * @brief Utility functions for error handling
 */
class ErrorUtils {
public:
  /// Convert error category to string
  static std::string category_to_string(ErrorCategory category);

  /// Convert error code to string
  static std::string code_to_string(ErrorCode code);

  /// Create a formatted error message
  static Error make_error(ErrorCategory category, ErrorCode code,
                          const std::string &message,
                          const std::string &context = "");
};

} // namespace nodeflux::core
