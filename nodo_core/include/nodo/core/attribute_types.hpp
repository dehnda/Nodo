#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <cstddef>
#include <cstdint>
#include <string>

namespace nodo::core {

/**
 * @brief Element classes that can own attributes
 *
 * Defines the topology level at which an attribute lives:
 * - POINT: Unique positions, shared by vertices
 * - VERTEX: Corners of primitives, can have split normals/UVs
 * - PRIMITIVE: Per-face/polygon attributes
 * - DETAIL: Global/scene-level attributes (single value for entire geometry)
 */
enum class ElementClass : uint8_t {
  POINT = 0,     ///< Point attributes (positions, point colors)
  VERTEX = 1,    ///< Vertex attributes (normals, UVs, vertex colors)
  PRIMITIVE = 2, ///< Primitive attributes (material IDs, face normals)
  DETAIL = 3     ///< Detail/global attributes (bounding box, metadata)
};

/**
 * @brief Supported attribute data types
 *
 * Comprehensive type system for procedural modeling:
 * - Scalars: float, int
 * - Vectors: Vec2f, Vec3f, Vec4f (positions, normals, colors, UVs)
 * - Matrices: Matrix3, Matrix4 (transforms, frames)
 * - Quaternion: For rotation interpolation
 * - String: Names, paths, metadata
 */
enum class AttributeType : uint8_t {
  FLOAT = 0,      ///< Single float
  INT = 1,        ///< Single int
  VEC2F = 2,      ///< Eigen::Vector2f (UVs, 2D coords)
  VEC3F = 3,      ///< Eigen::Vector3f (positions, normals, colors)
  VEC4F = 4,      ///< Eigen::Vector4f (RGBA, homogeneous coords)
  MATRIX3 = 5,    ///< Eigen::Matrix3f (3x3 transform, tangent frame)
  MATRIX4 = 6,    ///< Eigen::Matrix4f (4x4 transform)
  QUATERNION = 7, ///< Eigen::Quaternionf (rotations)
  STRING = 8      ///< std::string (names, paths, metadata)
};

/**
 * @brief Interpolation modes for attribute values
 *
 * Defines how attributes should be interpolated during operations like
 * subdivision, resampling, or attribute promotion/demotion.
 */
enum class InterpolationMode : uint8_t {
  LINEAR = 0, ///< Linear interpolation (default for most types)
  DISCRETE =
      1, ///< No interpolation, nearest-neighbor (IDs, material indices)
  QUATERNION_SLERP = 2, ///< Spherical linear interpolation (quaternions)
  SMOOTH = 3 ///< Smooth/cubic interpolation (future: for curves/surfaces)
};

// Type traits for attribute types
namespace attribute_traits {

/**
 * @brief Get size in bytes for an attribute type
 */
constexpr size_t size_of(AttributeType type) {
  switch (type) {
  case AttributeType::FLOAT:
    return sizeof(float);
  case AttributeType::INT:
    return sizeof(int);
  case AttributeType::VEC2F:
    return sizeof(Eigen::Vector2f);
  case AttributeType::VEC3F:
    return sizeof(Eigen::Vector3f);
  case AttributeType::VEC4F:
    return sizeof(Eigen::Vector4f);
  case AttributeType::MATRIX3:
    return sizeof(Eigen::Matrix3f);
  case AttributeType::MATRIX4:
    return sizeof(Eigen::Matrix4f);
  case AttributeType::QUATERNION:
    return sizeof(Eigen::Quaternionf);
  case AttributeType::STRING:
    return sizeof(std::string);
  default:
    return 0;
  }
}

/**
 * @brief Get number of scalar components for a type
 */
constexpr size_t component_count(AttributeType type) {
  switch (type) {
  case AttributeType::FLOAT:
  case AttributeType::INT:
    return 1;
  case AttributeType::VEC2F:
    return 2;
  case AttributeType::VEC3F:
    return 3;
  case AttributeType::VEC4F:
  case AttributeType::QUATERNION:
    return 4;
  case AttributeType::MATRIX3:
    return 9;
  case AttributeType::MATRIX4:
    return 16;
  case AttributeType::STRING:
    return 0; // N/A for strings
  default:
    return 0;
  }
}

/**
 * @brief Get default interpolation mode for a type
 */
constexpr InterpolationMode default_interpolation(AttributeType type) {
  switch (type) {
  case AttributeType::QUATERNION:
    return InterpolationMode::QUATERNION_SLERP;
  case AttributeType::INT:
  case AttributeType::STRING:
    return InterpolationMode::DISCRETE;
  default:
    return InterpolationMode::LINEAR;
  }
}

/**
 * @brief Check if type is numeric (can be used in math operations)
 */
constexpr bool is_numeric(AttributeType type) {
  return type != AttributeType::STRING;
}

/**
 * @brief Check if type is a vector type
 */
constexpr bool is_vector(AttributeType type) {
  return type == AttributeType::VEC2F || type == AttributeType::VEC3F ||
         type == AttributeType::VEC4F;
}

/**
 * @brief Check if type is a matrix type
 */
constexpr bool is_matrix(AttributeType type) {
  return type == AttributeType::MATRIX3 || type == AttributeType::MATRIX4;
}

/**
 * @brief Get type name as string (for debugging/serialization)
 */
constexpr const char *type_name(AttributeType type) {
  switch (type) {
  case AttributeType::FLOAT:
    return "float";
  case AttributeType::INT:
    return "int";
  case AttributeType::VEC2F:
    return "vec2f";
  case AttributeType::VEC3F:
    return "vec3f";
  case AttributeType::VEC4F:
    return "vec4f";
  case AttributeType::MATRIX3:
    return "matrix3";
  case AttributeType::MATRIX4:
    return "matrix4";
  case AttributeType::QUATERNION:
    return "quaternion";
  case AttributeType::STRING:
    return "string";
  default:
    return "unknown";
  }
}

/**
 * @brief Get element class name as string
 */
constexpr const char *element_class_name(ElementClass cls) {
  switch (cls) {
  case ElementClass::POINT:
    return "point";
  case ElementClass::VERTEX:
    return "vertex";
  case ElementClass::PRIMITIVE:
    return "primitive";
  case ElementClass::DETAIL:
    return "detail";
  default:
    return "unknown";
  }
}

/**
 * @brief Get interpolation mode name as string
 */
constexpr const char *interpolation_mode_name(InterpolationMode mode) {
  switch (mode) {
  case InterpolationMode::LINEAR:
    return "linear";
  case InterpolationMode::DISCRETE:
    return "discrete";
  case InterpolationMode::QUATERNION_SLERP:
    return "quaternion_slerp";
  case InterpolationMode::SMOOTH:
    return "smooth";
  default:
    return "unknown";
  }
}

} // namespace attribute_traits

// Type aliases for convenience
using Vec2f = Eigen::Vector2f;
using Vec3f = Eigen::Vector3f;
using Vec4f = Eigen::Vector4f;
using Matrix3f = Eigen::Matrix3f;
using Matrix4f = Eigen::Matrix4f;
using Quaternionf = Eigen::Quaternionf;

} // namespace nodo::core
