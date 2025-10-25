#include "nodeflux/sop/laplacian_sop.hpp"
#include "nodeflux/core/attribute_types.hpp"
#include "nodeflux/core/types.hpp"
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace attrs = nodeflux::core::standard_attrs;

namespace nodeflux::sop {

LaplacianSOP::LaplacianSOP(const std::string &name)
    : SOPNode(name, "Laplacian") {
  // Add input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Define parameters with UI metadata (SINGLE SOURCE OF TRUTH)
  register_parameter(define_int_parameter("iterations", 5)
                         .label("Iterations")
                         .range(1, 100)
                         .category("Smoothing")
                         .build());

  register_parameter(define_float_parameter("lambda", 0.5F)
                         .label("Lambda")
                         .range(0.0, 1.0)
                         .category("Smoothing")
                         .build());

  register_parameter(define_int_parameter("method", 0)
                         .label("Method")
                         .options({"Uniform", "Cotangent", "Taubin"})
                         .category("Smoothing")
                         .build());
}

std::shared_ptr<core::GeometryContainer> LaplacianSOP::execute() {
  // TODO: Implement Laplacian smoothing with GeometryContainer
  auto input = get_input_data(0);
  if (!input) {
    return nullptr;
  }
  return std::make_shared<core::GeometryContainer>(input->clone());
}

} // namespace nodeflux::sop
