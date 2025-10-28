#include "nodo/sop/laplacian_sop.hpp"
#include "nodo/core/attribute_types.hpp"
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace attrs = nodo::core::standard_attrs;

namespace nodo::sop {

namespace {

/**
 * @brief Build point-to-point connectivity graph from topology
 *
 * For each point, finds all directly connected neighbor points by
 * analyzing edges in the primitives.
 */
void build_point_connectivity(
    const core::GeometryContainer &container,
    std::unordered_map<int, std::unordered_set<int>> &point_neighbors) {

  const auto &topology = container.topology();
  const size_t num_prims = topology.primitive_count();

  // Loop through all primitives
  for (size_t prim_idx = 0; prim_idx < num_prims; ++prim_idx) {
    const auto &prim_vertices = topology.get_primitive_vertices(prim_idx);
    const size_t num_verts = prim_vertices.size();

    if (num_verts < 2) {
      continue; // Skip degenerate primitives
    }

    // For each edge in the primitive (consecutive vertex pairs)
    for (size_t i = 0; i < num_verts; ++i) {
      const size_t next_i = (i + 1) % num_verts;

      const int vert_a = prim_vertices[i];
      const int vert_b = prim_vertices[next_i];

      // Map vertices back to points
      const int point_a = topology.get_vertex_point(vert_a);
      const int point_b = topology.get_vertex_point(vert_b);

      // Add bidirectional connectivity
      point_neighbors[point_a].insert(point_b);
      point_neighbors[point_b].insert(point_a);
    }
  }
}

/**
 * @brief Perform one iteration of Laplacian smoothing
 */
void smooth_iteration(
    core::GeometryContainer &container,
    core::AttributeStorage<core::Vec3f> *P_storage,
    const std::unordered_map<int, std::unordered_set<int>> &point_neighbors,
    float lambda, int method) {

  const size_t num_points = container.point_count();

  // Store new positions (don't modify in place to avoid feedback)
  std::vector<core::Vec3f> new_positions(num_points);

  // Copy current positions
  for (size_t i = 0; i < num_points; ++i) {
    new_positions[i] = (*P_storage)[i];
  }

  // For each point, compute Laplacian and update position
  for (size_t point_idx = 0; point_idx < num_points; ++point_idx) {
    auto neighbors_it = point_neighbors.find(static_cast<int>(point_idx));

    // Skip points with no neighbors
    if (neighbors_it == point_neighbors.end() || neighbors_it->second.empty()) {
      continue;
    }

    const auto &neighbors = neighbors_it->second;
    const core::Vec3f &current_pos = (*P_storage)[point_idx];

    // Compute average neighbor position (uniform weighting)
    core::Vec3f avg_neighbor_pos(0.0F, 0.0F, 0.0F);
    for (int neighbor_idx : neighbors) {
      avg_neighbor_pos += (*P_storage)[neighbor_idx];
    }
    avg_neighbor_pos /= static_cast<float>(neighbors.size());

    // Laplacian vector = average - current
    const core::Vec3f laplacian = avg_neighbor_pos - current_pos;

    // Update position based on method
    if (method == 0) {
      // Uniform Laplacian
      new_positions[point_idx] = current_pos + lambda * laplacian;
    } else if (method == 2) {
      // Taubin smoothing (alternate shrink/expand)
      // For now, just use uniform (TODO: implement properly)
      new_positions[point_idx] = current_pos + lambda * laplacian;
    } else {
      // Cotangent weights (TODO: implement)
      new_positions[point_idx] = current_pos + lambda * laplacian;
    }
  }

  // Apply new positions
  for (size_t i = 0; i < num_points; ++i) {
    P_storage->set(i, new_positions[i]);
  }
}

} // anonymous namespace

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
  auto input = get_input_data(0);
  if (!input) {
    return nullptr;
  }

  // Clone input for modification
  auto output = std::make_shared<core::GeometryContainer>(input->clone());

  // Get parameters
  const int iterations = get_parameter<int>("iterations", 5);
  const float lambda = get_parameter<float>("lambda", 0.5F);
  const int method = get_parameter<int>("method", 0);

  // Get P attribute (point positions)
  auto *P_storage = output->get_point_attribute_typed<core::Vec3f>("P");
  if (P_storage == nullptr) {
    std::cerr << "LaplacianSOP: No P attribute found\n";
    return output;
  }

  // Check if we have any points to smooth
  if (output->point_count() == 0) {
    return output;
  }

  // Build point-to-point connectivity graph
  std::unordered_map<int, std::unordered_set<int>> point_neighbors;
  build_point_connectivity(*output, point_neighbors);

  // Perform smoothing iterations
  for (int iter = 0; iter < iterations; ++iter) {
    smooth_iteration(*output, P_storage, point_neighbors, lambda, method);
  }

  // If we have vertex or point normals, recompute them after smoothing
  // (they may be pointing in wrong directions now)
  auto *N_vertex = output->get_vertex_attribute_typed<core::Vec3f>("N");
  auto *N_point = output->get_point_attribute_typed<core::Vec3f>("N");

  if (N_vertex != nullptr || N_point != nullptr) {
    // TODO: Optionally recompute normals here
    // For now, we'll leave them as-is and let downstream nodes handle it
  }

  return output;
}

} // namespace nodo::sop
