#pragma once

#include "nodo/core/geometry_container.hpp"
#include "nodo/sop/sop_node.hpp"

#include <memory>
#include <string>

namespace nodo::sop {

/**
 * @brief Decimation SOP - Reduce mesh complexity
 *
 * Reduces the number of vertices and faces in a mesh while
 * preserving overall shape using error quadrics method.
 * Only works with triangular meshes.
 */
class DecimationSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  /**
   * @brief Construct decimation operator with name
   * @param name Unique identifier for this SOP instance
   */
  explicit DecimationSOP(const std::string& name = "decimate");

  /**
   * @brief Set target reduction percentage
   * @param percentage Target as percentage of original (0.0 to 1.0)
   */
  void set_target_percentage(float percentage) { set_parameter("target_percentage", percentage); }

  /**
   * @brief Set target vertex count
   * @param count Target number of vertices
   */
  void set_target_vertex_count(int count) { set_parameter("target_vertex_count", count); }

  /**
   * @brief Set reduction method
   * @param use_count If true, use target_vertex_count; if false, use
   * target_percentage
   */
  void set_use_vertex_count(bool use_count) { set_parameter("use_vertex_count", use_count); }

  /**
   * @brief Set aspect ratio for quality control
   * @param ratio Aspect ratio (0.0 to 10.0)
   */
  void set_aspect_ratio(float ratio) { set_parameter("aspect_ratio", ratio); }

  /**
   * @brief Set whether to preserve topology
   * @param preserve If true, no holes will be created
   */
  void set_preserve_topology(bool preserve) { set_parameter("preserve_topology", preserve); }

  /**
   * @brief Set whether to preserve boundaries
   * @param preserve If true, boundary edges will not be collapsed
   */
  void set_preserve_boundaries(bool preserve) { set_parameter("preserve_boundaries", preserve); }

  // Node metadata
  std::string get_category() const { return "Modify"; }
  std::string get_description() const { return "Reduce mesh complexity while preserving shape"; }

protected:
  core::Result<std::shared_ptr<core::GeometryContainer>> execute() override;

private:
  void initialize_parameters();
};

} // namespace nodo::sop
