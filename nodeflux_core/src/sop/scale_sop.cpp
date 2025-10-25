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

std::shared_ptr<GeometryData> ScaleSOP::execute() {
  // ============================================================================
  // STEP 1: Get input geometry
  // ============================================================================

  auto input = get_input_data(0);
  if (!input) {
    set_error("No input geometry");
    return nullptr;
  }

  // ============================================================================
  // STEP 2: Convert to GeometryContainer (temporary bridge)
  // ============================================================================
  //
  // NOTE: In the future, SOPs will receive GeometryContainer directly.
  // For now, we convert from the old Mesh-based GeometryData.

  auto container = convert_to_container(*input);
  if (!container) {
    set_error("Failed to convert input geometry");
    return nullptr;
  }

  // ============================================================================
  // STEP 3: Clone the geometry (we don't want to modify the input)
  // ============================================================================

  auto result = std::make_unique<core::GeometryContainer>(container->clone());

  // ============================================================================
  // STEP 4: Ensure required attributes exist
  // ============================================================================

  result->ensure_position_attribute();

  // ============================================================================
  // STEP 5: Get typed attribute storage (FAST!)
  // ============================================================================
  //
  // Method 1: Use standard attribute helpers (recommended)
  auto *positions = result->positions();

  if (!positions) {
    set_error("Input geometry has no position attribute");
    return nullptr;
  }

  // Method 2: Manual typed access (if needed for non-standard attributes)
  // auto* positions = result->get_point_attribute_typed<core::Vec3f>("P");

  // ============================================================================
  // STEP 6: Calculate scale origin
  // ============================================================================

  core::Vec3f origin(0.0F, 0.0F, 0.0F);

  if (!scale_from_origin_) {
    // Compute centroid
    core::Vec3f sum(0.0F, 0.0F, 0.0F);
    for (size_t i = 0; i < positions->size(); ++i) {
      sum += (*positions)[i];
    }
    if (positions->size() > 0) {
      origin = sum / static_cast<float>(positions->size());
    }
  }

  // ============================================================================
  // STEP 7: Process point positions (SoA = cache-friendly iteration!)
  // ============================================================================

  core::Vec3f scale_vec(scale_x_, scale_y_, scale_z_);

  // Get writable span for zero-cost iteration
  auto pos_span = positions->values_writable();

  for (auto &pos : pos_span) {
    // Translate to origin, scale, translate back
    core::Vec3f local = pos - origin;
    local = local.cwiseProduct(scale_vec); // Element-wise multiply
    pos = local + origin;
  }

  // Alternative: Direct indexing (also fast with SoA)
  // for (size_t i = 0; i < positions->size(); ++i) {
  //     core::Vec3f local = (*positions)[i] - origin;
  //     local = local.cwiseProduct(scale_vec);
  //     (*positions)[i] = local + origin;
  // }

  // ============================================================================
  // STEP 8: Process normals if present (optional attribute)
  // ============================================================================

  if (normalize_normals_ &&
      result->has_vertex_attribute(core::standard_attrs::N)) {
    auto *normals = result->normals();

    if (normals) {
      // When scaling non-uniformly, normals need to be adjusted
      // Scale by inverse of scale factors
      core::Vec3f inv_scale(1.0F / scale_x_, 1.0F / scale_y_, 1.0F / scale_z_);

      auto norm_span = normals->values_writable();
      for (auto &n : norm_span) {
        n = n.cwiseProduct(inv_scale);
        n.normalize(); // Renormalize
      }
    }
  }

  // ============================================================================
  // STEP 9: Add custom attributes (example)
  // ============================================================================

  // Add a detail (global) attribute to track that this was scaled
  if (!result->has_detail_attribute("was_scaled")) {
    result->add_detail_attribute("was_scaled", core::AttributeType::INT);
    result->detail_attributes().resize(1);

    auto *was_scaled = result->get_detail_attribute_typed<int>("was_scaled");
    if (was_scaled) {
      (*was_scaled)[0] = 1;
    }
  }

  // Example: Add per-point scale factor
  if (!result->has_point_attribute("scale_factor")) {
    result->add_point_attribute("scale_factor", core::AttributeType::FLOAT);

    auto *scale_factors =
        result->get_point_attribute_typed<float>("scale_factor");
    if (scale_factors) {
      // Calculate average scale for each point
      float avg_scale = (scale_x_ + scale_y_ + scale_z_) / 3.0F;
      for (size_t i = 0; i < scale_factors->size(); ++i) {
        (*scale_factors)[i] = avg_scale;
      }
    }
  }

  // ============================================================================
  // STEP 10: Validate result
  // ============================================================================

  if (!result->validate()) {
    set_error("Output geometry validation failed");
    return nullptr;
  }

  // ============================================================================
  // STEP 11: Convert back to old GeometryData (temporary bridge)
  // ============================================================================

  return convert_from_container(*result);
}

