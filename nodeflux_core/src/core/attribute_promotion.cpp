#include "nodeflux/core/attribute_promotion.hpp"
#include "nodeflux/core/standard_attributes.hpp"
#include <unordered_map>

namespace nodeflux::core {

// Helper to get output name (use input name if output not specified)
static std::string get_output_name(std::string_view attr_name,
                                   std::string_view output_name) {
  return output_name.empty() ? std::string(attr_name)
                             : std::string(output_name);
}

// ============================================================================
// Point ↔ Vertex
// ============================================================================

bool promote_point_to_vertex(GeometryContainer &container,
                             std::string_view attr_name,
                             std::string_view output_name) {
  const auto &topology = container.topology();
  const size_t vertex_count = topology.vertex_count();

  if (vertex_count == 0) {
    return false;
  }

  // Get source point attribute
  auto *src_storage = container.get_point_attribute(attr_name);
  if (!src_storage) {
    return false;
  }

  const auto &desc = src_storage->descriptor();
  const std::string out_name = get_output_name(attr_name, output_name);

  // Handle different types
  if (desc.type() == AttributeType::FLOAT) {
    return promote_point_to_vertex_typed<float>(container, attr_name, out_name);
  } else if (desc.type() == AttributeType::VEC3F) {
    return promote_point_to_vertex_typed<Vec3f>(container, attr_name, out_name);
  } else if (desc.type() == AttributeType::INT) {
    return promote_point_to_vertex_typed<int>(container, attr_name, out_name);
  }

  return false;
}

template <typename T>
bool promote_point_to_vertex_typed(GeometryContainer &container,
                                   std::string_view attr_name,
                                   std::string_view output_name) {
  const auto &topology = container.topology();
  const size_t vertex_count = topology.vertex_count();

  auto *src = container.get_point_attribute_typed<T>(attr_name);
  if (!src) {
    return false;
  }

  const std::string out_name = get_output_name(attr_name, output_name);
  const auto &desc = src->descriptor();

  if (!container.add_vertex_attribute(out_name, desc.type(),
                                      desc.interpolation())) {
    return false;
  }

  auto *dst = container.get_vertex_attribute_typed<T>(out_name);
  if (!dst) {
    return false;
  }

  auto src_span = src->values();
  auto dst_span = dst->values_writable();

  // Copy point values to vertices
  for (size_t v = 0; v < vertex_count; ++v) {
    int point_idx = topology.get_vertex_point(static_cast<int>(v));
    if (point_idx >= 0 && point_idx < static_cast<int>(src->size())) {
      dst_span[v] = src_span[point_idx];
    }
  }

  return true;
}

bool demote_vertex_to_point(GeometryContainer &container,
                            std::string_view attr_name,
                            std::string_view output_name) {
  const auto &topology = container.topology();
  const size_t point_count = topology.point_count();
  const size_t vertex_count = topology.vertex_count();

  if (point_count == 0 || vertex_count == 0) {
    return false;
  }

  // Get source vertex attribute
  auto *src_storage = container.get_vertex_attribute(attr_name);
  if (!src_storage) {
    return false;
  }

  const auto &desc = src_storage->descriptor();

  // This operation requires type-specific handling for averaging
  // For now, we'll handle common types: float, Vec3f

  if (desc.type() == AttributeType::FLOAT) {
    return demote_vertex_to_point_typed<float>(container, attr_name,
                                               output_name);
  } else if (desc.type() == AttributeType::VEC3F) {
    return demote_vertex_to_point_typed<Vec3f>(container, attr_name,
                                               output_name);
  } else if (desc.type() == AttributeType::INT) {
    return demote_vertex_to_point_typed<int>(container, attr_name, output_name);
  }

  // Unsupported type for averaging
  return false;
}

template <typename T>
bool demote_vertex_to_point_typed(GeometryContainer &container,
                                  std::string_view attr_name,
                                  std::string_view output_name) {
  const auto &topology = container.topology();
  const size_t point_count = topology.point_count();
  const size_t vertex_count = topology.vertex_count();

  // Get typed source
  auto *src = container.get_vertex_attribute_typed<T>(attr_name);
  if (!src) {
    return false;
  }

  const std::string out_name = get_output_name(attr_name, output_name);

  // Create point attribute
  const auto &desc = src->descriptor();
  if (!container.add_point_attribute(out_name, desc.type(),
                                     desc.interpolation())) {
    return false;
  }

  auto *dst = container.get_point_attribute_typed<T>(out_name);
  if (!dst) {
    return false;
  }

  // Build map: point_idx → list of vertex values
  std::vector<std::vector<T>> point_values(point_count);

  auto src_span = src->values();
  for (size_t v = 0; v < vertex_count; ++v) {
    int point_idx = topology.get_vertex_point(static_cast<int>(v));
    if (point_idx >= 0 && point_idx < static_cast<int>(point_count)) {
      point_values[point_idx].push_back(src_span[v]);
    }
  }

  // Average values for each point
  auto dst_span = dst->values_writable();
  for (size_t p = 0; p < point_count; ++p) {
    if (!point_values[p].empty()) {
      dst_span[p] = detail::average_values(point_values[p]);
    }
  }

  return true;
}

// ============================================================================
// Point ↔ Primitive
// ============================================================================

bool promote_point_to_primitive(GeometryContainer &container,
                                std::string_view attr_name,
                                std::string_view output_name) {
  const auto &topology = container.topology();
  const size_t prim_count = topology.primitive_count();

  if (prim_count == 0) {
    return false;
  }

  auto *src_storage = container.get_point_attribute(attr_name);
  if (!src_storage) {
    return false;
  }

  const auto &desc = src_storage->descriptor();

  // Type-specific handling for averaging
  if (desc.type() == AttributeType::FLOAT) {
    return promote_point_to_primitive_typed<float>(container, attr_name,
                                                   output_name);
  } else if (desc.type() == AttributeType::VEC3F) {
    return promote_point_to_primitive_typed<Vec3f>(container, attr_name,
                                                   output_name);
  } else if (desc.type() == AttributeType::INT) {
    return promote_point_to_primitive_typed<int>(container, attr_name,
                                                 output_name);
  }

  return false;
}

template <typename T>
bool promote_point_to_primitive_typed(GeometryContainer &container,
                                      std::string_view attr_name,
                                      std::string_view output_name) {
  const auto &topology = container.topology();
  const size_t prim_count = topology.primitive_count();

  auto *src = container.get_point_attribute_typed<T>(attr_name);
  if (!src) {
    return false;
  }

  const std::string out_name = get_output_name(attr_name, output_name);
  const auto &desc = src->descriptor();

  if (!container.add_primitive_attribute(out_name, desc.type(),
                                         desc.interpolation())) {
    return false;
  }

  auto *dst = container.get_primitive_attribute_typed<T>(out_name);
  if (!dst) {
    return false;
  }

  auto src_span = src->values();
  auto dst_span = dst->values_writable();

  // For each primitive, average its points' attribute values
  for (size_t prim_idx = 0; prim_idx < prim_count; ++prim_idx) {
    const auto &vert_indices = topology.get_primitive_vertices(prim_idx);
    std::vector<T> values;
    values.reserve(vert_indices.size());

    for (int v_idx : vert_indices) {
      int p_idx = topology.get_vertex_point(v_idx);
      if (p_idx >= 0 && p_idx < static_cast<int>(src->size())) {
        values.push_back(src_span[p_idx]);
      }
    }

    if (!values.empty()) {
      dst_span[prim_idx] = detail::average_values(values);
    }
  }

  return true;
}

bool demote_primitive_to_point(GeometryContainer &container,
                               std::string_view attr_name,
                               std::string_view output_name) {
  const auto &topology = container.topology();
  const size_t point_count = topology.point_count();
  const size_t prim_count = topology.primitive_count();

  if (point_count == 0 || prim_count == 0) {
    return false;
  }

  auto *src_storage = container.get_primitive_attribute(attr_name);
  if (!src_storage) {
    return false;
  }

  const auto &desc = src_storage->descriptor();

  if (desc.type() == AttributeType::FLOAT) {
    return demote_primitive_to_point_typed<float>(container, attr_name,
                                                  output_name);
  } else if (desc.type() == AttributeType::VEC3F) {
    return demote_primitive_to_point_typed<Vec3f>(container, attr_name,
                                                  output_name);
  } else if (desc.type() == AttributeType::INT) {
    return demote_primitive_to_point_typed<int>(container, attr_name,
                                                output_name);
  }

  return false;
}

template <typename T>
bool demote_primitive_to_point_typed(GeometryContainer &container,
                                     std::string_view attr_name,
                                     std::string_view output_name) {
  const auto &topology = container.topology();
  const size_t point_count = topology.point_count();
  const size_t prim_count = topology.primitive_count();

  auto *src = container.get_primitive_attribute_typed<T>(attr_name);
  if (!src) {
    return false;
  }

  const std::string out_name = get_output_name(attr_name, output_name);
  const auto &desc = src->descriptor();

  if (!container.add_point_attribute(out_name, desc.type(),
                                     desc.interpolation())) {
    return false;
  }

  auto *dst = container.get_point_attribute_typed<T>(out_name);
  if (!dst) {
    return false;
  }

  // Build map: point_idx → list of primitive values
  std::vector<std::vector<T>> point_values(point_count);

  auto src_span = src->values();

  for (size_t prim_idx = 0; prim_idx < prim_count; ++prim_idx) {
    const auto &vert_indices = topology.get_primitive_vertices(prim_idx);

    for (int v_idx : vert_indices) {
      int p_idx = topology.get_vertex_point(v_idx);
      if (p_idx >= 0 && p_idx < static_cast<int>(point_count)) {
        point_values[p_idx].push_back(src_span[prim_idx]);
      }
    }
  }

  // Average values for each point
  auto dst_span = dst->values_writable();
  for (size_t p = 0; p < point_count; ++p) {
    if (!point_values[p].empty()) {
      dst_span[p] = detail::average_values(point_values[p]);
    }
  }

  return true;
}

// ============================================================================
// Vertex ↔ Primitive
// ============================================================================

bool promote_vertex_to_primitive(GeometryContainer &container,
                                 std::string_view attr_name,
                                 std::string_view output_name) {
  const auto &topology = container.topology();
  const size_t prim_count = topology.primitive_count();

  if (prim_count == 0) {
    return false;
  }

  auto *src_storage = container.get_vertex_attribute(attr_name);
  if (!src_storage) {
    return false;
  }

  const auto &desc = src_storage->descriptor();

  if (desc.type() == AttributeType::FLOAT) {
    return promote_vertex_to_primitive_typed<float>(container, attr_name,
                                                    output_name);
  } else if (desc.type() == AttributeType::VEC3F) {
    return promote_vertex_to_primitive_typed<Vec3f>(container, attr_name,
                                                    output_name);
  } else if (desc.type() == AttributeType::INT) {
    return promote_vertex_to_primitive_typed<int>(container, attr_name,
                                                  output_name);
  }

  return false;
}

template <typename T>
bool promote_vertex_to_primitive_typed(GeometryContainer &container,
                                       std::string_view attr_name,
                                       std::string_view output_name) {
  const auto &topology = container.topology();
  const size_t prim_count = topology.primitive_count();

  auto *src = container.get_vertex_attribute_typed<T>(attr_name);
  if (!src) {
    return false;
  }

  const std::string out_name = get_output_name(attr_name, output_name);
  const auto &desc = src->descriptor();

  if (!container.add_primitive_attribute(out_name, desc.type(),
                                         desc.interpolation())) {
    return false;
  }

  auto *dst = container.get_primitive_attribute_typed<T>(out_name);
  if (!dst) {
    return false;
  }

  auto src_span = src->values();
  auto dst_span = dst->values_writable();

  // Average vertex values for each primitive
  for (size_t prim_idx = 0; prim_idx < prim_count; ++prim_idx) {
    const auto &vert_indices = topology.get_primitive_vertices(prim_idx);
    std::vector<T> values;
    values.reserve(vert_indices.size());

    for (int v_idx : vert_indices) {
      if (v_idx >= 0 && v_idx < static_cast<int>(src->size())) {
        values.push_back(src_span[v_idx]);
      }
    }

    if (!values.empty()) {
      dst_span[prim_idx] = detail::average_values(values);
    }
  }

  return true;
}

bool demote_primitive_to_vertex(GeometryContainer &container,
                                std::string_view attr_name,
                                std::string_view output_name) {
  const auto &topology = container.topology();
  const size_t vertex_count = topology.vertex_count();
  const size_t prim_count = topology.primitive_count();

  if (vertex_count == 0 || prim_count == 0) {
    return false;
  }

  auto *src_storage = container.get_primitive_attribute(attr_name);
  if (!src_storage) {
    return false;
  }

  const auto &desc = src_storage->descriptor();
  const std::string out_name = get_output_name(attr_name, output_name);

  // Create vertex attribute
  if (!container.add_vertex_attribute(out_name, desc.type(),
                                      desc.interpolation())) {
    return false;
  }

  auto *dst_storage = container.get_vertex_attribute(out_name);
  if (!dst_storage) {
    return false;
  }

  // For each vertex, copy its primitive's value
  // Need to find which primitive each vertex belongs to
  std::vector<int> vertex_to_prim(vertex_count, -1);

  for (size_t prim_idx = 0; prim_idx < prim_count; ++prim_idx) {
    const auto &vert_indices = topology.get_primitive_vertices(prim_idx);
    for (int v_idx : vert_indices) {
      if (v_idx >= 0 && v_idx < static_cast<int>(vertex_count)) {
        vertex_to_prim[v_idx] = static_cast<int>(prim_idx);
      }
    }
  }

  // Copy primitive values to vertices
  for (size_t v = 0; v < vertex_count; ++v) {
    int prim_idx = vertex_to_prim[v];
    if (prim_idx >= 0) {
      dst_storage->copy_element(prim_idx, v, *src_storage);
    }
  }

  return true;
}

// Forward declarations for template instantiation
template bool promote_point_to_vertex_typed<float>(GeometryContainer &,
                                                   std::string_view,
                                                   std::string_view);
template bool promote_point_to_vertex_typed<int>(GeometryContainer &,
                                                 std::string_view,
                                                 std::string_view);
template bool promote_point_to_vertex_typed<Vec3f>(GeometryContainer &,
                                                   std::string_view,
                                                   std::string_view);

template bool demote_vertex_to_point_typed<float>(GeometryContainer &,
                                                  std::string_view,
                                                  std::string_view);
template bool demote_vertex_to_point_typed<int>(GeometryContainer &,
                                                std::string_view,
                                                std::string_view);
template bool demote_vertex_to_point_typed<Vec3f>(GeometryContainer &,
                                                  std::string_view,
                                                  std::string_view);

template bool promote_point_to_primitive_typed<float>(GeometryContainer &,
                                                      std::string_view,
                                                      std::string_view);
template bool promote_point_to_primitive_typed<int>(GeometryContainer &,
                                                    std::string_view,
                                                    std::string_view);
template bool promote_point_to_primitive_typed<Vec3f>(GeometryContainer &,
                                                      std::string_view,
                                                      std::string_view);

template bool demote_primitive_to_point_typed<float>(GeometryContainer &,
                                                     std::string_view,
                                                     std::string_view);
template bool demote_primitive_to_point_typed<int>(GeometryContainer &,
                                                   std::string_view,
                                                   std::string_view);
template bool demote_primitive_to_point_typed<Vec3f>(GeometryContainer &,
                                                     std::string_view,
                                                     std::string_view);

template bool promote_vertex_to_primitive_typed<float>(GeometryContainer &,
                                                       std::string_view,
                                                       std::string_view);
template bool promote_vertex_to_primitive_typed<int>(GeometryContainer &,
                                                     std::string_view,
                                                     std::string_view);
template bool promote_vertex_to_primitive_typed<Vec3f>(GeometryContainer &,
                                                       std::string_view,
                                                       std::string_view);

} // namespace nodeflux::core
