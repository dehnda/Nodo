#pragma once

#include "nodo/core/geometry_container.hpp"
#include <optional>

namespace nodo::processing {

/// Parameters for hole filling
struct HoleFillingParams {
  /// Minimum hole size (number of boundary edges) to fill
  /// Holes smaller than this will be ignored
  int min_hole_size = 0;

  /// Maximum hole size (number of boundary edges) to fill
  /// Holes larger than this will be ignored (0 = no limit)
  int max_hole_size = 0;

  /// Whether to refine filled regions for better quality
  bool refine_fill = true;
};

/// Hole filling using PMP library
class HoleFilling {
public:
  /// Fill holes in a geometry container
  /// @param input Input geometry with holes
  /// @param params Hole filling parameters
  /// @return Repaired geometry, or nullopt on error
  static std::optional<core::GeometryContainer>
  fill_holes(const core::GeometryContainer &input,
             const HoleFillingParams &params = HoleFillingParams{});
};

} // namespace nodo::processing
