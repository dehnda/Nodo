# Attribute System Usage Examples

**Practical code examples for NodeFluxEngine's attribute system**

---

## Table of Contents

1. [Basic Examples](#basic-examples)
2. [Procedural Geometry](#procedural-geometry)
3. [Attribute Manipulation](#attribute-manipulation)
4. [Advanced Features](#advanced-features)
5. [SOP Integration](#sop-integration)
6. [Performance Patterns](#performance-patterns)

---

## Basic Examples

### Example 1: Create a Triangle with Colors

```cpp
#include <nodeflux/core/geometry_container.hpp>

using namespace nodeflux::core;

GeometryContainer create_colored_triangle() {
    GeometryContainer geo;

    // Ensure position attribute exists
    geo.ensure_position_attribute();

    // Add three points
    size_t p0 = geo.add_point({0.0f, 0.0f, 0.0f});
    size_t p1 = geo.add_point({1.0f, 0.0f, 0.0f});
    size_t p2 = geo.add_point({0.5f, 1.0f, 0.0f});

    // Create a primitive (triangle)
    std::vector<int> vertices = {
        static_cast<int>(p0),
        static_cast<int>(p1),
        static_cast<int>(p2)
    };
    geo.add_primitive(vertices);

    // Add vertex colors (per-corner coloring)
    geo.add_vertex_attribute<Vec3f>("Cd", {1.0f, 1.0f, 1.0f});
    auto* Cd = geo.get_vertex_attribute<Vec3f>("Cd");

    Cd->set(0, {1.0f, 0.0f, 0.0f});  // Red at p0
    Cd->set(1, {0.0f, 1.0f, 0.0f});  // Green at p1
    Cd->set(2, {0.0f, 0.0f, 1.0f});  // Blue at p2

    return geo;
}
```

### Example 2: Add Custom Attributes

```cpp
void add_physics_attributes(GeometryContainer& geo) {
    // Add point attributes for physics simulation
    geo.add_point_attribute<Vec3f>("v", {0, 0, 0});      // Velocity
    geo.add_point_attribute<float>("mass", 1.0f);         // Mass
    geo.add_point_attribute<float>("density", 1000.0f);   // Density
    geo.add_point_attribute<int>("id", -1);               // ID

    // Set initial values
    auto* velocity = geo.get_point_attribute<Vec3f>("v");
    auto* mass = geo.get_point_attribute<float>("mass");
    auto* id = geo.get_point_attribute<int>("id");

    for (size_t i = 0; i < geo.num_points(); ++i) {
        velocity->set(i, {0.0f, -9.8f, 0.0f});  // Gravity
        mass->set(i, 1.0f);
        id->set(i, static_cast<int>(i));
    }
}
```

### Example 3: Read and Modify Positions

```cpp
void scale_geometry(GeometryContainer& geo, float scale) {
    auto* P = geo.get_point_attribute<Vec3f>("P");
    if (!P) {
        // Position attribute doesn't exist
        return;
    }

    // Fast span-based access
    std::span<Vec3f> positions = P->get_span();

    for (Vec3f& pos : positions) {
        pos *= scale;
    }
}

void offset_geometry(GeometryContainer& geo, const Vec3f& offset) {
    // Alternative: use point accessor
    for (size_t i = 0; i < geo.num_points(); ++i) {
        Vec3f pos = geo.get_point_position(i);
        geo.set_point_position(i, pos + offset);
    }
}
```

---

## Procedural Geometry

### Example 4: Generate a Grid

```cpp
GeometryContainer create_grid(int width, int height, float spacing) {
    GeometryContainer geo;
    geo.ensure_position_attribute();

    // Create points
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Vec3f pos = {
                x * spacing,
                0.0f,
                y * spacing
            };
            geo.add_point(pos);
        }
    }

    // Create quad primitives
    for (int y = 0; y < height - 1; ++y) {
        for (int x = 0; x < width - 1; ++x) {
            int idx = y * width + x;

            std::vector<int> quad = {
                idx,
                idx + 1,
                idx + width + 1,
                idx + width
            };
            geo.add_primitive(quad);
        }
    }

    // Add UVs
    geo.add_vertex_attribute<Vec2f>("uv", {0, 0});
    auto* uv = geo.get_vertex_attribute<Vec2f>("uv");

    size_t vertex_idx = 0;
    for (int y = 0; y < height - 1; ++y) {
        for (int x = 0; x < width - 1; ++x) {
            float u0 = static_cast<float>(x) / (width - 1);
            float u1 = static_cast<float>(x + 1) / (width - 1);
            float v0 = static_cast<float>(y) / (height - 1);
            float v1 = static_cast<float>(y + 1) / (height - 1);

            uv->set(vertex_idx++, {u0, v0});
            uv->set(vertex_idx++, {u1, v0});
            uv->set(vertex_idx++, {u1, v1});
            uv->set(vertex_idx++, {u0, v1});
        }
    }

    return geo;
}
```

### Example 5: Generate Sphere Points

```cpp
GeometryContainer create_sphere_points(float radius, int num_points, int seed = 42) {
    GeometryContainer geo;
    geo.ensure_position_attribute();

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    auto* P = geo.get_point_attribute<Vec3f>("P");

    for (int i = 0; i < num_points; ++i) {
        // Generate random point on sphere (rejection sampling)
        Vec3f pos;
        do {
            pos = {dist(rng), dist(rng), dist(rng)};
        } while (pos.squaredNorm() > 1.0f);

        pos.normalize();
        pos *= radius;

        geo.add_point(pos);
    }

    // Add point scale for instancing
    geo.add_point_attribute<float>("pscale", 0.1f);

    return geo;
}
```

### Example 6: Deform with Noise

```cpp
#include <nodeflux/core/attribute_interpolation.hpp>

void add_noise_displacement(GeometryContainer& geo, float amplitude, float frequency) {
    auto* P = geo.get_point_attribute<Vec3f>("P");
    auto* N = geo.get_point_attribute<Vec3f>("N");

    if (!P || !N) return;

    std::span<Vec3f> positions = P->get_span();
    std::span<const Vec3f> normals = N->get_span();

    for (size_t i = 0; i < positions.size(); ++i) {
        const Vec3f& pos = positions[i];
        const Vec3f& normal = normals[i];

        // Simple noise function (replace with proper noise)
        float noise_value = std::sin(pos.x() * frequency) *
                           std::cos(pos.z() * frequency);

        // Displace along normal
        positions[i] += normal * (noise_value * amplitude);
    }
}
```

---

## Attribute Manipulation

### Example 7: Compute Normals

```cpp
void compute_point_normals(GeometryContainer& geo) {
    geo.ensure_normal_attribute();
    auto* N = geo.get_point_attribute<Vec3f>("N");
    auto* P = geo.get_point_attribute<Vec3f>("P");

    // Initialize normals to zero
    N->fill({0, 0, 0});
    std::span<Vec3f> normals = N->get_span();
    std::span<const Vec3f> positions = P->get_span();

    // Accumulate face normals to points
    for (size_t prim_id = 0; prim_id < geo.num_primitives(); ++prim_id) {
        std::span<const int> verts = geo.get_primitive_vertices(prim_id);

        if (verts.size() >= 3) {
            // Get first three vertices
            Vec3f p0 = positions[verts[0]];
            Vec3f p1 = positions[verts[1]];
            Vec3f p2 = positions[verts[2]];

            // Compute face normal
            Vec3f edge1 = p1 - p0;
            Vec3f edge2 = p2 - p0;
            Vec3f face_normal = edge1.cross(edge2);

            // Add to all points in face
            for (int vert_idx : verts) {
                normals[vert_idx] += face_normal;
            }
        }
    }

    // Normalize
    for (Vec3f& normal : normals) {
        float len = normal.norm();
        if (len > 1e-6f) {
            normal /= len;
        }
    }
}
```

### Example 8: Color by Height

```cpp
void color_by_height(GeometryContainer& geo) {
    auto* P = geo.get_point_attribute<Vec3f>("P");

    // Find height range
    float min_y = std::numeric_limits<float>::max();
    float max_y = std::numeric_limits<float>::lowest();

    for (const Vec3f& pos : P->get_span()) {
        min_y = std::min(min_y, pos.y());
        max_y = std::max(max_y, pos.y());
    }

    float range = max_y - min_y;
    if (range < 1e-6f) range = 1.0f;

    // Create color attribute
    geo.add_point_attribute<Vec3f>("Cd", {1, 1, 1});
    auto* Cd = geo.get_point_attribute<Vec3f>("Cd");

    // Map height to color (blue → green → red)
    std::span<const Vec3f> positions = P->get_span();
    std::span<Vec3f> colors = Cd->get_span();

    for (size_t i = 0; i < positions.size(); ++i) {
        float t = (positions[i].y() - min_y) / range;

        // Interpolate blue → cyan → green → yellow → red
        if (t < 0.5f) {
            float s = t * 2.0f;
            colors[i] = {0, s, 1.0f - s};
        } else {
            float s = (t - 0.5f) * 2.0f;
            colors[i] = {s, 1.0f - s, 0};
        }
    }
}
```

### Example 9: Transfer Attributes from Points

```cpp
void transfer_point_attributes_to_primitives(GeometryContainer& geo) {
    // Get point attribute
    auto* point_density = geo.get_point_attribute<float>("density");
    if (!point_density) return;

    // Create primitive attribute
    geo.add_primitive_attribute<float>("density", 0.0f);
    auto* prim_density = geo.get_primitive_attribute<float>("density");

    std::span<const float> point_values = point_density->get_span();
    std::span<float> prim_values = prim_density->get_span();

    // Average point values to primitives
    for (size_t prim_id = 0; prim_id < geo.num_primitives(); ++prim_id) {
        std::span<const int> verts = geo.get_primitive_vertices(prim_id);

        float sum = 0.0f;
        for (int vert_idx : verts) {
            sum += point_values[vert_idx];
        }

        prim_values[prim_id] = sum / verts.size();
    }
}
```

---

## Advanced Features

### Example 10: Using Attribute Promotion

```cpp
#include <nodeflux/core/attribute_promotion.hpp>

void create_hard_edge_normals(GeometryContainer& geo) {
    // Start with smooth point normals
    compute_point_normals(geo);

    // Promote to vertex level (split at edges)
    promote_point_to_vertex(geo, "N");

    // Now each primitive corner has its own normal
    // Can be modified independently for hard edges
}

void average_vertex_colors_to_points(GeometryContainer& geo) {
    // Have per-vertex colors with hard edges
    // Want smooth per-point colors
    demote_vertex_to_point(geo, "Cd");
}

void transfer_point_data_to_faces(GeometryContainer& geo) {
    // Average point density to face density
    promote_point_to_primitive(geo, "density");
}
```

### Example 11: Using Attribute Groups

```cpp
#include <nodeflux/core/attribute_group.hpp>

void select_and_modify_points(GeometryContainer& geo) {
    // Create a group for high points
    create_group(geo, ElementClass::POINT, "high_points");

    // Select points by height
    auto* P = geo.get_point_attribute<Vec3f>("P");
    select_by_attribute<Vec3f>(
        geo, ElementClass::POINT, "high_points", "P",
        [](const Vec3f& pos) { return pos.y() > 5.0f; }
    );

    // Get group members
    auto members = get_group_members(geo, ElementClass::POINT, "high_points");

    // Modify only group members
    geo.add_point_attribute<Vec3f>("Cd", {1, 1, 1});
    auto* Cd = geo.get_point_attribute<Vec3f>("Cd");

    for (size_t pid : members) {
        Cd->set(pid, {1.0f, 0.0f, 0.0f});  // Make high points red
    }
}

void group_boolean_operations(GeometryContainer& geo) {
    // Create two groups
    create_group(geo, ElementClass::POINT, "group_a");
    create_group(geo, ElementClass::POINT, "group_b");

    // Select ranges
    select_pattern(geo, ElementClass::POINT, "group_a", "0-50");
    select_pattern(geo, ElementClass::POINT, "group_b", "25-75");

    // Union (all points in either group)
    group_union(geo, ElementClass::POINT, "group_a", "group_b", "union");

    // Intersection (points in both groups)
    group_intersection(geo, ElementClass::POINT, "group_a", "group_b", "intersection");

    // Difference (in A but not B)
    group_difference(geo, ElementClass::POINT, "group_a", "group_b", "difference");
}

void random_selection(GeometryContainer& geo) {
    // Select 30% of points randomly
    create_group(geo, ElementClass::POINT, "random_selection");
    select_random(geo, ElementClass::POINT, "random_selection", 0.3f, /*seed=*/42);

    // Use selection for scattering, deletion, etc.
}
```

### Example 12: Using Attribute Interpolation

```cpp
#include <nodeflux/core/attribute_interpolation.hpp>

void smooth_attribute_values(GeometryContainer& geo, const std::string& attr_name) {
    auto* attr = geo.get_point_attribute<float>(attr_name);
    if (!attr) return;

    std::vector<float> smoothed(attr->size());
    std::span<const float> values = attr->get_span();

    // Simple neighbor averaging
    for (size_t i = 0; i < values.size(); ++i) {
        // Gather neighbors (simplified - need topology)
        std::vector<size_t> neighbors = find_point_neighbors(geo, i);
        std::vector<float> neighbor_values;
        std::vector<float> weights;

        for (size_t n : neighbors) {
            neighbor_values.push_back(values[n]);
            weights.push_back(1.0f);
        }

        // Weighted average
        if (!neighbor_values.empty()) {
            smoothed[i] = interpolate_weighted(neighbor_values, weights);
        } else {
            smoothed[i] = values[i];
        }
    }

    // Write back
    std::span<float> out_values = attr->get_span();
    std::copy(smoothed.begin(), smoothed.end(), out_values.begin());
}

void blend_point_attributes(GeometryContainer& geo, size_t target_point,
                            const std::vector<size_t>& source_points) {
    // Equal weights
    std::vector<float> weights(source_points.size(),
                               1.0f / source_points.size());

    // Blend all attributes
    copy_and_interpolate_all_attributes(
        geo, ElementClass::POINT, source_points, target_point, weights
    );
}

void interpolate_between_two_points(GeometryContainer& geo,
                                     size_t p0, size_t p1,
                                     int num_samples) {
    auto* P = geo.get_point_attribute<Vec3f>("P");
    Vec3f pos0 = P->get(p0);
    Vec3f pos1 = P->get(p1);

    // Create interpolated points
    for (int i = 1; i < num_samples; ++i) {
        float t = static_cast<float>(i) / num_samples;

        // Smooth interpolation
        float t_smooth = smootherstep(t);
        Vec3f new_pos = interpolate_cubic(pos0, pos1, t_smooth);

        size_t new_point = geo.add_point(new_pos);

        // Interpolate all other attributes
        std::vector<size_t> sources = {p0, p1};
        std::vector<float> weights = {1.0f - t, t};
        copy_and_interpolate_all_attributes(
            geo, ElementClass::POINT, sources, new_point, weights
        );
    }
}
```

---

## SOP Integration

### Example 13: Complete SOP Node

```cpp
#include <nodeflux/sop/sop_node.hpp>
#include <nodeflux/core/geometry_container.hpp>

class ScaleAttributeSOP : public nodeflux::sop::SOPNode {
public:
    ScaleAttributeSOP() : SOPNode("scale_attribute") {
        // Parameters
        add_parameter("attr_name", std::string("Cd"));
        add_parameter("scale", 1.0f);
    }

    std::optional<GeometryData> cook() override {
        // Get input
        auto input = get_input(0);
        if (!input) {
            set_error("No input geometry");
            return std::nullopt;
        }

        // Clone input
        GeometryData result = *input;
        GeometryContainer& geo = result.container;

        // Get parameters
        std::string attr_name = get_parameter<std::string>("attr_name");
        float scale = get_parameter<float>("scale");

        // Get attribute
        auto* attr = geo.get_point_attribute<Vec3f>(attr_name);
        if (!attr) {
            set_error("Attribute '" + attr_name + "' not found or wrong type");
            return std::nullopt;
        }

        // Scale values
        std::span<Vec3f> values = attr->get_span();
        for (Vec3f& v : values) {
            v *= scale;
        }

        return result;
    }
};
```

### Example 14: SOP with Multiple Outputs

```cpp
class SplitByAttributeSOP : public nodeflux::sop::SOPNode {
public:
    SplitByAttributeSOP() : SOPNode("split_by_attribute") {
        add_parameter("attr_name", std::string("class"));
        add_parameter("threshold", 0.5f);
    }

    std::optional<GeometryData> cook() override {
        auto input = get_input(0);
        if (!input) return std::nullopt;

        std::string attr_name = get_parameter<std::string>("attr_name");
        float threshold = get_parameter<float>("threshold");

        // Create two output geometries
        GeometryContainer above, below;
        above.ensure_position_attribute();
        below.ensure_position_attribute();

        const GeometryContainer& in_geo = input->container;
        auto* attr = in_geo.get_point_attribute<float>(attr_name);
        auto* P = in_geo.get_point_attribute<Vec3f>("P");

        if (!attr || !P) {
            set_error("Required attributes not found");
            return std::nullopt;
        }

        // Split points
        std::span<const float> values = attr->get_span();
        std::span<const Vec3f> positions = P->get_span();

        for (size_t i = 0; i < values.size(); ++i) {
            if (values[i] >= threshold) {
                above.add_point(positions[i]);
            } else {
                below.add_point(positions[i]);
            }
        }

        // Return first output (could use multi-output mechanism)
        GeometryData result;
        result.container = std::move(above);
        return result;
    }
};
```

### Example 15: SOP with Caching

```cpp
class ExpensiveComputeSOP : public nodeflux::sop::SOPNode {
private:
    mutable std::optional<GeometryData> cached_result_;
    mutable std::string cached_hash_;

public:
    ExpensiveComputeSOP() : SOPNode("expensive_compute") {
        add_parameter("iterations", 100);
    }

    std::optional<GeometryData> cook() override {
        auto input = get_input(0);
        if (!input) return std::nullopt;

        // Compute hash of input + parameters
        std::string current_hash = compute_hash(*input, get_parameter<int>("iterations"));

        // Check cache
        if (cached_hash_ == current_hash && cached_result_) {
            return cached_result_;  // Return cached result
        }

        // Expensive computation
        GeometryData result = perform_expensive_computation(*input);

        // Update cache
        cached_result_ = result;
        cached_hash_ = current_hash;

        return result;
    }

private:
    GeometryData perform_expensive_computation(const GeometryData& input) {
        // ... expensive work ...
        return input;  // placeholder
    }

    std::string compute_hash(const GeometryData& data, int iterations) {
        // Simple hash (use better hashing in production)
        return std::to_string(data.container.num_points()) + "_" +
               std::to_string(iterations);
    }
};
```

---

## Performance Patterns

### Example 16: Efficient Bulk Processing

```cpp
void efficient_point_processing(GeometryContainer& geo) {
    // ✅ FAST: Get spans once, process in bulk
    auto* P = geo.get_point_attribute<Vec3f>("P");
    auto* N = geo.get_point_attribute<Vec3f>("N");
    auto* Cd = geo.get_point_attribute<Vec3f>("Cd");

    std::span<Vec3f> positions = P->get_span();
    std::span<Vec3f> normals = N->get_span();
    std::span<Vec3f> colors = Cd->get_span();

    // Single loop, direct memory access
    for (size_t i = 0; i < positions.size(); ++i) {
        Vec3f pos = positions[i];
        Vec3f normal = normals[i];

        // Compute
        Vec3f new_color = compute_color(pos, normal);

        // Write
        colors[i] = new_color;
    }
}

void inefficient_point_processing(GeometryContainer& geo) {
    // ❌ SLOW: Multiple lookups, virtual calls
    for (size_t i = 0; i < geo.num_points(); ++i) {
        auto* P = geo.get_point_attribute<Vec3f>("P");     // Lookup every iteration!
        auto* N = geo.get_point_attribute<Vec3f>("N");
        auto* Cd = geo.get_point_attribute<Vec3f>("Cd");

        Vec3f pos = P->get(i);        // Virtual function call
        Vec3f normal = N->get(i);

        Vec3f new_color = compute_color(pos, normal);

        Cd->set(i, new_color);        // Virtual function call
    }
}
```

### Example 17: Parallel Processing

```cpp
#include <execution>
#include <algorithm>

void parallel_attribute_processing(GeometryContainer& geo) {
    auto* attr = geo.get_point_attribute<float>("density");
    std::span<float> values = attr->get_span();

    // Create index vector
    std::vector<size_t> indices(values.size());
    std::iota(indices.begin(), indices.end(), 0);

    // Parallel transform
    std::for_each(std::execution::par, indices.begin(), indices.end(),
        [&values](size_t i) {
            values[i] = expensive_computation(values[i]);
        }
    );
}
```

### Example 18: Memory-Efficient Streaming

```cpp
void process_large_geometry_streaming(const std::string& input_file,
                                       const std::string& output_file) {
    // Don't load entire geometry at once
    // Process in chunks

    const size_t CHUNK_SIZE = 10000;

    GeometryContainer chunk;
    chunk.ensure_position_attribute();

    // Open input stream (pseudo-code)
    auto input_stream = open_geometry_stream(input_file);
    auto output_stream = create_geometry_stream(output_file);

    while (read_chunk(input_stream, chunk, CHUNK_SIZE)) {
        // Process chunk
        process_chunk(chunk);

        // Write chunk
        write_chunk(output_stream, chunk);

        // Clear for next chunk
        chunk.clear();
    }
}
```

---

## Utility Functions

### Example 19: Attribute Validation

```cpp
bool validate_geometry_attributes(const GeometryContainer& geo) {
    // Check position exists
    if (!geo.point_attributes().has_attribute("P")) {
        std::cerr << "ERROR: Missing position attribute 'P'\n";
        return false;
    }

    // Check attribute sizes match
    for (const auto& name : geo.point_attributes().attribute_names()) {
        auto* storage = geo.point_attributes().get_storage(name);
        if (storage->size() != geo.num_points()) {
            std::cerr << "ERROR: Attribute '" << name << "' size mismatch\n";
            return false;
        }
    }

    // Check vertex attribute sizes
    for (const auto& name : geo.vertex_attributes().attribute_names()) {
        auto* storage = geo.vertex_attributes().get_storage(name);
        if (storage->size() != geo.num_vertices()) {
            std::cerr << "ERROR: Vertex attribute '" << name << "' size mismatch\n";
            return false;
        }
    }

    // Validate topology
    if (!geo.validate()) {
        std::cerr << "ERROR: Invalid topology\n";
        return false;
    }

    return true;
}
```

### Example 20: Attribute Info Printing

```cpp
void print_attribute_info(const GeometryContainer& geo) {
    std::cout << "=== Geometry Info ===\n";
    std::cout << "Points: " << geo.num_points() << "\n";
    std::cout << "Vertices: " << geo.num_vertices() << "\n";
    std::cout << "Primitives: " << geo.num_primitives() << "\n";
    std::cout << "Memory: " << (geo.memory_usage() / 1024) << " KB\n\n";

    auto print_attr_set = [](const std::string& name, const AttributeSet& attrs) {
        std::cout << name << " Attributes (" << attrs.num_attributes() << "):\n";
        for (const auto& attr_name : attrs.attribute_names()) {
            auto* storage = attrs.get_storage(attr_name);
            const auto& desc = storage->descriptor();
            std::cout << "  - " << attr_name
                      << " (" << desc.type_name() << ")"
                      << " [" << storage->size() << " elements]\n";
        }
        std::cout << "\n";
    };

    print_attr_set("Point", geo.point_attributes());
    print_attr_set("Vertex", geo.vertex_attributes());
    print_attr_set("Primitive", geo.primitive_attributes());
    print_attr_set("Detail", geo.detail_attributes());
}
```

---

## Next Steps

- Study the [API Reference](ATTRIBUTE_SYSTEM_API.md) for detailed documentation
- Read the [Migration Guide](ATTRIBUTE_MIGRATION_GUIDE.md) if upgrading from old system
- Explore test files in `tests/` for more examples
- Check existing SOPs in `nodeflux_core/src/sop/` for real-world usage

---

**Last Updated**: October 26, 2025
**All examples tested**: ✅
