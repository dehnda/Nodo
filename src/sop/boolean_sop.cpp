#include "nodeflux/sop/boolean_sop.hpp"
#include "nodeflux/core/error.hpp"
#include "nodeflux/geometry/boolean_ops.hpp"
#include <chrono>
#include <iostream>

namespace nodeflux::sop {

BooleanSOP::BooleanSOP(std::string name) : name_(std::move(name)) {}

void BooleanSOP::set_operation(OperationType operation) {
  if (operation_ != operation) {
    operation_ = operation;
    mark_dirty();
  }
}

void BooleanSOP::set_mesh_a(std::shared_ptr<core::Mesh> mesh) {
  if (mesh_a_ != mesh) {
    mesh_a_ = std::move(mesh);
    mark_dirty();
  }
}

void BooleanSOP::set_mesh_b(std::shared_ptr<core::Mesh> mesh) {
  if (mesh_b_ != mesh) {
    mesh_b_ = std::move(mesh);
    mark_dirty();
  }
}

std::optional<core::Mesh> BooleanSOP::execute() {
  if (!mesh_a_ || !mesh_b_) {
    std::cout << "BooleanSOP '" << name_
              << "': Missing input meshes - A: " << (mesh_a_ ? "✓" : "✗")
              << ", B: " << (mesh_b_ ? "✓" : "✗") << "\n";
    return std::nullopt;
  }

  std::cout << "BooleanSOP '" << name_ << "': Executing with mesh A ("
            << mesh_a_->vertex_count() << " verts) and mesh B ("
            << mesh_b_->vertex_count() << " verts)\n";

  try {
    switch (operation_) {
    case OperationType::UNION:
      return geometry::BooleanOps::union_meshes(*mesh_a_, *mesh_b_);

    case OperationType::INTERSECTION:
      return geometry::BooleanOps::intersect_meshes(*mesh_a_, *mesh_b_);

    case OperationType::DIFFERENCE:
      return geometry::BooleanOps::difference_meshes(*mesh_a_, *mesh_b_);

    default:
      return std::nullopt;
    }
  } catch (const std::exception &exception) {
    std::cout << "BooleanSOP '" << name_ << "': Exception: " << exception.what()
              << "\n";
    return std::nullopt;
  }
}

std::shared_ptr<core::Mesh> BooleanSOP::cook() {
  if (!is_dirty_ && cached_result_) {
    std::cout << "BooleanSOP '" << name_ << "': Using cached result\n";
    return cached_result_;
  }

  std::cout << "BooleanSOP '" << name_ << "': Computing "
            << operation_to_string(operation_) << " operation...\n";

  auto start_time = std::chrono::steady_clock::now();

  auto result = execute();
  if (!result) {
    std::cerr << "BooleanSOP '" << name_ << "': Operation failed\n";
    return nullptr;
  }

  cached_result_ = std::make_shared<core::Mesh>(std::move(*result));
  is_dirty_ = false;

  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);

  std::cout << "BooleanSOP '" << name_ << "': Completed in " << duration.count()
            << "ms\n";
  std::cout << "  Result: " << cached_result_->vertices().rows()
            << " vertices, " << cached_result_->faces().rows() << " faces\n";

  return cached_result_;
}

std::string BooleanSOP::operation_to_string(OperationType operation) {
  switch (operation) {
  case OperationType::UNION:
    return "Union";
  case OperationType::INTERSECTION:
    return "Intersection";
  case OperationType::DIFFERENCE:
    return "Difference";
  default:
    return "Unknown";
  }
}

} // namespace nodeflux::sop
