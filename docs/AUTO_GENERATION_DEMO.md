# Auto-Generated Documentation Demo

This shows what the `generate_node_docs.py` script produces.

## Example: How Sphere Node Doc is Generated

### Input: C++ Code

**From `sop_factory.cpp`:**
```cpp
{NodeType::Sphere, "Sphere", "Generator", "Create a UV sphere primitive"}
```

**From `sphere_sop.hpp`:**
```cpp
register_parameter(define_int_parameter("primitive_type", 0)
    .label("Primitive Type")
    .options({"Polygon", "Points"})
    .category("Universal")
    .build());

register_parameter(define_float_parameter("radius", 1.0f)
    .label("Radius")
    .range(0.01, 100.0)
    .category("Size")
    .description("Radius of the sphere")
    .build());

register_parameter(define_int_parameter("segments", 32)
    .label("Segments")
    .range(3, 256)
    .category("Resolution")
    .description("Number of vertical segments (longitude)")
    .build());

// Input configuration
InputConfig get_input_config() const override {
    return InputConfig(InputType::NONE, 0, 0, 0);
}
```

### Output: Generated Markdown

**File:** `docs/nodes/generator/sphere.md`

```markdown
# Sphere

**Category:** Generator

## Description

Create a UV sphere primitive

## Inputs

This node generates geometry and requires no inputs.

## Parameters

### Universal

**Primitive Type** (`int`)

- Default: `0` | Options: `Polygon`, `Points`

### Size

**Radius** (`float`)

Radius of the sphere

- Default: `1.0` | Range: `0.01` to `100.0`

### Resolution

**Segments** (`int`)

Number of vertical segments (longitude)

- Default: `32` | Range: `3` to `256`

**Rings** (`int`)

Number of horizontal rings (latitude)

- Default: `16` | Range: `3` to `128`

## Example Usage

Create a Sphere node from the Node Library panel. Adjust parameters to customize the generated geometry.

## See Also

- [Box](box.md)
- [Cylinder](cylinder.md)
- [Torus](torus.md)
```

---

## Example: Transform Node (Modifier)

### Input: C++ Code

**From `sop_factory.cpp`:**
```cpp
{NodeType::Transform, "Transform", "Modifier",
 "Transform geometry with translate, rotate, scale"}
```

**From `transform_sop.hpp`:**
```cpp
// Accepts single input
InputConfig get_input_config() const override {
    return InputConfig(InputType::SINGLE, 1, 1, 1);
}

register_parameter(define_vec3_parameter("translate", Vec3(0,0,0))
    .label("Translate")
    .category("Transform")
    .build());

register_parameter(define_vec3_parameter("rotate", Vec3(0,0,0))
    .label("Rotate")
    .category("Transform")
    .description("Rotation in degrees (XYZ)")
    .build());

register_parameter(define_vec3_parameter("scale", Vec3(1,1,1))
    .label("Scale")
    .category("Transform")
    .build());
```

### Output: Generated Markdown

**File:** `docs/nodes/modifier/transform.md`

```markdown
# Transform

**Category:** Modifier

## Description

Transform geometry with translate, rotate, scale

## Inputs

- **Input 0**: Geometry to process

## Parameters

### Transform

**Translate** (`vec3`)

- Default: `(0, 0, 0)`

**Rotate** (`vec3`)

Rotation in degrees (XYZ)

- Default: `(0, 0, 0)`

**Scale** (`vec3`)

- Default: `(1, 1, 1)`

## Example Usage

Connect geometry to the input, then adjust parameters to modify the result.

## See Also

- [Array](../array/array.md)
- [Mirror](mirror.md)
- [Align](align.md)
```

---

## Example: Boolean Node (Multi-Input)

### Input: C++ Code

**From `boolean_sop.hpp`:**
```cpp
// Accepts multiple inputs
InputConfig get_input_config() const override {
    return InputConfig(InputType::MULTIPLE, 2, -1, 2);
    // Min 2, max unlimited, 2 required
}

register_parameter(define_int_parameter("operation", 0)
    .label("Operation")
    .options({"Union", "Subtract", "Intersect"})
    .category("Boolean")
    .build());
```

### Output: Generated Markdown

```markdown
# Boolean

**Category:** Boolean

## Description

Perform boolean operations (union, subtract, intersect)

## Inputs

- **Inputs**: Accepts 2+ geometry inputs

## Parameters

### Boolean

**Operation** (`int`)

- Default: `0` | Options: `Union`, `Subtract`, `Intersect`

## Example Usage

Connect multiple geometry inputs to perform boolean operations.

## See Also

- [Merge](merge.md)
- [Split](../modifier/split.md)
```

---

## Generated Index Page

**File:** `docs/nodes/index.md`

```markdown
# Node Reference

Complete reference for all available nodes in Nodo.

## Generator

- **[Sphere](generator/sphere.md)** - Create a UV sphere primitive
- **[Box](generator/box.md)** - Create a box primitive
- **[Cylinder](generator/cylinder.md)** - Create a cylinder primitive
- **[Torus](generator/torus.md)** - Create a torus primitive
- **[Grid](generator/grid.md)** - Create a planar grid of polygons
- **[Line](generator/line.md)** - Create a line or curve

## Modifier

- **[Transform](modifier/transform.md)** - Transform geometry with translate, rotate, scale
- **[Extrude](modifier/extrude.md)** - Extrude geometry along normals
- **[Smooth](modifier/smooth_(laplacian).md)** - Smooth geometry using Laplacian method
- **[Subdivide](modifier/subdivide.md)** - Subdivide polygons for smoother geometry
... (all 44 nodes categorized)
```

---

## Workflow Summary

1. **Run Generator:**
   ```bash
   python tools/generate_node_docs.py
   ```

2. **Output:**
   - Creates `docs/nodes/{category}/{node}.md` for all 44 nodes
   - Creates `docs/nodes/index.md` with categorized list
   - Preserves manual enhancements if re-run

3. **Preview:**
   ```bash
   mkdocs serve
   # Open localhost:8000
   ```

4. **Deploy:**
   ```bash
   mkdocs gh-deploy
   # Live at github.io/Nodo
   ```

---

## Benefits

✅ **Automatic** - No manual typing for 44+ nodes
✅ **Accurate** - Always matches code
✅ **Consistent** - Same format for all nodes
✅ **Scalable** - New nodes auto-documented
✅ **Maintainable** - Update code, regenerate docs

Manual enhancements (examples, tips) added once and preserved on regeneration.
