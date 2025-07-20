#include "../../include/nodeflux/core/error.hpp"
#include <sstream>

namespace nodeflux::core {

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
  case ErrorCode::CGALInitializationFailed:
    return false;
  case ErrorCode::InvalidMesh:
  case ErrorCode::EmptyMesh:
  case ErrorCode::FileNotFound:
  case ErrorCode::InvalidFormat:
    return true;
  default:
    return true;
  }
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
  case ErrorCategory::CGAL:
    return "CGAL";
  case ErrorCategory::Unknown:
    return "Unknown";
  default:
    return "Unknown";
  }
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

  // CGAL errors
  case ErrorCode::CGALInitializationFailed:
    return "CGAL initialization failed";
  case ErrorCode::CGALConversionError:
    return "CGAL conversion error";
  case ErrorCode::CGALOperationTimeout:
    return "CGAL operation timeout";

  // General
  case ErrorCode::OutOfMemory:
    return "Out of memory";
  case ErrorCode::Unknown:
    return "Unknown error";
  default:
    return "Unknown error";
  }
}

Error ErrorUtils::make_error(ErrorCategory category, ErrorCode code,
                             const std::string &message,
                             const std::string &context) {
  return Error(category, code, message, context);
}

} // namespace nodeflux::core
