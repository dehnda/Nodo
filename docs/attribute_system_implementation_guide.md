# Attribute System Implementation Guide
**Production-Grade Attribute System - Step-by-Step Build Plan**

**Status**: Ready to Implement
**Timeline**: 12 weeks (3 phases × 4 weeks)
**Goal**: Replace dual attribute systems with unified, production-ready foundation

---

## Table of Contents

1. [Overview](#overview)
2. [Phase 1: Foundation (Weeks 1-4)](#phase-1-foundation-weeks-1-4)
3. [Phase 2: Integration (Weeks 5-8)](#phase-2-integration-weeks-5-8)
4. [Phase 3: Performance & Polish (Weeks 9-12)](#phase-3-performance--polish-weeks-9-12)
5. [Success Criteria](#success-criteria)
6. [Risk Mitigation](#risk-mitigation)

---

## Overview

### What We're Building

**Core Components** (4 new files):
1. `attribute_descriptor.hpp` - Metadata and schema
2. `standard_attributes.hpp` - Constants like "P", "N", "Cd"
3. `element_topology.hpp` - Point/Vertex/Primitive model
4. `attribute_storage.hpp` - Fast SoA containers

**Enhanced Components** (2 modified files):
1. `geometry_attributes.hpp` - Add descriptors, views, validation
2. `geometry_data.hpp` - Integrate new attribute system

**Migration**: 18 SOP nodes to new API

### Key Decisions (Agreed Upon)

✅ **Comprehensive type system**: float, int, Vec2/3/4, Matrix3/4, Quaternion, string
✅ **N-gon support**: Variable-length primitives from start
✅ **GPU-ready design**: SoA layout, implement GPU staging in Phase 3
✅ **Wrangle later**: Post-launch feature

---

# Phase 1: Foundation (Weeks 1-4)

**Goal**: Build core infrastructure without breaking existing code

**Deliverables**: 4 new header files, comprehensive tests, no SOP changes yet

---

## Week 1: Element Topology & Type System

### Task 1.1: Define Element Topology Model

**File**: `nodeflux_core/include/nodeflux/core/element_topology.hpp`

**Code to write**:

```cpp
#pragma once
#include <vector>
#include <cstddef>

namespace nodeflux::core {

/**
 * @brief Element class in procedural modeling hierarchy
 *
 * POINT:     Unique positions in space (shared by vertices)
 * VERTEX:    Corner of a primitive (references a point + has unique attributes)
 * PRIMITIVE: Face, curve, volume, etc.
 * DETAIL:    Global/geometry-level
 */
enum class ElementClass {
    POINT,      // Shared positions
    VERTEX,     // Per-corner attributes (split normals/UVs)
    PRIMITIVE,  // Per-face/curve/volume
    DETAIL      // Global/mesh-level
};

/**
 * @brief Type of primitive geometry
 */
enum class PrimitiveType {
    POLYGON,        // N-sided face (triangle, quad, etc.)
    POLYLINE,       // Open curve
    BEZIER_CURVE,   // Bezier curve
    NURBS_CURVE,    // NURBS curve
    VOLUME,         // Voxel grid
    PACKED_PRIM,    // Instanced geometry
};

/**
 * @brief Complete topology description for procedural geometry
 *
 * Defines the relationship between points, vertices, and primitives.
 * A point is a unique position in space.
 * A vertex is a corner of a primitive that references a point.
 * A primitive is a face, curve, or other geometric element.
 */
class ElementTopology {
public:
    ElementTopology() = default;

    // Element counts
    size_t point_count() const { return point_count_; }
    size_t vertex_count() const { return vertex_count_; }
    size_t primitive_count() const { return primitive_count_; }

    // Set counts (will resize internal structures)
    void set_point_count(size_t count);
    void set_vertex_count(size_t count);
    void set_primitive_count(size_t count);

    // Vertex → Point mapping
    int get_vertex_point(size_t vertex_idx) const;
    void set_vertex_point(size_t vertex_idx, int point_idx);
    const std::vector<int>& vertex_points() const { return vertex_points_; }

    // Primitive definitions (variable-length vertex lists)
    size_t get_primitive_vertex_count(size_t prim_idx) const;
    const std::vector<int>& get_primitive_vertices(size_t prim_idx) const;
    void set_primitive_vertices(size_t prim_idx, const std::vector<int>& vertices);

    // Primitive types
    PrimitiveType get_primitive_type(size_t prim_idx) const;
    void set_primitive_type(size_t prim_idx, PrimitiveType type);

    // Validation
    bool is_valid() const;
    std::string validate() const;  // Returns error message if invalid

private:
    size_t point_count_ = 0;
    size_t vertex_count_ = 0;
    size_t primitive_count_ = 0;

    // Vertex → Point mapping (vertex_points_[vertex_idx] = point_idx)
    std::vector<int> vertex_points_;

    // Primitive → Vertex mapping (variable-length)
    // primitive_vertex_starts_[i] = start index in primitive_vertices_
    // primitive_vertex_counts_[i] = how many vertices
    std::vector<int> primitive_vertex_starts_;
    std::vector<int> primitive_vertex_counts_;
    std::vector<int> primitive_vertices_;  // Flattened vertex indices

    // Primitive types
    std::vector<PrimitiveType> primitive_types_;
};

} // namespace nodeflux::core
```

**Implementation file**: `nodeflux_core/src/core/element_topology.cpp`

```cpp
#include "nodeflux/core/element_topology.hpp"
#include <stdexcept>
#include <sstream>

namespace nodeflux::core {

void ElementTopology::set_point_count(size_t count) {
    point_count_ = count;
}

void ElementTopology::set_vertex_count(size_t count) {
    vertex_count_ = count;
    vertex_points_.resize(count, -1);  // -1 = unassigned
}

void ElementTopology::set_primitive_count(size_t count) {
    primitive_count_ = count;
    primitive_vertex_starts_.resize(count, 0);
    primitive_vertex_counts_.resize(count, 0);
    primitive_types_.resize(count, PrimitiveType::POLYGON);
}

int ElementTopology::get_vertex_point(size_t vertex_idx) const {
    if (vertex_idx >= vertex_count_) {
        throw std::out_of_range("Vertex index out of range");
    }
    return vertex_points_[vertex_idx];
}

void ElementTopology::set_vertex_point(size_t vertex_idx, int point_idx) {
    if (vertex_idx >= vertex_count_) {
        throw std::out_of_range("Vertex index out of range");
    }
    if (point_idx >= static_cast<int>(point_count_) && point_idx != -1) {
        throw std::out_of_range("Point index out of range");
    }
    vertex_points_[vertex_idx] = point_idx;
}

size_t ElementTopology::get_primitive_vertex_count(size_t prim_idx) const {
    if (prim_idx >= primitive_count_) {
        throw std::out_of_range("Primitive index out of range");
    }
    return primitive_vertex_counts_[prim_idx];
}

const std::vector<int>& ElementTopology::get_primitive_vertices(size_t prim_idx) const {
    // Return a view/slice - for now, copy (optimize later with span)
    static thread_local std::vector<int> temp;
    temp.clear();

    if (prim_idx >= primitive_count_) {
        return temp;
    }

    int start = primitive_vertex_starts_[prim_idx];
    int count = primitive_vertex_counts_[prim_idx];
    temp.assign(primitive_vertices_.begin() + start,
                primitive_vertices_.begin() + start + count);
    return temp;
}

void ElementTopology::set_primitive_vertices(size_t prim_idx,
                                             const std::vector<int>& vertices) {
    if (prim_idx >= primitive_count_) {
        throw std::out_of_range("Primitive index out of range");
    }

    // Append to flattened array
    int start = static_cast<int>(primitive_vertices_.size());
    primitive_vertex_starts_[prim_idx] = start;
    primitive_vertex_counts_[prim_idx] = static_cast<int>(vertices.size());
    primitive_vertices_.insert(primitive_vertices_.end(),
                               vertices.begin(), vertices.end());
}

PrimitiveType ElementTopology::get_primitive_type(size_t prim_idx) const {
    if (prim_idx >= primitive_count_) {
        throw std::out_of_range("Primitive index out of range");
    }
    return primitive_types_[prim_idx];
}

void ElementTopology::set_primitive_type(size_t prim_idx, PrimitiveType type) {
    if (prim_idx >= primitive_count_) {
        throw std::out_of_range("Primitive index out of range");
    }
    primitive_types_[prim_idx] = type;
}

bool ElementTopology::is_valid() const {
    return validate().empty();
}

std::string ElementTopology::validate() const {
    std::ostringstream errors;

    // Check vertex → point mapping
    for (size_t i = 0; i < vertex_count_; ++i) {
        int pt = vertex_points_[i];
        if (pt < 0 || pt >= static_cast<int>(point_count_)) {
            errors << "Vertex " << i << " references invalid point " << pt << "\n";
        }
    }

    // Check primitive → vertex mapping
    for (size_t i = 0; i < primitive_count_; ++i) {
        int start = primitive_vertex_starts_[i];
        int count = primitive_vertex_counts_[i];

        if (start < 0 || start + count > static_cast<int>(primitive_vertices_.size())) {
            errors << "Primitive " << i << " has invalid vertex range\n";
            continue;
        }

        for (int j = start; j < start + count; ++j) {
            int vtx = primitive_vertices_[j];
            if (vtx < 0 || vtx >= static_cast<int>(vertex_count_)) {
                errors << "Primitive " << i << " references invalid vertex " << vtx << "\n";
            }
        }
    }

    return errors.str();
}

} // namespace nodeflux::core
```

**Tests**: `tests/test_element_topology.cpp`

```cpp
#include "nodeflux/core/element_topology.hpp"
#include <gtest/gtest.h>

using namespace nodeflux::core;

TEST(ElementTopologyTest, BasicCounts) {
    ElementTopology topo;

    topo.set_point_count(4);
    topo.set_vertex_count(4);
    topo.set_primitive_count(1);

    EXPECT_EQ(topo.point_count(), 4);
    EXPECT_EQ(topo.vertex_count(), 4);
    EXPECT_EQ(topo.primitive_count(), 1);
}

TEST(ElementTopologyTest, VertexPointMapping) {
    ElementTopology topo;
    topo.set_point_count(8);
    topo.set_vertex_count(24);  // Cube: 6 faces × 4 vertices

    // Map first 4 vertices to first 4 points (first face)
    topo.set_vertex_point(0, 0);
    topo.set_vertex_point(1, 1);
    topo.set_vertex_point(2, 2);
    topo.set_vertex_point(3, 3);

    EXPECT_EQ(topo.get_vertex_point(0), 0);
    EXPECT_EQ(topo.get_vertex_point(1), 1);
    EXPECT_EQ(topo.get_vertex_point(2), 2);
    EXPECT_EQ(topo.get_vertex_point(3), 3);
}

TEST(ElementTopologyTest, VariableLengthPrimitives) {
    ElementTopology topo;
    topo.set_point_count(4);
    topo.set_vertex_count(7);
    topo.set_primitive_count(2);

    // Triangle (3 vertices)
    topo.set_primitive_vertices(0, {0, 1, 2});
    EXPECT_EQ(topo.get_primitive_vertex_count(0), 3);

    // Quad (4 vertices)
    topo.set_primitive_vertices(1, {3, 4, 5, 6});
    EXPECT_EQ(topo.get_primitive_vertex_count(1), 4);
}

TEST(ElementTopologyTest, Validation) {
    ElementTopology topo;
    topo.set_point_count(3);
    topo.set_vertex_count(3);
    topo.set_primitive_count(1);

    // Valid triangle
    topo.set_vertex_point(0, 0);
    topo.set_vertex_point(1, 1);
    topo.set_vertex_point(2, 2);
    topo.set_primitive_vertices(0, {0, 1, 2});

    EXPECT_TRUE(topo.is_valid());

    // Invalid: vertex references non-existent point
    topo.set_vertex_point(2, 999);
    EXPECT_FALSE(topo.is_valid());
}
```

**Success Criteria**:
- ✅ All tests pass
- ✅ Can represent triangle meshes
- ✅ Can represent quad meshes (N-gons)
- ✅ Validation catches errors

**Time estimate**: 2 days

---

### Task 1.2: Define Attribute Type System

**File**: `nodeflux_core/include/nodeflux/core/attribute_types.hpp`

**Code to write**:

```cpp
#pragma once
#include "nodeflux/core/types.hpp"  // Vector3, etc.
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <string>
#include <variant>

namespace nodeflux::core {

/**
 * @brief Supported attribute data types
 */
enum class AttributeType {
    FLOAT,          // Single float
    INT,            // Single int
    VECTOR2,        // 2D vector (UV, etc.)
    VECTOR3,        // 3D vector (position, normal, color)
    VECTOR4,        // 4D vector (RGBA, quaternion)
    MATRIX3,        // 3×3 matrix
    MATRIX4,        // 4×4 matrix (transforms)
    QUATERNION,     // Rotation
    STRING,         // String metadata
};

/**
 * @brief Get size in bytes for a given type
 */
inline size_t attribute_type_size(AttributeType type) {
    switch (type) {
        case AttributeType::FLOAT:      return sizeof(float);
        case AttributeType::INT:        return sizeof(int);
        case AttributeType::VECTOR2:    return sizeof(Eigen::Vector2f);
        case AttributeType::VECTOR3:    return sizeof(Vector3);
        case AttributeType::VECTOR4:    return sizeof(Eigen::Vector4f);
        case AttributeType::MATRIX3:    return sizeof(Eigen::Matrix3f);
        case AttributeType::MATRIX4:    return sizeof(Eigen::Matrix4f);
        case AttributeType::QUATERNION: return sizeof(Eigen::Quaternionf);
        case AttributeType::STRING:     return sizeof(std::string);
    }
    return 0;
}

/**
 * @brief Get tuple size (number of components)
 */
inline int attribute_type_tuple_size(AttributeType type) {
    switch (type) {
        case AttributeType::FLOAT:      return 1;
        case AttributeType::INT:        return 1;
        case AttributeType::VECTOR2:    return 2;
        case AttributeType::VECTOR3:    return 3;
        case AttributeType::VECTOR4:    return 4;
        case AttributeType::MATRIX3:    return 9;
        case AttributeType::MATRIX4:    return 16;
        case AttributeType::QUATERNION: return 4;
        case AttributeType::STRING:     return 1;
    }
    return 0;
}

/**
 * @brief Get human-readable name
 */
inline const char* attribute_type_name(AttributeType type) {
    switch (type) {
        case AttributeType::FLOAT:      return "float";
        case AttributeType::INT:        return "int";
        case AttributeType::VECTOR2:    return "vector2";
        case AttributeType::VECTOR3:    return "vector3";
        case AttributeType::VECTOR4:    return "vector4";
        case AttributeType::MATRIX3:    return "matrix3";
        case AttributeType::MATRIX4:    return "matrix4";
        case AttributeType::QUATERNION: return "quaternion";
        case AttributeType::STRING:     return "string";
    }
    return "unknown";
}

/**
 * @brief C++ type → AttributeType mapping
 */
template<typename T> struct TypeToAttributeType;

template<> struct TypeToAttributeType<float> {
    static constexpr AttributeType value = AttributeType::FLOAT;
};

template<> struct TypeToAttributeType<int> {
    static constexpr AttributeType value = AttributeType::INT;
};

template<> struct TypeToAttributeType<Eigen::Vector2f> {
    static constexpr AttributeType value = AttributeType::VECTOR2;
};

template<> struct TypeToAttributeType<Vector3> {
    static constexpr AttributeType value = AttributeType::VECTOR3;
};

template<> struct TypeToAttributeType<Eigen::Vector4f> {
    static constexpr AttributeType value = AttributeType::VECTOR4;
};

template<> struct TypeToAttributeType<Eigen::Matrix3f> {
    static constexpr AttributeType value = AttributeType::MATRIX3;
};

template<> struct TypeToAttributeType<Eigen::Matrix4f> {
    static constexpr AttributeType value = AttributeType::MATRIX4;
};

template<> struct TypeToAttributeType<Eigen::Quaternionf> {
    static constexpr AttributeType value = AttributeType::QUATERNION;
};

template<> struct TypeToAttributeType<std::string> {
    static constexpr AttributeType value = AttributeType::STRING;
};

/**
 * @brief Interpolation modes for attribute blending
 */
enum class InterpolationMode {
    NONE,           // Don't interpolate (e.g., discrete IDs)
    LINEAR,         // Linear blend (positions, colors)
    SMOOTH,         // Smooth interpolation (Catmull-Rom)
    QUATERNION,     // Spherical interpolation (rotations)
    DISCRETE,       // Nearest neighbor
};

} // namespace nodeflux::core
```

**Tests**: `tests/test_attribute_types.cpp`

```cpp
#include "nodeflux/core/attribute_types.hpp"
#include <gtest/gtest.h>

using namespace nodeflux::core;

TEST(AttributeTypesTest, TypeSize) {
    EXPECT_EQ(attribute_type_size(AttributeType::FLOAT), sizeof(float));
    EXPECT_EQ(attribute_type_size(AttributeType::VECTOR3), sizeof(Vector3));
    EXPECT_EQ(attribute_type_size(AttributeType::MATRIX4), sizeof(Eigen::Matrix4f));
}

TEST(AttributeTypesTest, TupleSize) {
    EXPECT_EQ(attribute_type_tuple_size(AttributeType::FLOAT), 1);
    EXPECT_EQ(attribute_type_tuple_size(AttributeType::VECTOR3), 3);
    EXPECT_EQ(attribute_type_tuple_size(AttributeType::MATRIX3), 9);
}

TEST(AttributeTypesTest, TypeMapping) {
    EXPECT_EQ(TypeToAttributeType<float>::value, AttributeType::FLOAT);
    EXPECT_EQ(TypeToAttributeType<Vector3>::value, AttributeType::VECTOR3);
    EXPECT_EQ(TypeToAttributeType<Eigen::Quaternionf>::value, AttributeType::QUATERNION);
}
```

**Success Criteria**:
- ✅ All types defined
- ✅ Type mapping works
- ✅ Helper functions correct

**Time estimate**: 1 day

---

### Task 1.3: Standard Attribute Names

**File**: `nodeflux_core/include/nodeflux/core/standard_attributes.hpp`

**Code to write**:

```cpp
#pragma once

namespace nodeflux::core {
namespace StandardAttributes {

// ============================================================================
// Geometry Attributes
// ============================================================================

/// Position (Vector3) - point attribute
constexpr const char* POSITION = "P";

/// Normal (Vector3) - point or vertex attribute
constexpr const char* NORMAL = "N";

/// Color (Vector3 - RGB) - point or vertex attribute
constexpr const char* COLOR = "Cd";

/// Alpha/Opacity (float) - point or vertex attribute
constexpr const char* ALPHA = "Alpha";

/// Texture coordinates (Vector2) - vertex attribute
constexpr const char* UV = "uv";

// ============================================================================
// Transform Attributes
// ============================================================================

/// Velocity (Vector3) - point attribute
constexpr const char* VELOCITY = "v";

/// Orientation (Quaternion) - point attribute
constexpr const char* ORIENT = "orient";

/// Point scale (float) - point attribute
constexpr const char* POINT_SCALE = "pscale";

/// Up vector (Vector3) - point attribute
constexpr const char* UP = "up";

/// Transform matrix (Matrix4) - point or primitive attribute
constexpr const char* TRANSFORM = "transform";

// ============================================================================
// Topology Attributes
// ============================================================================

/// Point ID (int) - point attribute
constexpr const char* POINT_ID = "id";

/// Primitive ID (int) - primitive attribute
constexpr const char* PRIMITIVE_ID = "primid";

/// Vertex ID (int) - vertex attribute
constexpr const char* VERTEX_ID = "vtxid";

/// Instance ID (int) - point or vertex attribute (for instancing)
constexpr const char* INSTANCE_ID = "instance_id";

// ============================================================================
// Material Attributes
// ============================================================================

/// Material name (string) - primitive or detail attribute
constexpr const char* MATERIAL = "material";

/// Roughness (float) - point or primitive attribute
constexpr const char* ROUGHNESS = "rough";

/// Metallic (float) - point or primitive attribute
constexpr const char* METALLIC = "metallic";

/// Emission (Vector3) - point or primitive attribute
constexpr const char* EMISSION = "emit";

// ============================================================================
// Metadata Attributes
// ============================================================================

/// Object name (string) - detail attribute
constexpr const char* NAME = "name";

/// Object class/type (string) - primitive or detail attribute
constexpr const char* CLASS = "class";

/// Path (string) - detail attribute
constexpr const char* PATH = "path";

// ============================================================================
// Simulation Attributes
// ============================================================================

/// Mass (float) - point attribute
constexpr const char* MASS = "mass";

/// Force (Vector3) - point attribute
constexpr const char* FORCE = "force";

/// Acceleration (Vector3) - point attribute
constexpr const char* ACCELERATION = "accel";

/// Age (float) - point attribute
constexpr const char* AGE = "age";

/// Lifetime (float) - point attribute
constexpr const char* LIFETIME = "life";

} // namespace StandardAttributes
} // namespace nodeflux::core
```

**Tests**: `tests/test_standard_attributes.cpp`

```cpp
#include "nodeflux/core/standard_attributes.hpp"
#include <gtest/gtest.h>
#include <string>

using namespace nodeflux::core;

TEST(StandardAttributesTest, GeometryAttributes) {
    EXPECT_STREQ(StandardAttributes::POSITION, "P");
    EXPECT_STREQ(StandardAttributes::NORMAL, "N");
    EXPECT_STREQ(StandardAttributes::COLOR, "Cd");
    EXPECT_STREQ(StandardAttributes::UV, "uv");
}

TEST(StandardAttributesTest, TransformAttributes) {
    EXPECT_STREQ(StandardAttributes::VELOCITY, "v");
    EXPECT_STREQ(StandardAttributes::ORIENT, "orient");
    EXPECT_STREQ(StandardAttributes::POINT_SCALE, "pscale");
}

TEST(StandardAttributesTest, Uniqueness) {
    // Ensure no duplicate names
    std::set<std::string> names = {
        StandardAttributes::POSITION,
        StandardAttributes::NORMAL,
        StandardAttributes::COLOR,
        StandardAttributes::UV,
        StandardAttributes::VELOCITY,
        StandardAttributes::ORIENT,
        // ... all others
    };

    // Count should match number of unique constants
    EXPECT_GT(names.size(), 20);  // We have >20 standard attributes
}
```

**Success Criteria**:
- ✅ All common attributes defined
- ✅ Houdini-compatible names
- ✅ Well-documented

**Time estimate**: 0.5 days

---

## Week 2: Attribute Descriptor & Storage

### Task 2.1: Attribute Descriptor (Schema)

**File**: `nodeflux_core/include/nodeflux/core/attribute_descriptor.hpp`

**Code to write**:

```cpp
#pragma once
#include "nodeflux/core/attribute_types.hpp"
#include "nodeflux/core/element_topology.hpp"
#include <string>
#include <optional>
#include <vector>

namespace nodeflux::core {

/**
 * @brief Attribute descriptor - schema/metadata for an attribute
 *
 * Immutable description of what an attribute is, including:
 * - Name and type
 * - Which element class owns it
 * - Interpolation rules
 * - Default values
 * - Range constraints
 */
class AttributeDescriptor {
public:
    /**
     * @brief Construct descriptor
     */
    AttributeDescriptor(
        std::string name,
        AttributeType type,
        ElementClass owner,
        InterpolationMode interp = InterpolationMode::LINEAR
    );

    // Accessors
    const std::string& name() const { return name_; }
    AttributeType type() const { return type_; }
    ElementClass owner() const { return owner_; }
    InterpolationMode interpolation() const { return interpolation_; }

    int tuple_size() const { return tuple_size_; }
    size_t byte_size() const { return byte_size_; }

    // Metadata
    const std::string& description() const { return description_; }
    void set_description(const std::string& desc) { description_ = desc; }

    // Default value (type-erased as floats)
    const std::vector<float>& default_value() const { return default_value_; }
    void set_default_value(const std::vector<float>& value);

    // Range constraints
    std::optional<float> min_value() const { return min_value_; }
    std::optional<float> max_value() const { return max_value_; }
    void set_range(float min, float max);

    // Versioning (for change tracking)
    uint64_t version() const { return version_; }
    void increment_version() { ++version_; }

    // Type checking
    template<typename T>
    bool is_type() const {
        return TypeToAttributeType<T>::value == type_;
    }

    bool is_required() const { return required_; }
    void set_required(bool req) { required_ = req; }

private:
    std::string name_;
    AttributeType type_;
    ElementClass owner_;
    InterpolationMode interpolation_;

    int tuple_size_;
    size_t byte_size_;

    std::string description_;
    std::vector<float> default_value_;
    std::optional<float> min_value_;
    std::optional<float> max_value_;

    bool required_ = false;
    uint64_t version_ = 0;
};

/**
 * @brief Fluent builder for attribute descriptors
 */
class AttributeDescriptorBuilder {
public:
    AttributeDescriptorBuilder(std::string name, AttributeType type, ElementClass owner);

    AttributeDescriptorBuilder& description(const std::string& desc);
    AttributeDescriptorBuilder& interpolation(InterpolationMode mode);
    AttributeDescriptorBuilder& default_value(const std::vector<float>& value);
    AttributeDescriptorBuilder& range(float min, float max);
    AttributeDescriptorBuilder& required(bool req);

    AttributeDescriptor build() const;

private:
    AttributeDescriptor desc_;
};

/**
 * @brief Helper to create descriptor builders
 */
inline AttributeDescriptorBuilder make_attribute_descriptor(
    std::string name,
    AttributeType type,
    ElementClass owner
) {
    return AttributeDescriptorBuilder(std::move(name), type, owner);
}

} // namespace nodeflux::core
```

**Implementation**: `nodeflux_core/src/core/attribute_descriptor.cpp`

```cpp
#include "nodeflux/core/attribute_descriptor.hpp"

namespace nodeflux::core {

AttributeDescriptor::AttributeDescriptor(
    std::string name,
    AttributeType type,
    ElementClass owner,
    InterpolationMode interp
)
    : name_(std::move(name))
    , type_(type)
    , owner_(owner)
    , interpolation_(interp)
    , tuple_size_(attribute_type_tuple_size(type))
    , byte_size_(attribute_type_size(type))
{
}

void AttributeDescriptor::set_default_value(const std::vector<float>& value) {
    if (value.size() != static_cast<size_t>(tuple_size_)) {
        throw std::invalid_argument("Default value size mismatch");
    }
    default_value_ = value;
}

void AttributeDescriptor::set_range(float min, float max) {
    if (min > max) {
        throw std::invalid_argument("Invalid range: min > max");
    }
    min_value_ = min;
    max_value_ = max;
}

// Builder implementation
AttributeDescriptorBuilder::AttributeDescriptorBuilder(
    std::string name,
    AttributeType type,
    ElementClass owner
)
    : desc_(std::move(name), type, owner)
{
}

AttributeDescriptorBuilder& AttributeDescriptorBuilder::description(const std::string& desc) {
    desc_.set_description(desc);
    return *this;
}

AttributeDescriptorBuilder& AttributeDescriptorBuilder::interpolation(InterpolationMode mode) {
    // Modify descriptor (need friend access or setter)
    // For now, return *this (implement fully later)
    return *this;
}

AttributeDescriptorBuilder& AttributeDescriptorBuilder::default_value(const std::vector<float>& value) {
    desc_.set_default_value(value);
    return *this;
}

AttributeDescriptorBuilder& AttributeDescriptorBuilder::range(float min, float max) {
    desc_.set_range(min, max);
    return *this;
}

AttributeDescriptorBuilder& AttributeDescriptorBuilder::required(bool req) {
    desc_.set_required(req);
    return *this;
}

AttributeDescriptor AttributeDescriptorBuilder::build() const {
    return desc_;
}

} // namespace nodeflux::core
```

**Tests**: `tests/test_attribute_descriptor.cpp`

```cpp
#include "nodeflux/core/attribute_descriptor.hpp"
#include <gtest/gtest.h>

using namespace nodeflux::core;

TEST(AttributeDescriptorTest, BasicConstruction) {
    AttributeDescriptor desc("P", AttributeType::VECTOR3, ElementClass::POINT);

    EXPECT_EQ(desc.name(), "P");
    EXPECT_EQ(desc.type(), AttributeType::VECTOR3);
    EXPECT_EQ(desc.owner(), ElementClass::POINT);
    EXPECT_EQ(desc.tuple_size(), 3);
}

TEST(AttributeDescriptorTest, FluentBuilder) {
    auto desc = make_attribute_descriptor("Cd", AttributeType::VECTOR3, ElementClass::VERTEX)
        .description("Vertex color")
        .default_value({1.0F, 1.0F, 1.0F})  // White
        .range(0.0F, 1.0F)
        .required(true)
        .build();

    EXPECT_EQ(desc.name(), "Cd");
    EXPECT_EQ(desc.description(), "Vertex color");
    EXPECT_TRUE(desc.is_required());
}

TEST(AttributeDescriptorTest, TypeChecking) {
    AttributeDescriptor desc("N", AttributeType::VECTOR3, ElementClass::POINT);

    EXPECT_TRUE(desc.is_type<Vector3>());
    EXPECT_FALSE(desc.is_type<float>());
}

TEST(AttributeDescriptorTest, Versioning) {
    AttributeDescriptor desc("test", AttributeType::FLOAT, ElementClass::POINT);

    uint64_t v1 = desc.version();
    desc.increment_version();
    EXPECT_EQ(desc.version(), v1 + 1);
}
```

**Success Criteria**:
- ✅ Descriptors can be created
- ✅ Builder pattern works
- ✅ Metadata stored correctly
- ✅ Version tracking works

**Time estimate**: 2 days

---

### Task 2.2: Typed Attribute Storage (SoA)

**File**: `nodeflux_core/include/nodeflux/core/attribute_storage.hpp`

**Code to write**:

```cpp
#pragma once
#include "nodeflux/core/attribute_descriptor.hpp"
#include <vector>
#include <span>
#include <memory>
#include <stdexcept>

namespace nodeflux::core {

/**
 * @brief Type-erased interface for attribute storage
 */
class IAttributeStorage {
public:
    virtual ~IAttributeStorage() = default;

    virtual const AttributeDescriptor& descriptor() const = 0;
    virtual size_t size() const = 0;
    virtual void resize(size_t new_size) = 0;
    virtual void clear() = 0;

    virtual void* raw_data() = 0;
    virtual const void* raw_data() const = 0;

    virtual std::unique_ptr<IAttributeStorage> clone() const = 0;
};

/**
 * @brief Typed attribute storage using Structure of Arrays (SoA)
 *
 * Fast, cache-friendly storage for a single attribute across all elements.
 * Stores values in a contiguous std::vector<T>.
 */
template<typename T>
class AttributeStorage : public IAttributeStorage {
public:
    explicit AttributeStorage(AttributeDescriptor descriptor)
        : descriptor_(std::move(descriptor))
    {
        // Initialize with default values if provided
        if (!descriptor_.default_value().empty()) {
            // Convert default value to T
            default_value_ = default_from_floats(descriptor_.default_value());
        }
    }

    // Accessors
    const AttributeDescriptor& descriptor() const override { return descriptor_; }
    size_t size() const override { return data_.size(); }

    // Data access
    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }

    T& at(size_t index) {
        if (index >= data_.size()) {
            throw std::out_of_range("Attribute index out of range");
        }
        return data_[index];
    }

    const T& at(size_t index) const {
        if (index >= data_.size()) {
            throw std::out_of_range("Attribute index out of range");
        }
        return data_[index];
    }

    // Span-based views (zero-cost abstraction)
    std::span<T> values() { return {data_.data(), data_.size()}; }
    std::span<const T> values() const { return {data_.data(), data_.size()}; }

    // Raw vector access (for maximum performance)
    std::vector<T>& data() { return data_; }
    const std::vector<T>& data() const { return data_; }

    // Modification
    void resize(size_t new_size) override {
        data_.resize(new_size, default_value_);
        descriptor_.increment_version();
    }

    void clear() override {
        data_.clear();
        descriptor_.increment_version();
    }

    void set_all(const T& value) {
        std::fill(data_.begin(), data_.end(), value);
        descriptor_.increment_version();
    }

    // Type-erased interface
    void* raw_data() override { return data_.data(); }
    const void* raw_data() const override { return data_.data(); }

    std::unique_ptr<IAttributeStorage> clone() const override {
        auto copy = std::make_unique<AttributeStorage<T>>(descriptor_);
        copy->data_ = data_;
        copy->default_value_ = default_value_;
        return copy;
    }

private:
    AttributeDescriptor descriptor_;
    std::vector<T> data_;
    T default_value_{};  // Default value for resizing

    // Convert descriptor's float default to T
    static T default_from_floats(const std::vector<float>& floats);
};

// Template specializations for default_from_floats
template<>
inline float AttributeStorage<float>::default_from_floats(const std::vector<float>& floats) {
    return floats.empty() ? 0.0F : floats[0];
}

template<>
inline int AttributeStorage<int>::default_from_floats(const std::vector<float>& floats) {
    return floats.empty() ? 0 : static_cast<int>(floats[0]);
}

template<>
inline Vector3 AttributeStorage<Vector3>::default_from_floats(const std::vector<float>& floats) {
    if (floats.size() >= 3) {
        return Vector3(floats[0], floats[1], floats[2]);
    }
    return Vector3::Zero();
}

template<>
inline Eigen::Vector2f AttributeStorage<Eigen::Vector2f>::default_from_floats(const std::vector<float>& floats) {
    if (floats.size() >= 2) {
        return Eigen::Vector2f(floats[0], floats[1]);
    }
    return Eigen::Vector2f::Zero();
}

// Add specializations for other types...

} // namespace nodeflux::core
```

**Tests**: `tests/test_attribute_storage.cpp`

```cpp
#include "nodeflux/core/attribute_storage.hpp"
#include <gtest/gtest.h>

using namespace nodeflux::core;

TEST(AttributeStorageTest, FloatStorage) {
    auto desc = AttributeDescriptor("test", AttributeType::FLOAT, ElementClass::POINT);
    AttributeStorage<float> storage(desc);

    storage.resize(100);
    EXPECT_EQ(storage.size(), 100);

    // Random access
    storage[0] = 1.5F;
    storage[99] = 2.5F;
    EXPECT_EQ(storage[0], 1.5F);
    EXPECT_EQ(storage[99], 2.5F);
}

TEST(AttributeStorageTest, Vector3Storage) {
    auto desc = AttributeDescriptor("P", AttributeType::VECTOR3, ElementClass::POINT);
    AttributeStorage<Vector3> storage(desc);

    storage.resize(50);
    storage[0] = Vector3(1.0, 2.0, 3.0);

    EXPECT_EQ(storage[0].x(), 1.0);
    EXPECT_EQ(storage[0].y(), 2.0);
    EXPECT_EQ(storage[0].z(), 3.0);
}

TEST(AttributeStorageTest, SpanAccess) {
    auto desc = AttributeDescriptor("positions", AttributeType::VECTOR3, ElementClass::POINT);
    AttributeStorage<Vector3> storage(desc);

    storage.resize(10);

    // Iterate via span (fast)
    auto positions = storage.values();
    for (size_t i = 0; i < positions.size(); ++i) {
        positions[i] = Vector3(i, i*2, i*3);
    }

    EXPECT_EQ(storage[5].x(), 5.0);
    EXPECT_EQ(storage[5].y(), 10.0);
}

TEST(AttributeStorageTest, DefaultValue) {
    auto desc = make_attribute_descriptor("Cd", AttributeType::VECTOR3, ElementClass::VERTEX)
        .default_value({1.0F, 1.0F, 1.0F})
        .build();

    AttributeStorage<Vector3> storage(desc);
    storage.resize(10);

    // All values should be default (white)
    EXPECT_EQ(storage[0].x(), 1.0);
    EXPECT_EQ(storage[0].y(), 1.0);
    EXPECT_EQ(storage[0].z(), 1.0);
}

TEST(AttributeStorageTest, Versioning) {
    auto desc = AttributeDescriptor("test", AttributeType::FLOAT, ElementClass::POINT);
    AttributeStorage<float> storage(desc);

    uint64_t v1 = storage.descriptor().version();
    storage.resize(10);
    EXPECT_GT(storage.descriptor().version(), v1);
}

TEST(AttributeStorageTest, Clone) {
    auto desc = AttributeDescriptor("test", AttributeType::FLOAT, ElementClass::POINT);
    AttributeStorage<float> storage(desc);

    storage.resize(5);
    storage[0] = 1.5F;

    auto clone = storage.clone();
    auto* typed_clone = dynamic_cast<AttributeStorage<float>*>(clone.get());
    ASSERT_NE(typed_clone, nullptr);
    EXPECT_EQ((*typed_clone)[0], 1.5F);
}
```

**Success Criteria**:
- ✅ Fast SoA storage
- ✅ Span-based access
- ✅ Type-safe
- ✅ Version tracking
- ✅ Default values work

**Time estimate**: 3 days

---

## Week 3: Integration Infrastructure

### Task 3.1: Enhance GeometryAttributes

**File**: `nodeflux_core/include/nodeflux/core/geometry_attributes.hpp` (MODIFY)

**Changes to make**:

```cpp
#pragma once
#include "nodeflux/core/attribute_descriptor.hpp"
#include "nodeflux/core/attribute_storage.hpp"
#include "nodeflux/core/element_topology.hpp"
#include "nodeflux/core/standard_attributes.hpp"
#include <unordered_map>
#include <memory>
#include <span>

namespace nodeflux::core {

/**
 * @brief Enhanced geometry attribute system
 *
 * NEW FEATURES:
 * - Descriptor-based schema
 * - Fast typed views
 * - Change tracking
 * - Validation
 * - Standard attribute helpers
 */
class GeometryAttributes {
public:
    GeometryAttributes() = default;

    // Topology (NEW)
    ElementTopology& topology() { return topology_; }
    const ElementTopology& topology() const { return topology_; }

    // ============================================================================
    // Attribute Management (Enhanced)
    // ============================================================================

    /**
     * @brief Add attribute with descriptor
     */
    template<typename T>
    void add_attribute(const AttributeDescriptor& desc) {
        validate_descriptor(desc);

        auto storage = std::make_unique<AttributeStorage<T>>(desc);
        storage->resize(get_element_count(desc.owner()));

        std::string key = make_key(desc.name(), desc.owner());
        storage_[key] = std::move(storage);
        descriptors_[key] = desc;
    }

    /**
     * @brief Add attribute with name and type (auto-create descriptor)
     */
    template<typename T>
    void add_attribute(const std::string& name, ElementClass owner) {
        auto desc = AttributeDescriptor(
            name,
            TypeToAttributeType<T>::value,
            owner
        );
        add_attribute<T>(desc);
    }

    /**
     * @brief Check if attribute exists
     */
    bool has_attribute(const std::string& name, ElementClass owner) const {
        return storage_.count(make_key(name, owner)) > 0;
    }

    /**
     * @brief Remove attribute
     */
    bool remove_attribute(const std::string& name, ElementClass owner) {
        std::string key = make_key(name, owner);
        descriptors_.erase(key);
        return storage_.erase(key) > 0;
    }

    // ============================================================================
    // Fast Typed Access (NEW)
    // ============================================================================

    /**
     * @brief Get typed span for fast iteration
     */
    template<typename T>
    std::span<T> get_attribute_view(const std::string& name, ElementClass owner) {
        std::string key = make_key(name, owner);
        auto it = storage_.find(key);

        if (it == storage_.end()) {
            throw std::runtime_error("Attribute not found: " + name);
        }

        auto* storage = dynamic_cast<AttributeStorage<T>*>(it->second.get());
        if (!storage) {
            throw std::runtime_error("Attribute type mismatch: " + name);
        }

        return storage->values();
    }

    template<typename T>
    std::span<const T> get_attribute_view(const std::string& name, ElementClass owner) const {
        std::string key = make_key(name, owner);
        auto it = storage_.find(key);

        if (it == storage_.end()) {
            throw std::runtime_error("Attribute not found: " + name);
        }

        auto* storage = dynamic_cast<const AttributeStorage<T>*>(it->second.get());
        if (!storage) {
            throw std::runtime_error("Attribute type mismatch: " + name);
        }

        return storage->values();
    }

    // ============================================================================
    // Standard Attribute Helpers (NEW)
    // ============================================================================

    /**
     * @brief Get positions (standard "P" attribute)
     */
    std::span<Vector3> get_positions() {
        if (!has_attribute(StandardAttributes::POSITION, ElementClass::POINT)) {
            throw std::runtime_error("Position attribute 'P' not found");
        }
        return get_attribute_view<Vector3>(StandardAttributes::POSITION, ElementClass::POINT);
    }

    std::span<const Vector3> get_positions() const {
        if (!has_attribute(StandardAttributes::POSITION, ElementClass::POINT)) {
            throw std::runtime_error("Position attribute 'P' not found");
        }
        return get_attribute_view<Vector3>(StandardAttributes::POSITION, ElementClass::POINT);
    }

    /**
     * @brief Get normals (standard "N" attribute)
     */
    std::span<Vector3> get_normals() {
        return get_attribute_view<Vector3>(StandardAttributes::NORMAL, ElementClass::POINT);
    }

    std::span<const Vector3> get_normals() const {
        return get_attribute_view<Vector3>(StandardAttributes::NORMAL, ElementClass::POINT);
    }

    /**
     * @brief Get colors (standard "Cd" attribute)
     */
    std::span<Vector3> get_colors() {
        return get_attribute_view<Vector3>(StandardAttributes::COLOR, ElementClass::POINT);
    }

    // Add more standard attribute helpers...

    // ============================================================================
    // Descriptor Access (NEW)
    // ============================================================================

    /**
     * @brief Get descriptor for an attribute
     */
    const AttributeDescriptor& get_descriptor(const std::string& name, ElementClass owner) const {
        std::string key = make_key(name, owner);
        auto it = descriptors_.find(key);
        if (it == descriptors_.end()) {
            throw std::runtime_error("Attribute descriptor not found: " + name);
        }
        return it->second;
    }

    /**
     * @brief Get all attribute names for an element class
     */
    std::vector<std::string> get_attribute_names(ElementClass owner) const {
        std::vector<std::string> names;
        for (const auto& [key, desc] : descriptors_) {
            if (desc.owner() == owner) {
                names.push_back(desc.name());
            }
        }
        return names;
    }

    // ============================================================================
    // Validation (NEW)
    // ============================================================================

    /**
     * @brief Validate all attributes have correct sizes
     */
    bool validate() const {
        for (const auto& [key, storage] : storage_) {
            const auto& desc = descriptors_.at(key);
            size_t expected = get_element_count(desc.owner());

            if (storage->size() != expected) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Resize all attributes when topology changes
     */
    void resize_to_topology() {
        for (const auto& [key, storage] : storage_) {
            const auto& desc = descriptors_.at(key);
            size_t expected = get_element_count(desc.owner());
            storage->resize(expected);
        }
    }

private:
    ElementTopology topology_;
    std::unordered_map<std::string, std::unique_ptr<IAttributeStorage>> storage_;
    std::unordered_map<std::string, AttributeDescriptor> descriptors_;

    // Helper: make unique key from name + owner
    static std::string make_key(const std::string& name, ElementClass owner) {
        return name + "_" + std::to_string(static_cast<int>(owner));
    }

    // Helper: get element count for a class
    size_t get_element_count(ElementClass cls) const {
        switch (cls) {
            case ElementClass::POINT:     return topology_.point_count();
            case ElementClass::VERTEX:    return topology_.vertex_count();
            case ElementClass::PRIMITIVE: return topology_.primitive_count();
            case ElementClass::DETAIL:    return 1;
        }
        return 0;
    }

    // Helper: validate descriptor
    void validate_descriptor(const AttributeDescriptor& desc) const {
        // Check for name conflicts
        if (has_attribute(desc.name(), desc.owner())) {
            throw std::runtime_error("Attribute already exists: " + desc.name());
        }
    }
};

} // namespace nodeflux::core
```

**Tests**: `tests/test_geometry_attributes_enhanced.cpp`

```cpp
#include "nodeflux/core/geometry_attributes.hpp"
#include <gtest/gtest.h>

using namespace nodeflux::core;

TEST(GeometryAttributesEnhancedTest, AddAndAccessAttribute) {
    GeometryAttributes attrs;
    attrs.topology().set_point_count(100);

    // Add position attribute
    attrs.add_attribute<Vector3>(StandardAttributes::POSITION, ElementClass::POINT);

    EXPECT_TRUE(attrs.has_attribute(StandardAttributes::POSITION, ElementClass::POINT));

    // Get typed view
    auto positions = attrs.get_attribute_view<Vector3>(StandardAttributes::POSITION, ElementClass::POINT);
    EXPECT_EQ(positions.size(), 100);

    // Modify
    positions[0] = Vector3(1.0, 2.0, 3.0);
    EXPECT_EQ(positions[0].x(), 1.0);
}

TEST(GeometryAttributesEnhancedTest, StandardAttributeHelpers) {
    GeometryAttributes attrs;
    attrs.topology().set_point_count(50);

    attrs.add_attribute<Vector3>(StandardAttributes::POSITION, ElementClass::POINT);
    attrs.add_attribute<Vector3>(StandardAttributes::NORMAL, ElementClass::POINT);

    // Use convenience helpers
    auto positions = attrs.get_positions();
    auto normals = attrs.get_normals();

    positions[10] = Vector3(1.0, 0.0, 0.0);
    normals[10] = Vector3(0.0, 1.0, 0.0);

    EXPECT_EQ(attrs.get_positions()[10].x(), 1.0);
    EXPECT_EQ(attrs.get_normals()[10].y(), 1.0);
}

TEST(GeometryAttributesEnhancedTest, DescriptorAccess) {
    GeometryAttributes attrs;
    attrs.topology().set_point_count(10);

    auto desc = make_attribute_descriptor("test", AttributeType::FLOAT, ElementClass::POINT)
        .description("Test attribute")
        .range(0.0F, 1.0F)
        .build();

    attrs.add_attribute<float>(desc);

    const auto& retrieved = attrs.get_descriptor("test", ElementClass::POINT);
    EXPECT_EQ(retrieved.name(), "test");
    EXPECT_EQ(retrieved.description(), "Test attribute");
}

TEST(GeometryAttributesEnhancedTest, AutoResize) {
    GeometryAttributes attrs;
    attrs.topology().set_point_count(10);

    attrs.add_attribute<Vector3>(StandardAttributes::POSITION, ElementClass::POINT);

    // Attribute should auto-size to 10
    EXPECT_EQ(attrs.get_positions().size(), 10);

    // Change topology
    attrs.topology().set_point_count(20);
    attrs.resize_to_topology();

    // Attribute should now be 20
    EXPECT_EQ(attrs.get_positions().size(), 20);
}

TEST(GeometryAttributesEnhancedTest, TypeSafety) {
    GeometryAttributes attrs;
    attrs.topology().set_point_count(10);

    attrs.add_attribute<Vector3>(StandardAttributes::POSITION, ElementClass::POINT);

    // Correct type - should work
    EXPECT_NO_THROW(attrs.get_attribute_view<Vector3>(StandardAttributes::POSITION, ElementClass::POINT));

    // Wrong type - should throw
    EXPECT_THROW(attrs.get_attribute_view<float>(StandardAttributes::POSITION, ElementClass::POINT),
                 std::runtime_error);
}
```

**Success Criteria**:
- ✅ GeometryAttributes uses new infrastructure
- ✅ Fast span-based access works
- ✅ Standard attribute helpers work
- ✅ Type safety enforced
- ✅ All tests pass

**Time estimate**: 3 days

---

### Task 3.2: Create Integration Tests

**File**: `tests/test_attribute_system_integration.cpp`

**Test end-to-end workflows**:

```cpp
#include "nodeflux/core/geometry_attributes.hpp"
#include "nodeflux/core/standard_attributes.hpp"
#include <gtest/gtest.h>

using namespace nodeflux::core;

TEST(AttributeSystemIntegrationTest, TriangleMesh) {
    // Create a simple triangle
    GeometryAttributes geo;

    // Set topology
    geo.topology().set_point_count(3);
    geo.topology().set_vertex_count(3);
    geo.topology().set_primitive_count(1);

    // Map vertices to points (1:1 for this simple case)
    geo.topology().set_vertex_point(0, 0);
    geo.topology().set_vertex_point(1, 1);
    geo.topology().set_vertex_point(2, 2);

    // Define triangle primitive
    geo.topology().set_primitive_vertices(0, {0, 1, 2});

    // Add position attribute
    geo.add_attribute<Vector3>(StandardAttributes::POSITION, ElementClass::POINT);

    // Set positions
    auto positions = geo.get_positions();
    positions[0] = Vector3(0.0, 0.0, 0.0);
    positions[1] = Vector3(1.0, 0.0, 0.0);
    positions[2] = Vector3(0.0, 1.0, 0.0);

    // Validate
    EXPECT_TRUE(geo.topology().is_valid());
    EXPECT_TRUE(geo.validate());

    // Verify
    EXPECT_EQ(geo.topology().point_count(), 3);
    EXPECT_EQ(geo.get_positions()[1].x(), 1.0);
}

TEST(AttributeSystemIntegrationTest, QuadMesh) {
    // Create a quad (N-gon)
    GeometryAttributes geo;

    geo.topology().set_point_count(4);
    geo.topology().set_vertex_count(4);
    geo.topology().set_primitive_count(1);

    // Map vertices to points
    for (int i = 0; i < 4; ++i) {
        geo.topology().set_vertex_point(i, i);
    }

    // Quad has 4 vertices
    geo.topology().set_primitive_vertices(0, {0, 1, 2, 3});

    // Add attributes
    geo.add_attribute<Vector3>(StandardAttributes::POSITION, ElementClass::POINT);
    geo.add_attribute<Vector3>(StandardAttributes::COLOR, ElementClass::VERTEX);

    // Set positions
    auto positions = geo.get_positions();
    positions[0] = Vector3(-1, -1, 0);
    positions[1] = Vector3( 1, -1, 0);
    positions[2] = Vector3( 1,  1, 0);
    positions[3] = Vector3(-1,  1, 0);

    // Set per-vertex colors (different from point colors!)
    auto colors = geo.get_attribute_view<Vector3>(StandardAttributes::COLOR, ElementClass::VERTEX);
    colors[0] = Vector3(1, 0, 0);  // Red
    colors[1] = Vector3(0, 1, 0);  // Green
    colors[2] = Vector3(0, 0, 1);  // Blue
    colors[3] = Vector3(1, 1, 0);  // Yellow

    EXPECT_TRUE(geo.topology().is_valid());
    EXPECT_EQ(geo.topology().get_primitive_vertex_count(0), 4);
}

TEST(AttributeSystemIntegrationTest, CubeWithSplitNormals) {
    // Cube: 8 points, 24 vertices (6 faces × 4 corners), 6 primitives
    GeometryAttributes geo;

    geo.topology().set_point_count(8);
    geo.topology().set_vertex_count(24);
    geo.topology().set_primitive_count(6);

    // Set vertex → point mapping (each point referenced 3 times)
    // Front face vertices 0-3 → points 0-3
    geo.topology().set_vertex_point(0, 0);
    geo.topology().set_vertex_point(1, 1);
    geo.topology().set_vertex_point(2, 2);
    geo.topology().set_vertex_point(3, 3);
    // ... etc for other faces

    // Add position (per-point) and normal (per-vertex) attributes
    geo.add_attribute<Vector3>(StandardAttributes::POSITION, ElementClass::POINT);
    geo.add_attribute<Vector3>(StandardAttributes::NORMAL, ElementClass::VERTEX);

    // Each vertex can have different normal even if they share the same point!
    auto normals = geo.get_attribute_view<Vector3>(StandardAttributes::NORMAL, ElementClass::VERTEX);

    // Front face normals
    normals[0] = Vector3(0, 0, 1);
    normals[1] = Vector3(0, 0, 1);
    normals[2] = Vector3(0, 0, 1);
    normals[3] = Vector3(0, 0, 1);

    // Right face normals (different for same points!)
    normals[4] = Vector3(1, 0, 0);
    // ... etc

    EXPECT_EQ(geo.topology().point_count(), 8);
    EXPECT_EQ(geo.topology().vertex_count(), 24);
}

TEST(AttributeSystemIntegrationTest, PerformanceComparison) {
    // Compare old variant-based vs new SoA performance
    const size_t COUNT = 1000000;

    GeometryAttributes geo;
    geo.topology().set_point_count(COUNT);
    geo.add_attribute<Vector3>(StandardAttributes::POSITION, ElementClass::POINT);

    auto positions = geo.get_positions();

    // Benchmark: set all positions
    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < COUNT; ++i) {
        positions[i] = Vector3(i, i*2, i*3);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should be FAST (< 50ms for 1M elements)
    EXPECT_LT(duration.count(), 50);

    std::cout << "SoA set " << COUNT << " positions in " << duration.count() << "ms\n";
}
```

**Success Criteria**:
- ✅ Can represent triangle meshes
- ✅ Can represent quad/N-gon meshes
- ✅ Split normals work (point vs vertex)
- ✅ Performance is good (<50ms for 1M elements)

**Time estimate**: 2 days

---

## Week 4: Documentation & Phase 1 Wrap-Up

### Task 4.1: Write Comprehensive Documentation

**File**: `docs/attribute_system_guide.md`

**Contents**:
1. Architecture overview
2. Core concepts (Point vs Vertex)
3. API reference
4. Code examples
5. Best practices
6. Performance tips
7. Migration guide (for Phase 2)

**Time estimate**: 3 days

---

### Task 4.2: Benchmark Suite

**File**: `tests/benchmark_attributes.cpp`

**Benchmarks to include**:
- Attribute creation
- Attribute access (random vs sequential)
- Attribute iteration
- Memory usage
- Comparison: old variant system vs new SoA

**Time estimate**: 2 days

---

## Phase 1 Summary

**Deliverables** (Week 1-4):
- ✅ 4 new header files (topology, types, descriptor, storage)
- ✅ Enhanced GeometryAttributes class
- ✅ Comprehensive test suite (>50 tests)
- ✅ Integration tests
- ✅ Performance benchmarks
- ✅ Documentation

**What's NOT done yet**:
- ❌ No changes to GeometryData
- ❌ No SOP migrations
- ❌ Old system still in place

**Next Phase**: Integrate into GeometryData and migrate SOPs

---

# Phase 2: Integration (Weeks 5-8)

**Goal**: Remove GeometryData wrapper and use GeometryContainer directly in all SOPs

**Strategy**: Direct replacement - eliminate the wrapper layer entirely

---

## Week 5: Direct GeometryContainer Integration

### Task 5.1: Update SOP Node Base Class

**Strategy**: Change SOPNode to use GeometryContainer directly, remove GeometryData

**File**: `nodeflux_core/include/nodeflux/sop/sop_node.hpp` (MODIFY)

**Changes**:

```cpp
// OLD:
virtual std::shared_ptr<GeometryData> execute() = 0;

std::shared_ptr<GeometryData> cook() {
    // ... caching logic
    return execute();
}

// NEW:
virtual std::shared_ptr<core::GeometryContainer> execute() = 0;

std::shared_ptr<core::GeometryContainer> cook() {
    // ... caching logic (unchanged)
    return execute();
}
```

**File**: `nodeflux_core/include/nodeflux/sop/node_port.hpp` (MODIFY)

**Changes**:

```cpp
// OLD:
class NodePort {
    mutable std::shared_ptr<GeometryData> cached_data_;

    std::shared_ptr<GeometryData> get_data() const;
    void set_data(std::shared_ptr<GeometryData> data);
};

// NEW:
class NodePort {
    mutable std::shared_ptr<core::GeometryContainer> cached_data_;

    std::shared_ptr<core::GeometryContainer> get_data() const;
    void set_data(std::shared_ptr<core::GeometryContainer> data);
};
```

**Success Criteria**:
- ✅ SOPNode base class uses GeometryContainer
- ✅ NodePort stores GeometryContainer
- ✅ No GeometryData references in base classes

**Time estimate**: 1 day

---

### Task 5.2: Create SOP Utilities (No Mesh/GeometryData conversions!)

**File**: `nodeflux_core/include/nodeflux/sop/sop_utils.hpp` (NEW)

**Helper functions - GeometryContainer only**:

```cpp
#pragma once

#include "nodeflux/core/geometry_container.hpp"
#include <span>

namespace nodeflux::sop {

/**
 * @brief Initialize standard mesh attributes
 */
inline void ensure_standard_attributes(core::GeometryContainer* container) {
    // Ensure position attribute exists
    if (!container->has_point_attribute(core::standard_attrs::P)) {
        container->add_point_attribute(core::standard_attrs::P,
                                      core::AttributeType::VEC3F);
    }
}

/**
 * @brief Get positions as span for fast iteration
 */
inline std::span<core::Vec3f> get_positions(core::GeometryContainer* container) {
    auto* positions = container->get_point_attribute_typed<core::Vec3f>(
        core::standard_attrs::P);
    if (!positions) {
        throw std::runtime_error("Position attribute 'P' not found");
    }
    return std::span<core::Vec3f>(positions->data(), positions->size());
}

inline std::span<const core::Vec3f> get_positions(const core::GeometryContainer* container) {
    auto* positions = container->get_point_attribute_typed<core::Vec3f>(
        core::standard_attrs::P);
    if (!positions) {
        throw std::runtime_error("Position attribute 'P' not found");
    }
    return std::span<const core::Vec3f>(positions->data(), positions->size());
}

/**
 * @brief Get normals as span (creates if doesn't exist)
 */
inline std::span<core::Vec3f> get_or_create_normals(core::GeometryContainer* container) {
    if (!container->has_point_attribute(core::standard_attrs::N)) {
        container->add_point_attribute(core::standard_attrs::N,
                                      core::AttributeType::VEC3F);
    }

    auto* normals = container->get_point_attribute_typed<core::Vec3f>(
        core::standard_attrs::N);
    return std::span<core::Vec3f>(normals->data(), normals->size());
}

/**
 * @brief Get colors as span (creates if doesn't exist)
 */
inline std::span<core::Vec3f> get_or_create_colors(core::GeometryContainer* container) {
    if (!container->has_point_attribute(core::standard_attrs::Cd)) {
        container->add_point_attribute(core::standard_attrs::Cd,
                                      core::AttributeType::VEC3F);
    }

    auto* colors = container->get_point_attribute_typed<core::Vec3f>(
        core::standard_attrs::Cd);
    return std::span<core::Vec3f>(colors->data(), colors->size());
}

/**
 * @brief Compute face normals for a geometry
 */
inline void compute_face_normals(core::GeometryContainer* container) {
    auto positions = get_positions(container);
    const auto& topo = container->topology();

    // Add primitive normal attribute
    if (!container->has_primitive_attribute(core::standard_attrs::N)) {
        container->add_primitive_attribute(core::standard_attrs::N,
                                          core::AttributeType::VEC3F);
    }

    auto* normals = container->get_primitive_attribute_typed<core::Vec3f>(
        core::standard_attrs::N);

    // Compute normal for each face
    for (size_t i = 0; i < topo.primitive_count(); ++i) {
        auto verts = topo.primitive_vertices(i);
        if (verts.size() >= 3) {
            const auto& p0 = positions[verts[0]];
            const auto& p1 = positions[verts[1]];
            const auto& p2 = positions[verts[2]];

            core::Vec3f edge1(p1.x() - p0.x(), p1.y() - p0.y(), p1.z() - p0.z());
            core::Vec3f edge2(p2.x() - p0.x(), p2.y() - p0.y(), p2.z() - p0.z());

            // Cross product
            core::Vec3f normal(
                edge1.y() * edge2.z() - edge1.z() * edge2.y(),
                edge1.z() * edge2.x() - edge1.x() * edge2.z(),
                edge1.x() * edge2.y() - edge1.y() * edge2.x()
            );

            // Normalize
            float len = std::sqrt(normal.x()*normal.x() +
                                 normal.y()*normal.y() +
                                 normal.z()*normal.z());
            if (len > 0.0f) {
                normal.x() /= len;
                normal.y() /= len;
                normal.z() /= len;
            }

            (*normals)[i] = normal;
        }
    }
}

/**
 * @brief Compute smooth vertex normals (average of adjacent face normals)
 */
inline void compute_vertex_normals(core::GeometryContainer* container) {
    // First compute face normals
    compute_face_normals(container);

    auto* face_normals = container->get_primitive_attribute_typed<core::Vec3f>(
        core::standard_attrs::N);

    // Create vertex normals (point attribute)
    auto vertex_normals = get_or_create_normals(container);

    // Zero out
    for (auto& n : vertex_normals) {
        n = core::Vec3f(0, 0, 0);
    }

    // Accumulate face normals to vertices
    const auto& topo = container->topology();
    for (size_t i = 0; i < topo.primitive_count(); ++i) {
        auto verts = topo.primitive_vertices(i);
        const auto& fn = (*face_normals)[i];

        for (int v : verts) {
            vertex_normals[v].x() += fn.x();
            vertex_normals[v].y() += fn.y();
            vertex_normals[v].z() += fn.z();
        }
    }

    // Normalize
    for (auto& n : vertex_normals) {
        float len = std::sqrt(n.x()*n.x() + n.y()*n.y() + n.z()*n.z());
        if (len > 0.0f) {
            n.x() /= len;
            n.y() /= len;
            n.z() /= len;
        }
    }
}

} // namespace nodeflux::sop
```

**Success Criteria**:
- ✅ Helper functions simplify SOP migration
- ✅ NO Mesh conversions - pure GeometryContainer
- ✅ Includes normal computation (replaces Mesh functionality)
- ✅ Standard attribute access is easy

**Time estimate**: 1 day;
            container_.add_polygon(face_verts);
        }

        // Initialize position attribute
        if (!container_.has_point_attribute(core::standard_attrs::P)) {
            container_.add_point_attribute(core::standard_attrs::P,
                                          core::AttributeType::VEC3F);
        }

        // Copy positions
        auto* positions = container_.get_point_attribute_typed<core::Vec3f>(
            core::standard_attrs::P);
        for (int i = 0; i < vertices.rows(); ++i) {
            (*positions)[i] = core::Vec3f(
                vertices(i, 0), vertices(i, 1), vertices(i, 2));
        }
    }

    /**
     * @brief Sync container positions back to mesh
     */
    void sync_container_to_mesh() {
        if (!mesh_data_) return;
        if (!container_.has_point_attribute(core::standard_attrs::P)) return;

        auto* positions = container_.get_point_attribute_typed<core::Vec3f>(
---

## Week 6-7: SOP Migration (First Half)

**Strategy**: Migrate simplest SOPs first, validate, then move to complex ones

### Migration Order (9 SOPs in weeks 6-7)

**Week 6**:
1. ✅ ScatterSOP - Only creates attributes, no reads
2. ✅ LineSOP - Simple generator
3. ✅ MirrorSOP - Copies attributes without modification
4. ✅ TransformSOP - Modifies "P" only

**Week 7**:
5. ✅ NoiseDisplacementSOP - Reads/writes "P"
6. ✅ ArraySOP - Already uses attributes well
7. ✅ LaplacianSOP - Modifies "P" based on topology
8. ✅ ResampleSOP - Attribute interpolation
9. ✅ SubdivisionSOP - Complex attribute interpolation

### Example Migration: TransformSOP

**BEFORE** (current code):
```cpp
std::shared_ptr<GeometryData> TransformSOP::execute() {
    auto input = get_input_data(0);
    auto output = std::make_shared<GeometryData>(*input);

    auto mesh = output->get_mesh();
    auto& vertices = mesh->vertices();

    for (int i = 0; i < vertices.rows(); ++i) {
        vertices.row(i) += translation_.transpose();
    }

    return output;
}
```

**AFTER** (migrated code):
```cpp
std::shared_ptr<core::GeometryContainer> TransformSOP::execute() {
    auto input = get_input_data(0);
    auto output = std::make_shared<core::GeometryContainer>(*input);

    // Use new container API directly - much simpler!
    auto positions = sop::get_positions(output.get());

    // Transform positions
    for (auto& pos : positions) {
        pos.x() += translation_.x();
        pos.y() += translation_.y();
        pos.z() += translation_.z();
    }

    return output;
}
```

**Per-SOP Checklist**:
- [ ] Change return type to `std::shared_ptr<core::GeometryContainer>`
- [ ] Use GeometryContainer directly (no wrapper)
- [ ] Use typed spans instead of mesh vertices
- [ ] Use helper functions from sop_utils.hpp
- [ ] Update tests
- [ ] Verify output matches old behavior

**Time estimate per SOP**: 0.5-1 day (4-5 SOPs per week)

**Important**: SOPs now work directly with GeometryContainer - no Mesh, no GeometryData!

---

## Week 8: SOP Migration (Second Half) + Testing

**Week 8 - Remaining 9 SOPs**:
10. ✅ ExtrudeSOP - Complex attribute transfer
11. ✅ PolyExtrudeSOP - Per-face operations
12. ✅ BooleanSOP - Attribute merging from two inputs
13. ✅ CopyToPointsSOP - Instance attributes
14. ✅ SphereSOP - Generator with standard attributes
15. ✅ BoxSOP, CylinderSOP, PlaneSOP, TorusSOP - Other generators
16. ✅ FileSOP - Load and initialize attributes

### Task 8.1: Comprehensive Testing

**Full regression test suite**:
- All 18 SOPs with new system
- Performance benchmarks
- Memory usage comparison

**Success Criteria**:
- ✅ All SOPs migrated
- ✅ All tests pass
- ✅ No performance regression
- ✅ Clean codebase (no Mesh or GeometryData)

**Time estimate**: Week 8

---

# Phase 3: Performance & Polish (Weeks 9-12)

**Goal**: Cleanup, optimize, add advanced features

---

## Week 9: Cleanup and Optimization

### Task 9.1: Delete Legacy Code

**Delete these files entirely**:
- `nodeflux_core/include/nodeflux/core/mesh.hpp`
- `nodeflux_core/src/core/mesh.cpp`
- `nodeflux_core/include/nodeflux/sop/geometry_data.hpp`
- `nodeflux_core/src/sop/geometry_data.cpp` (if exists)

**Update ExecutionEngine**:
- Remove all Mesh conversion helpers
- Use GeometryContainer directly throughout

**Success Criteria**:
- ✅ No Mesh or GeometryData references remain
- ✅ All SOPs work with GeometryContainer
- ✅ Codebase is cleaner
- ✅ Build succeeds

**Time estimate**: 1 day

---

### Task 9.2: Optimize Hot Paths

**Profile and optimize**:
- Attribute access in loops
- Attribute creation
- Memory allocations

**Techniques**:
- Inline critical functions
- Reserve capacity
- Minimize allocations

**Success Criteria**:
- ✅ No performance regression vs old system
- ✅ Ideally 10-50% faster

**Time estimate**: 3 days

---

## Week 10-11: Advanced Features

### Task 10.1: Attribute Promotion/Demotion

**File**: `nodeflux_core/include/nodeflux/core/attribute_promotion.hpp`

**Functions**:
```cpp
// Point → Vertex (replicate)
void promote_point_to_vertex(GeometryAttributes& geo, const std::string& attr_name);

// Vertex → Point (average)
void demote_vertex_to_point(GeometryAttributes& geo, const std::string& attr_name);

// Point → Primitive (average)
void promote_point_to_primitive(GeometryAttributes& geo, const std::string& attr_name);

// Primitive → Point (splat)
void demote_primitive_to_point(GeometryAttributes& geo, const std::string& attr_name);
```

**Time estimate**: 3 days

---

### Task 10.2: Attribute Groups

**File**: `nodeflux_core/include/nodeflux/core/attribute_group.hpp`

**Implementation**:
```cpp
class AttributeGroup {
    std::string name_;
    core::ElementClass element_class_;
    std::vector<bool> membership_;  // Bitset for fast membership test

public:
    void add(int element_id);
    void remove(int element_id);
    bool contains(int element_id) const;

    // Iteration over members only
    std::vector<int> members() const;
};
```

**Time estimate**: 3 days

---

### Task 10.3: GPU Staging (Basic)

**File**: `nodeflux_core/include/nodeflux/core/gpu_attribute_buffer.hpp`

**Basic pack/unpack**:
```cpp
// Pack attributes into contiguous buffer for GPU
std::vector<float> pack_for_gpu(
    const GeometryAttributes& geo,
    const std::vector<std::string>& attr_names
);

// Unpack from GPU buffer back to attributes
void unpack_from_gpu(
    GeometryAttributes& geo,
    const std::vector<float>& buffer,
    const std::vector<std::string>& attr_names
);
```

**Time estimate**: 3 days

---

## Week 12: Final Polish & Documentation

### Task 12.1: Production Documentation

**Create**:
- User guide
- API reference
- Migration guide from old system
- Best practices
- Performance tuning guide

**Time estimate**: 3 days

---

### Task 12.2: Final Validation

**Checklist**:
- [ ] All 18 SOPs migrated and tested
- [ ] Performance benchmarks meet targets
- [ ] Memory usage reasonable
- [ ] Documentation complete
- [ ] Examples work
- [ ] No compiler warnings
- [ ] Code review complete

**Time estimate**: 2 days

---

# Success Criteria

## Phase 1 (Foundation)
- ✅ 4 new core classes implemented
- ✅ >50 unit tests passing
- ✅ Documentation written
- ✅ No changes to existing SOPs

## Phase 2 (Integration)
- ✅ All 18 SOPs migrated
- ✅ All tests pass
- ✅ Outputs match old system exactly
- ✅ No performance regression

## Phase 3 (Polish)
- ✅ Old system removed
- ✅ Performance optimized
- ✅ Advanced features working
- ✅ Production-ready

## Overall Success
- ✅ Unified attribute system
- ✅ Standard names ("P", "N", "Cd")
- ✅ Fast SoA storage
- ✅ Type-safe API
- ✅ Comprehensive docs
- ✅ Production quality

---

# Risk Mitigation

## High Risks

### Risk: Performance Regression
**Mitigation**:
- Benchmark every week
- Profile hot paths
- Optimize before removing old system

### Risk: Breaking Changes
**Mitigation**:
- Keep old system working in Phase 2
- Migrate one SOP at a time
- Comprehensive tests
- Feature flag to rollback

### Risk: Scope Creep
**Mitigation**:
- Stick to plan
- Phase 4 (GPU, wrangle) is post-launch
- Focus on core correctness first

## Medium Risks

### Risk: API Complexity
**Mitigation**:
- Comprehensive examples
- Standard attribute helpers
- Clear documentation

### Risk: Test Coverage Gaps
**Mitigation**:
- >50 unit tests
- Integration tests
- Regression tests
- Performance tests

---

# Timeline Summary

| Phase | Duration | Key Deliverable |
|-------|----------|-----------------|
| **Phase 1: Foundation** | 4 weeks | Core infrastructure |
| **Phase 2: Integration** | 4 weeks | All SOPs migrated |
| **Phase 3: Polish** | 4 weeks | Production-ready |
| **Total** | **12 weeks** | Unified attribute system |

---

# Next Steps

**To start immediately**:

1. ✅ Read this guide thoroughly
2. ⬜ Set up project tracking (e.g., GitHub Projects)
3. ⬜ Create feature branch: `feature/attribute-system`
4. ⬜ Start Week 1, Task 1.1: Element Topology
5. ⬜ Write first test, see it pass
6. ⬜ Commit early, commit often
7. ⬜ Update this guide as you learn

**Daily workflow**:
- Morning: Review task, understand requirements
- Work: Implement + test
- Evening: Commit, update progress
- Weekly: Review, adjust plan if needed

---

**Ready to start? Begin with Week 1, Task 1.1: Element Topology Model!**

Good luck! 🚀
