#pragma once

#include "nodeflux/core/mesh.hpp"
#include "nodeflux/geometry/boolean_ops.hpp"
#include <memory>
#include <optional>
#include <string>

namespace nodeflux::sop {

/**
 * @brief Boolean SOP - Performs boolean operations between geometries
 *
 * Supports union, intersection, and difference operations using Manifold
 * (Apache 2.0) - fully compatible with commercial use.
 */
class BooleanSOP {
public:
  enum class OperationType {
    UNION,        ///< Combine meshes (A ∪ B)
    INTERSECTION, ///< Keep only overlapping parts (A ∩ B)
    DIFFERENCE    ///< Subtract B from A (A - B)
  };

private:
  std::string name_;
  OperationType operation_ = OperationType::UNION;
  bool is_dirty_ = true;
  std::shared_ptr<core::Mesh> cached_result_;

  // Input meshes
  std::shared_ptr<core::Mesh> mesh_a_;
  std::shared_ptr<core::Mesh> mesh_b_;

public:
  /**
   * @brief Construct a new Boolean SOP
   * @param name Node name for debugging
   */
  explicit BooleanSOP(std::string name);

  /**
   * @brief Set the boolean operation type
   * @param operation The operation to perform
   */
  void set_operation(OperationType operation);

  /**
   * @brief Get the current operation type
   * @return The current operation
   */
  OperationType get_operation() const { return operation_; }

  /**
   * @brief Set the first input mesh (A)
   * @param mesh The first mesh
   */
  void set_mesh_a(std::shared_ptr<core::Mesh> mesh);

  /**
   * @brief Set the second input mesh (B)
   * @param mesh The second mesh
   */
  void set_mesh_b(std::shared_ptr<core::Mesh> mesh);

  /**
   * @brief Execute the boolean operation
   * @return Result mesh or nullopt on failure
   */
  std::optional<core::Mesh> execute();

  /**
   * @brief Get cached result or compute if dirty
   * @return Result mesh or nullopt on failure
   */
  std::shared_ptr<core::Mesh> cook();

  /**
   * @brief Mark node as needing recomputation
   */
  void mark_dirty() {
    is_dirty_ = true;
    cached_result_.reset();
  }

  /**
   * @brief Get node name
   * @return The node name
   */
  const std::string &get_name() const { return name_; }

  /**
   * @brief Convert operation type to string
   * @param operation The operation type
   * @return String representation
   */
  static std::string operation_to_string(OperationType operation);
};

} // namespace nodeflux::sop
