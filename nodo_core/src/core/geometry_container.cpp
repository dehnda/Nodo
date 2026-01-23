#include "nodo/core/geometry_container.hpp"

#include "nodo/core/attribute_group.hpp"
#include "nodo/core/result.hpp"

#include <unordered_set>

namespace nodo::core {

Result<std::shared_ptr<GeometryContainer>> GeometryContainer::delete_elements(const std::string& group_name,
                                                                              ElementClass element_class,
                                                                              bool delete_orphaned_points) const {
  // Check if group exists
  if (!has_group(*this, group_name, element_class)) {
    return {"Group '" + group_name + "' does not exist on geometry"};
  }

  // Get elements to delete
  auto elements_to_delete = get_group_elements(*this, group_name, element_class);
  if (elements_to_delete.empty()) {
    // No elements to delete, return clone
    return {std::make_shared<GeometryContainer>(this->clone())};
  }

  // Convert to set for fast lookup
  std::unordered_set<size_t> delete_set(elements_to_delete.begin(), elements_to_delete.end());

  GeometryContainer result;

  if (element_class == ElementClass::PRIMITIVE) {
    // Delete primitives
    return delete_primitives(delete_set, delete_orphaned_points);
  } else if (element_class == ElementClass::POINT) {
    // Delete points (and primitives that reference them)
    return delete_points(delete_set);
  }

  return {"Invalid element class"};
}

Result<std::shared_ptr<GeometryContainer>>
GeometryContainer::delete_primitives(const std::unordered_set<size_t>& delete_set, bool delete_orphaned_points) const {
  GeometryContainer result;

  // Step 1: Copy all points first
  result.set_point_count(point_count());

  // Copy all point attributes
  for (const auto& attr_name : get_point_attribute_names()) {
    auto* src_storage = get_point_attribute(attr_name);
    if (!src_storage) {
      continue;
    }

    const auto type = src_storage->descriptor().type();
    result.add_point_attribute(attr_name, type, src_storage->descriptor().interpolation());

    // Copy based on type
    switch (type) {
      case AttributeType::INT: {
        auto* src = get_point_attribute_typed<int>(attr_name);
        auto* dst = result.get_point_attribute_typed<int>(attr_name);
        for (size_t i = 0; i < point_count(); ++i) {
          (*dst)[i] = (*src)[i];
        }
        break;
      }
      case AttributeType::FLOAT: {
        auto* src = get_point_attribute_typed<float>(attr_name);
        auto* dst = result.get_point_attribute_typed<float>(attr_name);
        for (size_t i = 0; i < point_count(); ++i) {
          (*dst)[i] = (*src)[i];
        }
        break;
      }
      case AttributeType::VEC2F: {
        auto* src = get_point_attribute_typed<Eigen::Vector2f>(attr_name);
        auto* dst = result.get_point_attribute_typed<Eigen::Vector2f>(attr_name);
        for (size_t i = 0; i < point_count(); ++i) {
          (*dst)[i] = (*src)[i];
        }
        break;
      }
      case AttributeType::VEC3F: {
        auto* src = get_point_attribute_typed<Eigen::Vector3f>(attr_name);
        auto* dst = result.get_point_attribute_typed<Eigen::Vector3f>(attr_name);
        for (size_t i = 0; i < point_count(); ++i) {
          (*dst)[i] = (*src)[i];
        }
        break;
      }
      case AttributeType::VEC4F: {
        auto* src = get_point_attribute_typed<Eigen::Vector4f>(attr_name);
        auto* dst = result.get_point_attribute_typed<Eigen::Vector4f>(attr_name);
        for (size_t i = 0; i < point_count(); ++i) {
          (*dst)[i] = (*src)[i];
        }
        break;
      }
      default:
        break;
    }
  }

  // Step 2: Copy primitives that are NOT in delete set
  size_t vertex_offset = 0;

  for (size_t prim_idx = 0; prim_idx < primitive_count(); ++prim_idx) {
    if (delete_set.find(prim_idx) == delete_set.end()) {
      // Keep this primitive
      const auto& src_verts = topology_.get_primitive_vertices(prim_idx);

      // Set up vertices for this primitive
      result.set_vertex_count(vertex_offset + src_verts.size());

      std::vector<int> new_verts;
      for (int src_vert_idx : src_verts) {
        int point_idx = topology_.get_vertex_point(src_vert_idx);
        result.topology().set_vertex_point(vertex_offset, point_idx);
        new_verts.push_back(static_cast<int>(vertex_offset));
        vertex_offset++;
      }

      result.add_primitive(new_verts);
    }
  }

  // Step 3: Optionally remove orphaned points
  if (delete_orphaned_points) {
    return remove_orphaned_points(result);
  }

  return {std::make_shared<GeometryContainer>(result.clone())};
}

Result<std::shared_ptr<GeometryContainer>>
GeometryContainer::delete_points(const std::unordered_set<size_t>& delete_set) const {
  GeometryContainer result;

  // Step 1: Build point index mapping (old -> new)
  std::vector<int> point_remap(point_count(), -1);
  size_t new_point_idx = 0;

  for (size_t old_pt_idx = 0; old_pt_idx < point_count(); ++old_pt_idx) {
    if (delete_set.find(old_pt_idx) == delete_set.end()) {
      point_remap[old_pt_idx] = static_cast<int>(new_point_idx);
      new_point_idx++;
    }
  }

  // Step 2: Copy all point attributes with remapping
  result.set_point_count(new_point_idx);

  for (const auto& attr_name : get_point_attribute_names()) {
    auto* src_storage = get_point_attribute(attr_name);
    if (!src_storage) {
      continue;
    }

    const auto type = src_storage->descriptor().type();
    result.add_point_attribute(attr_name, type, src_storage->descriptor().interpolation());

    // Copy with remapping based on type
    switch (type) {
      case AttributeType::INT: {
        auto* src = get_point_attribute_typed<int>(attr_name);
        auto* dst = result.get_point_attribute_typed<int>(attr_name);
        for (size_t old_idx = 0; old_idx < point_count(); ++old_idx) {
          if (point_remap[old_idx] >= 0) {
            (*dst)[point_remap[old_idx]] = (*src)[old_idx];
          }
        }
        break;
      }
      case AttributeType::FLOAT: {
        auto* src = get_point_attribute_typed<float>(attr_name);
        auto* dst = result.get_point_attribute_typed<float>(attr_name);
        for (size_t old_idx = 0; old_idx < point_count(); ++old_idx) {
          if (point_remap[old_idx] >= 0) {
            (*dst)[point_remap[old_idx]] = (*src)[old_idx];
          }
        }
        break;
      }
      case AttributeType::VEC2F: {
        auto* src = get_point_attribute_typed<Eigen::Vector2f>(attr_name);
        auto* dst = result.get_point_attribute_typed<Eigen::Vector2f>(attr_name);
        for (size_t old_idx = 0; old_idx < point_count(); ++old_idx) {
          if (point_remap[old_idx] >= 0) {
            (*dst)[point_remap[old_idx]] = (*src)[old_idx];
          }
        }
        break;
      }
      case AttributeType::VEC3F: {
        auto* src = get_point_attribute_typed<Eigen::Vector3f>(attr_name);
        auto* dst = result.get_point_attribute_typed<Eigen::Vector3f>(attr_name);
        for (size_t old_idx = 0; old_idx < point_count(); ++old_idx) {
          if (point_remap[old_idx] >= 0) {
            (*dst)[point_remap[old_idx]] = (*src)[old_idx];
          }
        }
        break;
      }
      case AttributeType::VEC4F: {
        auto* src = get_point_attribute_typed<Eigen::Vector4f>(attr_name);
        auto* dst = result.get_point_attribute_typed<Eigen::Vector4f>(attr_name);
        for (size_t old_idx = 0; old_idx < point_count(); ++old_idx) {
          if (point_remap[old_idx] >= 0) {
            (*dst)[point_remap[old_idx]] = (*src)[old_idx];
          }
        }
        break;
      }
      default:
        break;
    }
  }

  // Step 3: Copy primitives, skipping those with deleted points
  size_t vertex_offset = 0;

  for (size_t prim_idx = 0; prim_idx < primitive_count(); ++prim_idx) {
    const auto& src_verts = topology_.get_primitive_vertices(prim_idx);

    // Check if any vertex references a deleted point
    bool has_deleted_point = false;
    for (int src_vert_idx : src_verts) {
      int old_point_idx = topology_.get_vertex_point(src_vert_idx);
      if (point_remap[old_point_idx] < 0) {
        has_deleted_point = true;
        break;
      }
    }

    if (!has_deleted_point) {
      // Keep this primitive with remapped points
      result.set_vertex_count(vertex_offset + src_verts.size());

      std::vector<int> new_verts;
      for (int src_vert_idx : src_verts) {
        int old_point_idx = topology_.get_vertex_point(src_vert_idx);
        int new_point_idx = point_remap[old_point_idx];

        result.topology().set_vertex_point(vertex_offset, new_point_idx);
        new_verts.push_back(static_cast<int>(vertex_offset));
        vertex_offset++;
      }

      result.add_primitive(new_verts);
    }
  }

  return {std::make_shared<GeometryContainer>(result.clone())};
}

Result<std::shared_ptr<GeometryContainer>>
GeometryContainer::remove_orphaned_points(const GeometryContainer& input) const {
  // Build set of used points
  std::unordered_set<size_t> used_points;
  for (size_t prim_idx = 0; prim_idx < input.primitive_count(); ++prim_idx) {
    const auto& vert_indices = input.topology().get_primitive_vertices(prim_idx);
    for (int vert_idx : vert_indices) {
      int point_idx = input.topology().get_vertex_point(vert_idx);
      used_points.insert(point_idx);
    }
  }

  // If all points are used, return input as-is
  if (used_points.size() == input.point_count()) {
    return {std::make_shared<GeometryContainer>(input.clone())};
  }

  // Build point remapping
  std::vector<int> point_remap(input.point_count(), -1);
  size_t new_point_idx = 0;

  for (size_t old_pt_idx = 0; old_pt_idx < input.point_count(); ++old_pt_idx) {
    if (used_points.find(old_pt_idx) != used_points.end()) {
      point_remap[old_pt_idx] = static_cast<int>(new_point_idx);
      new_point_idx++;
    }
  }

  // Create result with only used points
  GeometryContainer result;
  result.set_point_count(new_point_idx);

  if (input.has_point_attribute("P")) {
    auto src_P = input.get_point_attribute_typed<Eigen::Vector3f>("P");
    result.add_point_attribute("P", AttributeType::VEC3F);
    auto dst_P = result.get_point_attribute_typed<Eigen::Vector3f>("P");

    for (size_t old_idx = 0; old_idx < input.point_count(); ++old_idx) {
      if (point_remap[old_idx] >= 0) {
        (*dst_P)[point_remap[old_idx]] = (*src_P)[old_idx];
      }
    }
  }

  // Copy primitives with remapped points
  size_t vertex_offset = 0;

  for (size_t prim_idx = 0; prim_idx < input.primitive_count(); ++prim_idx) {
    const auto& src_verts = input.topology().get_primitive_vertices(prim_idx);

    result.set_vertex_count(vertex_offset + src_verts.size());

    std::vector<int> new_verts;
    for (int src_vert_idx : src_verts) {
      int old_point_idx = input.topology().get_vertex_point(src_vert_idx);
      int new_point_idx = point_remap[old_point_idx];

      result.topology().set_vertex_point(vertex_offset, new_point_idx);
      new_verts.push_back(static_cast<int>(vertex_offset));
      vertex_offset++;
    }

    result.add_primitive(new_verts);
  }

  return {std::make_shared<GeometryContainer>(result.clone())};
}

} // namespace nodo::core
