#pragma once

#include "attribute_types.hpp"

#include <string_view>

namespace nodo::core {

/**
 * @brief Standard attribute names and definitions
 *
 * Houdini-compatible naming conventions for common attributes.
 * Using these standard names ensures:
 * - Interoperability between nodes
 * - Automatic recognition by renderers/exporters
 * - Consistent behavior across the pipeline
 *
 * Naming convention:
 * - Single letter uppercase for fundamental attributes (P, N, Cd)
 * - Lowercase for secondary attributes (uv, id, name)
 * - Descriptive names for specialized attributes (instance_id, material_id)
 */
namespace standard_attrs {

// ============================================================================
// POINT ATTRIBUTES (ElementClass::POINT)
// ============================================================================

/// Position (Vec3f) - The fundamental point position in 3D space
constexpr std::string_view P = "P";

/// Velocity (Vec3f) - Motion vector for dynamics/animation
constexpr std::string_view v = "v";

/// Point color (Vec3f) - RGB color per point
constexpr std::string_view Cd = "Cd";

/// Point alpha (float) - Transparency/opacity
constexpr std::string_view Alpha = "Alpha";

/// Point scale (float or Vec3f) - Uniform or non-uniform scale
constexpr std::string_view pscale = "pscale";

/// Point ID (int) - Unique identifier for point tracking
constexpr std::string_view id = "id";

/// Point normal (Vec3f) - For point clouds
constexpr std::string_view point_normal = "point_N";

/// Mass (float) - For physics/dynamics
constexpr std::string_view mass = "mass";

// ============================================================================
// VERTEX ATTRIBUTES (ElementClass::VERTEX)
// ============================================================================

/// Vertex normal (Vec3f) - The most common normal attribute
constexpr std::string_view N = "N";

/// Vertex UV coordinates (Vec2f or Vec3f) - Texture mapping
constexpr std::string_view uv = "uv";

/// Vertex color (Vec3f) - RGB color per vertex
constexpr std::string_view vertex_Cd = "vertex_Cd";

/// Vertex alpha (float) - Per-vertex transparency
constexpr std::string_view vertex_Alpha = "vertex_Alpha";

/// Tangent (Vec3f) - For normal mapping
constexpr std::string_view tangentu = "tangentu";

/// Bitangent (Vec3f) - For normal mapping
constexpr std::string_view tangentv = "tangentv";

/// Secondary UV set (Vec2f or Vec3f)
constexpr std::string_view uv2 = "uv2";

/// Vertex weights (float) - For skinning/deformation
constexpr std::string_view weight = "weight";

// ============================================================================
// PRIMITIVE ATTRIBUTES (ElementClass::PRIMITIVE)
// ============================================================================

/// Primitive/face normal (Vec3f)
constexpr std::string_view primitive_N = "primitive_N";

/// Material ID (int) - Index into material array
constexpr std::string_view material_id = "material_id";

/// Material path (string) - Path to material definition
constexpr std::string_view material = "material";

/// Primitive color (Vec3f)
constexpr std::string_view primitive_Cd = "primitive_Cd";

/// Primitive ID (int) - Unique face identifier
constexpr std::string_view primitive_id = "prim_id";

/// Instance ID (int) - For arrayed/instanced geometry
constexpr std::string_view instance_id = "instance_id";

/// Group name (string) - Primitive group membership
constexpr std::string_view group = "group";

/// Area (float) - Surface area of primitive
constexpr std::string_view area = "area";

/// Primitive center (Vec3f) - Centroid of face
constexpr std::string_view primitive_center = "prim_center";

// ============================================================================
// DETAIL ATTRIBUTES (ElementClass::DETAIL)
// ============================================================================

/// Bounding box minimum (Vec3f)
constexpr std::string_view bounds_min = "bounds_min";

/// Bounding box maximum (Vec3f)
constexpr std::string_view bounds_max = "bounds_max";

/// Total primitive count (int)
constexpr std::string_view num_primitives = "num_primitives";

/// Total point count (int)
constexpr std::string_view num_points = "num_points";

/// Total vertex count (int)
constexpr std::string_view num_vertices = "num_vertices";

/// Frame number (int) - For animation
constexpr std::string_view frame = "frame";

/// Time (float) - Animation time in seconds
constexpr std::string_view time = "time";

/// Name (string) - Object/geometry name
constexpr std::string_view name = "name";

/// Path (string) - File path for loaded geometry
constexpr std::string_view path = "path";

/// Array/instance count (int)
constexpr std::string_view instance_count = "instance_count";

/// Array type (string) - Type of array operation
constexpr std::string_view array_type = "array_type";

// ============================================================================
// SPECIALIZED ATTRIBUTES
// ============================================================================

/// Transform matrix (Matrix4) - Local to world transform
constexpr std::string_view transform = "transform";

/// Orientation (Quaternion) - Rotation as quaternion
constexpr std::string_view orient = "orient";

/// Up vector (Vec3f) - For orientation/alignment
constexpr std::string_view up = "up";

/// Rest position (Vec3f) - Original position before deformation
constexpr std::string_view rest_P = "rest_P";

/// Rest normal (Vec3f) - Original normal before deformation
constexpr std::string_view rest_N = "rest_N";

} // namespace standard_attrs

/**
 * @brief Attribute metadata for standard attributes
 *
 * Provides type and class information for standard attributes.
 * Used for automatic attribute creation and validation.
 */
struct StandardAttributeInfo {
  std::string_view name;
  AttributeType type;
  ElementClass element_class;
  InterpolationMode interpolation;

