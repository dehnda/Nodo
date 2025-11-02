#include "nodo/sop/bevel_sop.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <utility>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::sop {

BevelSOP::BevelSOP(const std::string &name) : SOPNode(name, "Bevel") {
  // Input geometry port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Parameters (schema)
  register_parameter(define_float_parameter("width", DEFAULT_WIDTH)
                         .label("Width")
                         .range(0.0, 1000.0)
                         .category("Bevel")
                         .description("Bevel width/offset distance")
                         .build());

  register_parameter(
      define_int_parameter("segments", DEFAULT_SEGMENTS)
          .label("Segments")
          .range(1, 16)
          .category("Bevel")
          .description(
              "Number of segments for rounded bevel (topology-preserving "
              "placeholder uses this as a smoothing hint)")
          .build());

  register_parameter(
      define_float_parameter("profile", DEFAULT_PROFILE)
          .label("Profile")
          .range(0.0, 1.0)
          .category("Bevel")
          .description(
              "Bevel profile shape (0.0 = linear, 0.5 = smooth, 1.0 = sharp)")
          .build());

  register_parameter(
      define_int_parameter("bevel_type", static_cast<int>(BevelType::Vertex))
          .label("Mode")
          .options({"Vertex", "Edge", "Face"})
          .category("Bevel")
          .description("Bevel mode: Vertex/Edge/Face (Face = inset)")
          .build());

  register_parameter(define_bool_parameter("clamp_overlap", true)
                         .label("Clamp Overlap")
                         .category("Bevel")
                         .description("Attempt to avoid overlaps (best effort)")
                         .build());

  // Optional: angle limit for future edge selection (not used in placeholder)
  register_parameter(
      define_float_parameter("angle_limit", 30.0f)
          .label("Angle Limit")
          .range(0.0, 180.0)
          .category("Limits")
          .description(
              "Only bevel edges above this angle (not fully implemented)")
          .build());
}

std::shared_ptr<core::GeometryContainer> BevelSOP::execute() {
  auto input = get_input_data(0);
  if (!input) {
    set_error("No input geometry");
    return nullptr;
  }

  [[maybe_unused]] auto &topology = input->topology();

  return nullptr;
}

} // namespace nodo::sop
