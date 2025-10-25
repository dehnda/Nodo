#include "nodeflux/sop/scale_sop.hpp"
#include "nodeflux/core/standard_attributes.hpp"

namespace nodeflux::sop {

ScaleSOP::ScaleSOP(const std::string &name) : SOPNode(name, "Scale") {
  // Set up input port
  input_ports_.add_port("0", NodePort::Type::INPUT,
                        NodePort::DataType::GEOMETRY, this);

  // Set up output port (automatic in base class, but shown here for clarity)
  output_ports_.add_port("0", NodePort::Type::OUTPUT,
                         NodePort::DataType::GEOMETRY, this);
}

void ScaleSOP::set_scale(float sx, float sy, float sz) {
  if (scale_x_ != sx || scale_y_ != sy || scale_z_ != sz) {
    scale_x_ = sx;
    scale_y_ = sy;
    scale_z_ = sz;
    mark_dirty();
  }
}

void ScaleSOP::set_uniform_scale(float scale) {
  set_scale(scale, scale, scale);
}

std::shared_ptr<core::GeometryContainer> ScaleSOP::execute() {
  // ============================================================================
  // STEP 1: Get input geometry (already GeometryContainer)
  // ============================================================================

  auto input = get_input_data(0);
  if (!input) {
    set_error("No input geometry");
    return nullptr;
  }

  // ============================================================================
  // STEP 2: Clone the geometry (we don't want to modify the input)
  // ============================================================================

  auto result = std::make_shared<core::GeometryContainer>(input->clone());

  // ============================================================================
  // STEP 3: Get typed attribute storage
  // ============================================================================

  auto *positions = result->get_point_attribute_typed<Eigen::Vector3f>("P");

  if (!positions) {
    set_error("Input geometry has no position attribute");
    return nullptr;
  }

  // ============================================================================
  // STEP 4: Calculate scale origin
  // ============================================================================

  Eigen::Vector3f origin(0.0F, 0.0F, 0.0F);

  if (!scale_from_origin_) {
    // Compute centroid
    Eigen::Vector3f sum(0.0F, 0.0F, 0.0F);
    auto pos_span = positions->values();
    for (const auto &pos : pos_span) {
      sum += pos;
    }
    if (pos_span.size() > 0) {
      origin = sum / static_cast<float>(pos_span.size());
    }
  }

  // ============================================================================
  // STEP 5: Process point positions
  // ============================================================================

  Eigen::Vector3f scale_vec(scale_x_, scale_y_, scale_z_);

  // Get writable span for zero-cost iteration
  auto pos_span = positions->values_writable();

  for (auto &pos : pos_span) {
    // Translate to origin, scale, translate back
    Eigen::Vector3f local = pos - origin;
    local = local.cwiseProduct(scale_vec); // Element-wise multiply
    pos = local + origin;
  }

  // ============================================================================
  // STEP 6: Process normals if present (optional attribute)
  // ============================================================================

  if (normalize_normals_ && result->has_point_attribute("N")) {
    auto *normals = result->get_point_attribute_typed<Eigen::Vector3f>("N");

    if (normals != nullptr) {
      // When scaling non-uniformly, normals need to be adjusted
      // Scale by inverse of scale factors
      Eigen::Vector3f inv_scale(1.0F / scale_x_, 1.0F / scale_y_,
                                1.0F / scale_z_);

      auto norm_span = normals->values_writable();
      for (auto &normal : norm_span) {
        normal = normal.cwiseProduct(inv_scale);
        normal.normalize(); // Renormalize
      }
    }
  }

  // ============================================================================
  // STEP 7: Validate result
  // ============================================================================

  if (!result->topology().validate()) {
    set_error("Output geometry validation failed");
    return nullptr;
  }

  // ============================================================================
  // STEP 8: Return the result
  // ============================================================================

  return result;
}

} // namespace nodeflux::sop
