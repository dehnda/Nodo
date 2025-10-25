#include "nodeflux/core/attribute_interpolation.hpp"
#include <algorithm>
#include <cmath>

namespace nodeflux::core {

// ============================================================================
// Linear Interpolation
// ============================================================================

template <typename T> T interpolate_linear(const T &a, const T &b, float t) {
  if constexpr (std::is_floating_point_v<T>) {
    return a + (b - a) * t;
  } else if constexpr (std::is_same_v<T, Vec3f>) {
    return Vec3f{a.x() + (b.x() - a.x()) * t, a.y() + (b.y() - a.y()) * t,
                 a.z() + (b.z() - a.z()) * t};
  } else if constexpr (std::is_same_v<T, Vec2f>) {
    return Vec2f{a.x() + (b.x() - a.x()) * t, a.y() + (b.y() - a.y()) * t};
  } else if constexpr (std::is_same_v<T, Vec4f>) {
    return Vec4f{a.x() + (b.x() - a.x()) * t, a.y() + (b.y() - a.y()) * t,
                 a.z() + (b.z() - a.z()) * t, a.w() + (b.w() - a.w()) * t};
  } else if constexpr (std::is_integral_v<T>) {
    // For integers, round to nearest
    float result = static_cast<float>(a) +
                   (static_cast<float>(b) - static_cast<float>(a)) * t;
    return static_cast<T>(std::round(result));
  } else {
    // Default: just return a (no interpolation for unknown types)
    return a;
  }
}

// ============================================================================
// Cubic Interpolation (Hermite)
// ============================================================================

template <typename T> T interpolate_cubic(const T &a, const T &b, float t) {
  // Hermite curve: 3t² - 2t³
  float smooth_t = smoothstep(t);
  return interpolate_linear(a, b, smooth_t);
}

// ============================================================================
// Weighted Average
// ============================================================================

template <typename T>
T interpolate_weighted(std::span<const T> values,
                       std::span<const float> weights) {
  if (values.empty() || weights.empty() || values.size() != weights.size()) {
    return T{};
  }

  if constexpr (std::is_floating_point_v<T>) {
    T result = 0;
    for (size_t i = 0; i < values.size(); ++i) {
      result += values[i] * weights[i];
    }
    return result;
  } else if constexpr (std::is_same_v<T, Vec3f>) {
    Vec3f result{0.0f, 0.0f, 0.0f};
    for (size_t i = 0; i < values.size(); ++i) {
      result = Vec3f{result.x() + values[i].x() * weights[i],
                     result.y() + values[i].y() * weights[i],
                     result.z() + values[i].z() * weights[i]};
    }
    return result;
  } else if constexpr (std::is_same_v<T, Vec2f>) {
    Vec2f result{0.0f, 0.0f};
    for (size_t i = 0; i < values.size(); ++i) {
      result = Vec2f{result.x() + values[i].x() * weights[i],
                     result.y() + values[i].y() * weights[i]};
    }
    return result;
  } else if constexpr (std::is_same_v<T, Vec4f>) {
    Vec4f result{0.0f, 0.0f, 0.0f, 0.0f};
    for (size_t i = 0; i < values.size(); ++i) {
      result = Vec4f{result.x() + values[i].x() * weights[i],
                     result.y() + values[i].y() * weights[i],
                     result.z() + values[i].z() * weights[i],
                     result.w() + values[i].w() * weights[i]};
    }
    return result;
  } else if constexpr (std::is_integral_v<T>) {
    float result = 0.0f;
    for (size_t i = 0; i < values.size(); ++i) {
      result += static_cast<float>(values[i]) * weights[i];
    }
    return static_cast<T>(std::round(result));
  } else {
    return values[0];
  }
}

// ============================================================================
// Barycentric Interpolation (Triangle)
// ============================================================================

template <typename T>
T interpolate_barycentric(const T &v0, const T &v1, const T &v2, float u,
                          float v) {
  // Barycentric coordinates: w = 1 - u - v
  float w = 1.0f - u - v;

  if constexpr (std::is_floating_point_v<T>) {
    return v0 * w + v1 * u + v2 * v;
  } else if constexpr (std::is_same_v<T, Vec3f>) {
    return Vec3f{v0.x() * w + v1.x() * u + v2.x() * v,
                 v0.y() * w + v1.y() * u + v2.y() * v,
                 v0.z() * w + v1.z() * u + v2.z() * v};
  } else if constexpr (std::is_same_v<T, Vec2f>) {
    return Vec2f{v0.x() * w + v1.x() * u + v2.x() * v,
                 v0.y() * w + v1.y() * u + v2.y() * v};
  } else if constexpr (std::is_same_v<T, Vec4f>) {
    return Vec4f{v0.x() * w + v1.x() * u + v2.x() * v,
                 v0.y() * w + v1.y() * u + v2.y() * v,
                 v0.z() * w + v1.z() * u + v2.z() * v,
                 v0.w() * w + v1.w() * u + v2.w() * v};
  } else if constexpr (std::is_integral_v<T>) {
    float result = static_cast<float>(v0) * w + static_cast<float>(v1) * u +
                   static_cast<float>(v2) * v;
    return static_cast<T>(std::round(result));
  } else {
    return v0;
  }
}

// ============================================================================
// Bilinear Interpolation (Quad)
// ============================================================================

template <typename T>
T interpolate_bilinear(const T &v00, const T &v10, const T &v01, const T &v11,
                       float u, float v) {
  // Interpolate along u direction first
  T a = interpolate_linear(v00, v10, u);
  T b = interpolate_linear(v01, v11, u);

  // Then interpolate along v direction
  return interpolate_linear(a, b, v);
}

// ============================================================================
// Blend Attributes
// ============================================================================

template <typename T>
bool blend_attributes(GeometryContainer &container, std::string_view attr_name,
                      ElementClass element_class,
                      const std::vector<size_t> &source_indices,
                      size_t target_index, const std::vector<float> &weights) {
  if (source_indices.empty()) {
    return false;
  }

  // Get the attribute
  AttributeStorage<T> *attr = nullptr;
  switch (element_class) {
  case ElementClass::POINT:
    attr = container.get_point_attribute_typed<T>(attr_name);
    break;
  case ElementClass::PRIMITIVE:
    attr = container.get_primitive_attribute_typed<T>(attr_name);
    break;
  case ElementClass::VERTEX:
    attr = container.get_vertex_attribute_typed<T>(attr_name);
    break;
  case ElementClass::DETAIL:
    return false;
  }

  if (!attr || target_index >= attr->size()) {
    return false;
  }

  // Collect source values
  std::vector<T> values;
  values.reserve(source_indices.size());

  auto attr_values = attr->values();
  for (size_t idx : source_indices) {
    if (idx < attr_values.size()) {
      values.push_back(attr_values[idx]);
    }
  }

  if (values.empty()) {
    return false;
  }

  // Compute weights if not provided
  std::vector<float> w = weights;
  if (w.empty() || w.size() != values.size()) {
    // Equal weighting
    float equal_weight = 1.0f / static_cast<float>(values.size());
    w.assign(values.size(), equal_weight);
  }

  // Blend and write result
  T blended = interpolate_weighted<T>(values, w);
  auto writable = attr->values_writable();
  writable[target_index] = blended;

  return true;
}

// ============================================================================
// Copy and Interpolate All Attributes
// ============================================================================

bool copy_and_interpolate_all_attributes(
    GeometryContainer &container, ElementClass element_class,
    const std::vector<size_t> &source_indices, size_t target_index,
    const std::vector<float> &weights) {
  if (source_indices.empty()) {
    return false;
  }

  // Get attribute set
  const AttributeSet *attr_set = nullptr;
  switch (element_class) {
  case ElementClass::POINT:
    attr_set = &container.point_attributes();
    break;
  case ElementClass::PRIMITIVE:
    attr_set = &container.primitive_attributes();
    break;
  case ElementClass::VERTEX:
    attr_set = &container.vertex_attributes();
    break;
  case ElementClass::DETAIL:
    return false;
  }

  if (!attr_set) {
    return false;
  }

  // Iterate through all attributes and blend them
  bool success = true;
  for (const auto &attr_name : attr_set->attribute_names()) {
    auto *storage = attr_set->get_storage(attr_name);
    if (!storage) {
      continue;
    }

    const auto type = storage->descriptor().type();

    // Blend based on type
    switch (type) {
    case AttributeType::FLOAT:
      success &= blend_attributes<float>(container, attr_name, element_class,
                                         source_indices, target_index, weights);
      break;
    case AttributeType::VEC3F:
      success &= blend_attributes<Vec3f>(container, attr_name, element_class,
                                         source_indices, target_index, weights);
      break;
    case AttributeType::VEC2F:
      success &= blend_attributes<Vec2f>(container, attr_name, element_class,
                                         source_indices, target_index, weights);
      break;
    case AttributeType::VEC4F:
      success &= blend_attributes<Vec4f>(container, attr_name, element_class,
                                         source_indices, target_index, weights);
      break;
    case AttributeType::INT:
      success &= blend_attributes<int>(container, attr_name, element_class,
                                       source_indices, target_index, weights);
      break;
    default:
      // Skip unsupported types
      break;
    }
  }

  return success;
}

// ============================================================================
// Transfer Point to Primitive Attributes
// ============================================================================

bool transfer_point_to_primitive_attributes(
    GeometryContainer &container, const std::vector<int> &point_indices,
    size_t prim_index) {
  if (point_indices.empty()) {
    return false;
  }

  // Convert int indices to size_t
  std::vector<size_t> source_indices;
  source_indices.reserve(point_indices.size());
  for (int idx : point_indices) {
    if (idx >= 0) {
      source_indices.push_back(static_cast<size_t>(idx));
    }
  }

  if (source_indices.empty()) {
    return false;
  }

  // Equal weights for averaging
  std::vector<float> weights(source_indices.size(),
                             1.0f / static_cast<float>(source_indices.size()));

  // For each point attribute, create/update corresponding primitive attribute
  const auto &point_attrs = container.point_attributes();

  bool success = true;
  for (const auto &attr_name : point_attrs.attribute_names()) {
    auto *point_attr = point_attrs.get_storage(attr_name);
    if (!point_attr) {
      continue;
    }

    const auto type = point_attr->descriptor().type();

    // Create primitive attribute if it doesn't exist
    if (!container.has_primitive_attribute(attr_name)) {
      container.add_primitive_attribute(attr_name, type);
    }

    // Blend point values to primitive
    switch (type) {
    case AttributeType::FLOAT:
      success &=
          blend_attributes<float>(container, attr_name, ElementClass::POINT,
                                  source_indices, prim_index, weights);
      break;
    case AttributeType::VEC3F:
      success &=
          blend_attributes<Vec3f>(container, attr_name, ElementClass::POINT,
                                  source_indices, prim_index, weights);
      break;
    case AttributeType::VEC2F:
      success &=
          blend_attributes<Vec2f>(container, attr_name, ElementClass::POINT,
                                  source_indices, prim_index, weights);
      break;
    case AttributeType::VEC4F:
      success &=
          blend_attributes<Vec4f>(container, attr_name, ElementClass::POINT,
                                  source_indices, prim_index, weights);
      break;
    case AttributeType::INT:
      success &=
          blend_attributes<int>(container, attr_name, ElementClass::POINT,
                                source_indices, prim_index, weights);
      break;
    default:
      break;
    }
  }

  return success;
}

// ============================================================================
// Resample Curve Attribute
// ============================================================================

template <typename T>
T resample_curve_attribute(const GeometryContainer &container,
                           std::string_view attr_name,
                           const std::vector<int> &point_indices, float t) {
  if (point_indices.size() < 2) {
    return T{};
  }

  // Get the attribute
  const auto *attr = container.get_point_attribute_typed<T>(attr_name);
  if (!attr) {
    return T{};
  }

  auto values = attr->values();

  // Clamp t to [0, 1]
  t = saturate(t);

  // Find segment
  float segment_t = t * static_cast<float>(point_indices.size() - 1);
  size_t segment_idx = static_cast<size_t>(std::floor(segment_t));
  float local_t = segment_t - static_cast<float>(segment_idx);

  // Clamp segment index
  if (segment_idx >= point_indices.size() - 1) {
    segment_idx = point_indices.size() - 2;
    local_t = 1.0f;
  }

  int idx0 = point_indices[segment_idx];
  int idx1 = point_indices[segment_idx + 1];

  if (idx0 < 0 || idx0 >= static_cast<int>(values.size()) || idx1 < 0 ||
      idx1 >= static_cast<int>(values.size())) {
    return T{};
  }

  return interpolate_linear(values[idx0], values[idx1], local_t);
}

// ============================================================================
// Specialized Interpolations
// ============================================================================

Vec4f slerp(const Vec4f &q0, const Vec4f &q1, float t) {
  // Quaternion spherical linear interpolation
  float dot =
      q0.x() * q1.x() + q0.y() * q1.y() + q0.z() * q1.z() + q0.w() * q1.w();

  // If the dot product is negative, negate q1 to take the shorter path
  Vec4f q1_adj = q1;
  if (dot < 0.0f) {
    q1_adj = Vec4f{-q1.x(), -q1.y(), -q1.z(), -q1.w()};
    dot = -dot;
  }

  // If quaternions are very close, use linear interpolation
  if (dot > 0.9995f) {
    return interpolate_linear(q0, q1_adj, t);
  }

  // Calculate interpolation using slerp formula
  float theta = std::acos(dot);
  float sin_theta = std::sin(theta);

  float w0 = std::sin((1.0f - t) * theta) / sin_theta;
  float w1 = std::sin(t * theta) / sin_theta;

  return Vec4f{q0.x() * w0 + q1_adj.x() * w1, q0.y() * w0 + q1_adj.y() * w1,
               q0.z() * w0 + q1_adj.z() * w1, q0.w() * w0 + q1_adj.w() * w1};
}

Vec3f interpolate_normal(const Vec3f &n0, const Vec3f &n1, float t) {
  // Linear interpolation followed by normalization
  Vec3f result = interpolate_linear(n0, n1, t);
  float length = std::sqrt(result.x() * result.x() + result.y() * result.y() +
                           result.z() * result.z());

  if (length > 1e-6f) {
    return Vec3f{result.x() / length, result.y() / length, result.z() / length};
  }

  return result;
}

Vec3f interpolate_color(const Vec3f &c0, const Vec3f &c1, float t,
                        bool linearize) {
  if (!linearize) {
    return interpolate_linear(c0, c1, t);
  }

  // sRGB to linear conversion (simplified)
  auto to_linear = [](float c) -> float {
    return (c <= 0.04045f) ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
  };

  auto to_srgb = [](float c) -> float {
    return (c <= 0.0031308f) ? c * 12.92f
                             : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
  };

  // Convert to linear, interpolate, convert back
  Vec3f c0_linear{to_linear(c0.x()), to_linear(c0.y()), to_linear(c0.z())};
  Vec3f c1_linear{to_linear(c1.x()), to_linear(c1.y()), to_linear(c1.z())};

  Vec3f result_linear = interpolate_linear(c0_linear, c1_linear, t);

  return Vec3f{to_srgb(result_linear.x()), to_srgb(result_linear.y()),
               to_srgb(result_linear.z())};
}

template <typename T>
T interpolate_clamped(const T &a, const T &b, float t, const T &min_val,
                      const T &max_val) {
  T result = interpolate_linear(a, b, t);
  return std::max(min_val, std::min(max_val, result));
}

// Explicit template instantiations
template float interpolate_linear<float>(const float &, const float &, float);
template int interpolate_linear<int>(const int &, const int &, float);
template Vec2f interpolate_linear<Vec2f>(const Vec2f &, const Vec2f &, float);
template Vec3f interpolate_linear<Vec3f>(const Vec3f &, const Vec3f &, float);
template Vec4f interpolate_linear<Vec4f>(const Vec4f &, const Vec4f &, float);

template float interpolate_cubic<float>(const float &, const float &, float);
template Vec3f interpolate_cubic<Vec3f>(const Vec3f &, const Vec3f &, float);

template float interpolate_weighted<float>(std::span<const float>,
                                           std::span<const float>);
template int interpolate_weighted<int>(std::span<const int>,
                                       std::span<const float>);
template Vec2f interpolate_weighted<Vec2f>(std::span<const Vec2f>,
                                           std::span<const float>);
template Vec3f interpolate_weighted<Vec3f>(std::span<const Vec3f>,
                                           std::span<const float>);
template Vec4f interpolate_weighted<Vec4f>(std::span<const Vec4f>,
                                           std::span<const float>);

template float interpolate_barycentric<float>(const float &, const float &,
                                              const float &, float, float);
template Vec3f interpolate_barycentric<Vec3f>(const Vec3f &, const Vec3f &,
                                              const Vec3f &, float, float);

template float interpolate_bilinear<float>(const float &, const float &,
                                           const float &, const float &, float,
                                           float);
template Vec3f interpolate_bilinear<Vec3f>(const Vec3f &, const Vec3f &,
                                           const Vec3f &, const Vec3f &, float,
                                           float);

template bool blend_attributes<float>(GeometryContainer &, std::string_view,
                                      ElementClass, const std::vector<size_t> &,
                                      size_t, const std::vector<float> &);
template bool blend_attributes<int>(GeometryContainer &, std::string_view,
                                    ElementClass, const std::vector<size_t> &,
                                    size_t, const std::vector<float> &);
template bool blend_attributes<Vec2f>(GeometryContainer &, std::string_view,
                                      ElementClass, const std::vector<size_t> &,
                                      size_t, const std::vector<float> &);
template bool blend_attributes<Vec3f>(GeometryContainer &, std::string_view,
                                      ElementClass, const std::vector<size_t> &,
                                      size_t, const std::vector<float> &);
template bool blend_attributes<Vec4f>(GeometryContainer &, std::string_view,
                                      ElementClass, const std::vector<size_t> &,
                                      size_t, const std::vector<float> &);

template float resample_curve_attribute<float>(const GeometryContainer &,
                                               std::string_view,
                                               const std::vector<int> &, float);
template Vec3f resample_curve_attribute<Vec3f>(const GeometryContainer &,
                                               std::string_view,
                                               const std::vector<int> &, float);

template float interpolate_clamped<float>(const float &, const float &, float,
                                          const float &, const float &);
template int interpolate_clamped<int>(const int &, const int &, float,
                                      const int &, const int &);

} // namespace nodeflux::core
