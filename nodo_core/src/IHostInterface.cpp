#include "nodo_core/IHostInterface.h"
#include <iostream>

namespace nodo {

void DefaultHostInterface::log(const std::string &level,
                               const std::string &message) {
  // Simple console output with level prefix
  std::cout << "[" << level << "] " << message << '\n';
}

std::string DefaultHostInterface::get_host_info() const {
  return "Nodo Studio (Standalone)";
}

} // namespace nodo
