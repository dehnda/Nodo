#include "nodeflux/sop/boolean_sop.hpp"
#include "nodeflux/geometry/boolean_ops.hpp"
#include <iostream>

namespace nodeflux::sop {

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

std::shared_ptr<GeometryData> BooleanSOP::execute_boolean() {
  if (!mesh_a_ || !mesh_b_) {
    std::cout << "BooleanSOP '" << name_
              << "': Missing input meshes - A: " << (mesh_a_ ? "✓" : "✗")
              << ", B: " << (mesh_b_ ? "✓" : "✗") << "\n";
    return nullptr;
  }

  std::cout << "BooleanSOP '" << name_ << "': Executing with mesh A ("
            << mesh_a_->vertex_count() << " verts) and mesh B ("
            << mesh_b_->vertex_count() << " verts)\n";

  try {
    std::optional<core::Mesh> result_mesh;

    switch (operation_) {
    case OperationType::UNION:
      result_mesh = geometry::BooleanOps::union_meshes(*mesh_a_, *mesh_b_);
      break;

    case OperationType::INTERSECTION:
      result_mesh = geometry::BooleanOps::intersect_meshes(*mesh_a_, *mesh_b_);
      break;

    case OperationType::DIFFERENCE:
      result_mesh = geometry::BooleanOps::difference_meshes(*mesh_a_, *mesh_b_);
      break;

    default:
      return nullptr;
    }

    if (!result_mesh.has_value()) {
      std::cerr << "BooleanSOP '" << name_ << "': Boolean operation failed: "
                << geometry::BooleanOps::last_error().message << "\n";
      return nullptr;
    }

    return std::make_shared<GeometryData>(
        std::make_shared<core::Mesh>(std::move(result_mesh.value())));

  } catch (const std::exception &exception) {
    std::cout << "BooleanSOP '" << name_ << "': Exception: " << exception.what()
              << "\n";
    return nullptr;
  }
}

std::shared_ptr<GeometryData> BooleanSOP::execute() {

  std::cout << "BooleanSOP '" << name_ << "': Computing "
            << operation_to_string(operation_) << " operation...\n";

  auto result = execute_boolean();
  if (!result) {
    std::cerr << "BooleanSOP '" << name_ << "': Operation failed\n";
    return nullptr;
  }

  return result;
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