  constexpr StandardAttributeInfo(
      std::string_view n, AttributeType t, ElementClass e,
      InterpolationMode i = InterpolationMode::LINEAR)
      : name(n), type(t), element_class(e), interpolation(i) {}
};

/**
 * @brief Registry of standard attribute definitions
 *
 * Use this to look up standard attribute metadata by name.
 */
namespace standard_attr_registry {

// Point attributes
constexpr StandardAttributeInfo P{standard_attrs::P, AttributeType::VEC3F,
                                  ElementClass::POINT};
constexpr StandardAttributeInfo v{standard_attrs::v, AttributeType::VEC3F,
                                  ElementClass::POINT};
constexpr StandardAttributeInfo Cd{standard_attrs::Cd, AttributeType::VEC3F,
                                   ElementClass::POINT};
constexpr StandardAttributeInfo Alpha{
    standard_attrs::Alpha, AttributeType::FLOAT, ElementClass::POINT};
constexpr StandardAttributeInfo pscale{
    standard_attrs::pscale, AttributeType::FLOAT, ElementClass::POINT};
constexpr StandardAttributeInfo id{standard_attrs::id, AttributeType::INT,
                                   ElementClass::POINT,
                                   InterpolationMode::DISCRETE};

// Vertex attributes
constexpr StandardAttributeInfo N{standard_attrs::N, AttributeType::VEC3F,
                                  ElementClass::VERTEX};
constexpr StandardAttributeInfo uv{standard_attrs::uv, AttributeType::VEC2F,
                                   ElementClass::VERTEX};
constexpr StandardAttributeInfo tangentu{
    standard_attrs::tangentu, AttributeType::VEC3F, ElementClass::VERTEX};
constexpr StandardAttributeInfo tangentv{
    standard_attrs::tangentv, AttributeType::VEC3F, ElementClass::VERTEX};

// Primitive attributes
constexpr StandardAttributeInfo material_id{
    standard_attrs::material_id, AttributeType::INT, ElementClass::PRIMITIVE,
    InterpolationMode::DISCRETE};
constexpr StandardAttributeInfo material{
    standard_attrs::material, AttributeType::STRING, ElementClass::PRIMITIVE,
    InterpolationMode::DISCRETE};
constexpr StandardAttributeInfo instance_id{
    standard_attrs::instance_id, AttributeType::INT, ElementClass::PRIMITIVE,
    InterpolationMode::DISCRETE};

// Detail attributes
constexpr StandardAttributeInfo bounds_min{
    standard_attrs::bounds_min, AttributeType::VEC3F, ElementClass::DETAIL};
constexpr StandardAttributeInfo bounds_max{
    standard_attrs::bounds_max, AttributeType::VEC3F, ElementClass::DETAIL};
constexpr StandardAttributeInfo frame{standard_attrs::frame, AttributeType::INT,
                                      ElementClass::DETAIL,
                                      InterpolationMode::DISCRETE};
constexpr StandardAttributeInfo time{standard_attrs::time, AttributeType::FLOAT,
                                     ElementClass::DETAIL};
constexpr StandardAttributeInfo name{
    standard_attrs::name, AttributeType::STRING, ElementClass::DETAIL,
    InterpolationMode::DISCRETE};

// Specialized attributes
constexpr StandardAttributeInfo transform{
    standard_attrs::transform, AttributeType::MATRIX4, ElementClass::POINT};
constexpr StandardAttributeInfo orient{
    standard_attrs::orient, AttributeType::QUATERNION, ElementClass::POINT,
    InterpolationMode::QUATERNION_SLERP};
constexpr StandardAttributeInfo up{standard_attrs::up, AttributeType::VEC3F,
                                   ElementClass::POINT};
constexpr StandardAttributeInfo rest_P{
    standard_attrs::rest_P, AttributeType::VEC3F, ElementClass::POINT};
constexpr StandardAttributeInfo rest_N{
    standard_attrs::rest_N, AttributeType::VEC3F, ElementClass::VERTEX};

} // namespace standard_attr_registry

/**
 * @brief Helper to check if an attribute name is a standard attribute
 */
inline bool is_standard_attribute(std::string_view attr_name) {
  // Check against common standard attributes
  return attr_name == standard_attrs::P || attr_name == standard_attrs::N ||
         attr_name == standard_attrs::Cd || attr_name == standard_attrs::uv ||
         attr_name == standard_attrs::v || attr_name == standard_attrs::Alpha ||
         attr_name == standard_attrs::pscale || attr_name == standard_attrs::id;
  // Add more as needed
}

} // namespace nodo::core
