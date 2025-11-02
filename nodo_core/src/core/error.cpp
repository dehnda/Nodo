#include "nodo/core/error.hpp"
#include <sstream>

namespace nodo::core {

std::string Error::description() const {
  std::ostringstream oss;
  oss << "[" << ErrorUtils::category_to_string(category) << "] "
      << ErrorUtils::code_to_string(code) << ": " << message;
  if (!context.empty()) {
    oss << " (" << context << ")";
  }
  return oss.str();
}

bool Error::is_recoverable() const noexcept {
  switch (code) {
  case ErrorCode::OutOfMemory:
  case ErrorCode::InitializationFailed:
    return false;
  case ErrorCode::InvalidMesh:
  case ErrorCode::NonManifoldMesh:
  case ErrorCode::EmptyMesh:
  case ErrorCode::BooleanOperationFailed:
  case ErrorCode::FileNotFound:
  case ErrorCode::InvalidFormat:
  case ErrorCode::ReadError:
  case ErrorCode::WriteError:
  case ErrorCode::DegenerateFaces:
  case ErrorCode::DuplicateVertices:
  case ErrorCode::UnreferencedVertices:
  case ErrorCode::NonClosedMesh:
  case ErrorCode::CompilationFailed:
  case ErrorCode::UnsupportedOperation:
  case ErrorCode::RuntimeError:
  case ErrorCode::Unknown:
    return true;
  }
  return true; // Default case
}

std::string ErrorUtils::category_to_string(ErrorCategory category) {
  switch (category) {
  case ErrorCategory::Geometry:
    return "Geometry";
  case ErrorCategory::IO:
    return "IO";
  case ErrorCategory::Validation:
    return "Validation";
  case ErrorCategory::Memory:
    return "Memory";
  case ErrorCategory::GPU:
    return "GPU";
  case ErrorCategory::System:
    return "System";
  case ErrorCategory::Unknown:
    return "Unknown";
  }
  return "Unknown"; // Default case
}

std::string ErrorUtils::code_to_string(ErrorCode code) {
  switch (code) {
  // Geometry errors
  case ErrorCode::InvalidMesh:
    return "Invalid mesh";
  case ErrorCode::NonManifoldMesh:
    return "Non-manifold mesh";
  case ErrorCode::EmptyMesh:
    return "Empty mesh";
  case ErrorCode::BooleanOperationFailed:
    return "Boolean operation failed";

  // IO errors
  case ErrorCode::FileNotFound:
    return "File not found";
  case ErrorCode::InvalidFormat:
    return "Invalid format";
  case ErrorCode::ReadError:
    return "Read error";
  case ErrorCode::WriteError:
    return "Write error";

  // Validation errors
  case ErrorCode::DegenerateFaces:
    return "Degenerate faces";
  case ErrorCode::DuplicateVertices:
    return "Duplicate vertices";
  case ErrorCode::UnreferencedVertices:
    return "Unreferenced vertices";
  case ErrorCode::NonClosedMesh:
    return "Non-closed mesh";

  // GPU/System errors
  case ErrorCode::InitializationFailed:
    return "Initialization failed";
  case ErrorCode::CompilationFailed:
    return "Compilation failed";
  case ErrorCode::UnsupportedOperation:
    return "Unsupported operation";
  case ErrorCode::RuntimeError:
    return "Runtime error";

  // General
  case ErrorCode::OutOfMemory:
    return "Out of memory";
  case ErrorCode::Unknown:
    return "Unknown error";
  }
  return "Unknown error"; // Default case
}

Error ErrorUtils::make_error(ErrorCategory category, ErrorCode code,
                             const std::string &message,
                             const std::string &context) {
  return Error(category, code, message, context);
}

} // namespace nodo::core