// ============================================================================
// Conversion Helpers (Temporary Bridge Code)
// ============================================================================

std::unique_ptr<core::GeometryContainer>
ScaleSOP::convert_to_container(const GeometryData &old_data) {
  auto container = std::make_unique<core::GeometryContainer>();

  // Get old mesh
  auto mesh = old_data.get_mesh();
  if (!mesh) {
    return nullptr;
  }

  const auto &vertices = mesh->vertices();
  const auto &faces = mesh->faces();

  // Set topology
  size_t point_count = vertices.rows();
  size_t face_count = faces.rows();

  container->set_point_count(point_count);

  // In old system: vertices = points (no split normals)
  // For simple migration: vertex count = sum of all face vertex counts
  size_t total_vertices = 0;
  for (int i = 0; i < faces.rows(); ++i) {
    total_vertices += faces.cols(); // Assuming triangles
  }
  container->set_vertex_count(total_vertices);

  // Build topology (simple 1:1 vertex→point mapping for now)
  size_t vert_idx = 0;
  for (int face_idx = 0; face_idx < faces.rows(); ++face_idx) {
    std::vector<int> prim_verts;

    for (int j = 0; j < faces.cols(); ++j) {
      int point_idx = faces(face_idx, j);
      container->topology().set_vertex_point(vert_idx, point_idx);
      prim_verts.push_back(static_cast<int>(vert_idx));
      ++vert_idx;
    }

    container->add_primitive(prim_verts);
  }

  // Copy positions
  container->add_point_attribute("P", core::AttributeType::VEC3F);
  auto *positions = container->positions();

  for (int i = 0; i < vertices.rows(); ++i) {
    (*positions)[i] = vertices.row(i).cast<float>();
  }

  // Copy normals if computed
  try {
    const auto &old_normals = mesh->vertex_normals();
    container->add_vertex_attribute("N", core::AttributeType::VEC3F);
    auto *normals = container->normals();

    size_t v_idx = 0;
    for (int face_idx = 0; face_idx < faces.rows(); ++face_idx) {
      for (int j = 0; j < faces.cols(); ++j) {
        int point_idx = faces(face_idx, j);
        (*normals)[v_idx] = old_normals.row(point_idx).cast<float>();
        ++v_idx;
      }
    }
  } catch (...) {
    // Normals not available, skip
  }

  // TODO: Copy other attributes from old_data

  return container;
}

std::shared_ptr<GeometryData>
ScaleSOP::convert_from_container(const core::GeometryContainer &container) {
  // Convert back to old Mesh format
  auto *positions = container.positions();
  if (!positions) {
    return nullptr;
  }

  const size_t point_count = container.point_count();
  const size_t prim_count = container.primitive_count();

  // Build vertices matrix
  Eigen::MatrixXd vertices(point_count, 3);
  for (size_t i = 0; i < point_count; ++i) {
    vertices.row(i) = (*positions)[i].cast<double>();
  }

  // Build faces matrix (assuming triangles for simplicity)
  Eigen::MatrixXi faces(prim_count, 3);
  for (size_t i = 0; i < prim_count; ++i) {
    const auto &prim_verts = container.topology().get_primitive_vertices(i);
    for (size_t j = 0; j < std::min(prim_verts.size(), size_t(3)); ++j) {
      int vert_idx = prim_verts[j];
      int point_idx = container.topology().get_vertex_point(vert_idx);
      faces(i, j) = point_idx;
    }
  }

  auto mesh =
      std::make_shared<core::Mesh>(std::move(vertices), std::move(faces));

  // NOTE: Normals will be computed on-demand by Mesh
  // We could transfer them from the container, but for simplicity we let Mesh
  // recompute

  return std::make_shared<GeometryData>(mesh);
}

} // namespace nodeflux::sop
