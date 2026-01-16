#include "nodo/processing/hole_filling.hpp"

#include "nodo/core/math.hpp"
#include "nodo/processing/pmp_converter.hpp"
#include "nodo/processing/processing_common.hpp"

#include <set>

#include <fmt/core.h>
#include <pmp/algorithms/hole_filling.h>

namespace nodo::processing {

std::optional<core::GeometryContainer> HoleFilling::fill_holes(const core::GeometryContainer& input,
                                                               const HoleFillingParams& params) {
  // Validate input
  auto validation_error = detail::PMPConverter::validate_for_pmp(input);
  if (!validation_error.empty()) {
    fmt::print("HoleFilling: Validation failed: {}\n", validation_error);
    return std::nullopt;
  }

  // Convert to PMP
  auto pmp_mesh = detail::PMPConverter::to_pmp(input);

  fmt::print("HoleFilling: Input mesh has {} vertices, {} faces\n", pmp_mesh.n_vertices(), pmp_mesh.n_faces());

  // Find all boundary halfedges (holes)
  std::vector<pmp::Halfedge> boundary_halfedges;
  std::set<int> visited; // Use int indices instead of Halfedge directly

  for (auto h : pmp_mesh.halfedges()) {
    if (pmp_mesh.is_boundary(h) && visited.find(h.idx()) == visited.end()) {
      // Found a new boundary loop - trace it
      auto current = h;
      int hole_size = 0;

      do {
        visited.insert(current.idx());
        current = pmp_mesh.next_halfedge(current);
        hole_size++;
      } while (current != h);

      // Check if this hole should be filled based on size constraints
      bool should_fill = true;

      if (params.min_hole_size > 0 && hole_size < params.min_hole_size) {
        should_fill = false;
        fmt::print("  Skipping small hole (size {})\n", hole_size);
      }

      if (params.max_hole_size > 0 && hole_size > params.max_hole_size) {
        should_fill = false;
        fmt::print("  Skipping large hole (size {})\n", hole_size);
      }

      if (should_fill) {
        boundary_halfedges.push_back(h);
        fmt::print("  Found hole with {} boundary edges\n", hole_size);
      }
    }
  }

  if (boundary_halfedges.empty()) {
    fmt::print("HoleFilling: No holes found to fill\n");
    return detail::PMPConverter::from_pmp(pmp_mesh);
  }

  fmt::print("HoleFilling: Filling {} holes\n", boundary_halfedges.size());

  // Fill each hole
  int filled_count = 0;
  for (auto h : boundary_halfedges) {
    try {
      pmp::fill_hole(pmp_mesh, h);
      filled_count++;
    } catch (const std::exception& e) {
      fmt::print("  Warning: Failed to fill hole: {}\n", e.what());
    }
  }

  fmt::print("HoleFilling: Successfully filled {} holes\n", filled_count);
  fmt::print("HoleFilling: Output mesh has {} vertices, {} faces\n", pmp_mesh.n_vertices(), pmp_mesh.n_faces());

  // Convert back to Nodo format
  return detail::PMPConverter::from_pmp(pmp_mesh);
}

} // namespace nodo::processing
