# Attributes

Attributes are data attached to your geometry. They're the secret to data-driven procedural modeling - enabling selections, coloring, and advanced workflows.

## What are Attributes?

**Attributes** store information on geometry components:

```
Vertex (Point)  → Point Attributes   (position, color, normal, ...)
Face (Triangle) → Face Attributes    (material ID, ...)
Mesh (Object)   → Primitive Attribs  (name, layer, ...)
```

Think of them as **spreadsheet data** attached to your 3D geometry.

---

## Built-in Attributes

Nodo automatically creates some attributes:

### Point Attributes

| Name | Type | Description |
|------|------|-------------|
| `P` | vector3 | Position (x, y, z) |
| `N` | vector3 | Normal vector |
| `Cd` | vector3 | Color (RGB, 0-1 range) |
| `uv` | vector2 | Texture coordinates |
| `id` | int | Vertex ID/index |

**Access in expressions:**
```
@P       # Position vector
@P.x     # X coordinate
@P.y     # Y coordinate
@P.z     # Z coordinate
@N       # Normal
@Cd      # Color
```

### Face Attributes

| Name | Type | Description |
|------|------|-------------|
| `materialid` | int | Material assignment |
| `N` | vector3 | Face normal |

### Primitive Attributes

| Name | Type | Description |
|------|------|-------------|
| `name` | string | Object name |

---

## Creating Attributes

Use the **Attribute Create** node to add custom attributes.

### Example: Height Attribute

**Goal:** Store Y-position for each vertex

**Setup:**
```
Sphere
  ↓
Attribute Create
  - Name: "height"
  - Type: Float
  - Value: @P.y
```

**Result:** Each vertex has a `height` attribute = its Y coordinate

**Use:**
- Select high vertices: `@height > 0.5`
- Color by height: `@Cd = [@height, 0, 0]`

---

## Attribute Types

### Float

Single decimal number:
```
@height = 1.5
@temperature = 37.2
@weight = 0.8
```

### Int

Whole number:
```
@id = 42
@material = 3
@group_id = 1
```

### Vector2

Two floats (UV coordinates):
```
@uv = [0.5, 0.75]
```

### Vector3

Three floats (positions, colors, normals):
```
@P = [1.0, 2.0, 3.0]
@Cd = [1.0, 0.0, 0.0]  # Red
@N = [0.0, 1.0, 0.0]   # Up
```

### String

Text:
```
@name = "pillar_base"
@material_name = "brick"
```

---

## Using Attributes

### 1. Selection (Groups)

Select geometry based on attribute values:

```
Group Node:
- Mode: Expression
- Expression: @P.y > 0.5

Result: Select all vertices above Y=0.5
```

### 2. Color

Set vertex colors procedurally:

```
Attribute Create:
- Name: Cd
- Type: Vector3
- Value: [@P.x, @P.y, 0]

Result: Color based on position (red = X, green = Y)
```

### 3. Conditional Operations

Use attributes to drive operations:

```
Delete faces where @materialid == 2
Extrude faces where @selected == 1
```

### 4. Data-Driven Deformations

**Example:** Twist based on height

```
Attribute Create:
- Name: twist_amount
- Value: @P.y * 45

Then in Twist node:
- Angle: ch("@twist_amount")
```

---

## Common Workflows

### Color Gradient by Height

```
1. Sphere
2. Attribute Create:
   Name: Cd
   Type: Vector3
   Value: [0, @P.y + 0.5, 0]

Result: Green gradient (dark bottom, bright top)
```

### Random Colors

```
1. Box
2. Attribute Create:
   Name: Cd
   Type: Vector3
   Value: [rand(@id), rand(@id+1), rand(@id+2)]

Result: Each vertex has random color
```

### Material ID by Position

```
1. Cylinder
2. Attribute Create:
   Name: materialid
   Type: Int
   Value: @P.y > 0 ? 1 : 2

Result: Top half = material 1, bottom = material 2
```

### Distance from Center

```
1. Grid
2. Attribute Create:
   Name: dist
   Type: Float
   Value: length(@P)

Result: Each point stores distance from origin
```

---

## Attribute Interpolation

When geometry changes, attributes can be interpolated:

### Subdivide

```
Vertex A: @height = 0.0
Vertex B: @height = 1.0

After subdivide:
New vertex (midpoint): @height = 0.5 (interpolated)
```

Attributes blend smoothly across new geometry.

### Merge

```
Mesh A vertices: @id = 0, 1, 2, ...
Mesh B vertices: @id = 0, 1, 2, ...

After merge:
Combined: @id renumbered to avoid conflicts
```

---

## Attribute Promotion

Convert between attribute classes:

### Point → Face

Average point values for each face:

```
3 vertices with @height = [0.5, 0.6, 0.7]
Face @height = (0.5 + 0.6 + 0.7) / 3 = 0.6
```

### Face → Point

Distribute face values to points:

```
Face @material = 2
All 3 vertices get @material = 2
```

**Use Case:** Material IDs, selections

---

## Deleting Attributes

Use **Attribute Delete** node to remove unneeded attributes:

```
Attribute Delete:
- Name: "temp_*"  (wildcard pattern)
- or specific: "old_color", "scratch_data"
```

**Why delete?**
- Reduce memory usage
- Clean up temporary data
- Prepare for export

---

## Expressions with Attributes

### Reading Attributes

```
@P.y               # Current point's Y position
@Cd.r              # Red component of color
@N                 # Normal vector
@id                # Vertex ID
```

### Writing Attributes

```
@Cd = [1, 0, 0]           # Set red
@height = @P.y * 2        # Double Y position
@active = @P.y > 0 ? 1 : 0  # Boolean flag
```

### Math on Attributes

```
@dist = length(@P)               # Distance from origin
@normalized_height = @P.y / 10   # Normalize
@sum = @val1 + @val2             # Combine values
```

### Conditionals

```
@result = @value > 0.5 ? 1 : 0
@color = @hot ? [1,0,0] : [0,0,1]  # Red if hot, blue if cold
```

---

## Advanced Patterns

### Checkerboard Pattern

```
Attribute Create:
- Name: checker
- Value: (floor(@P.x) + floor(@P.z)) % 2
- Then use for color:
  @Cd = @checker == 0 ? [1,1,1] : [0,0,0]
```

### Radial Gradient

```
Attribute Create:
- Name: radial
- Value: length([@ P.x, @P.z])
- Then:
  @Cd = [1 - @radial, @radial, 0]
```

### Selection Mask

```
Attribute Create:
- Name: mask
- Value: sin(@P.x * 3.14) > 0 ? 1 : 0

Group (Expression): @mask == 1
Group Delete: Removes masked areas
```

### Instance ID

```
After Copy to Points:
Each instance gets @instanceid

Use for variation:
@Cd = [@instanceid * 0.1, 0, 0]
```

---

## Attribute Scope

### Point Attributes

- Stored **per vertex**
- Most common type
- Used for: positions, colors, normals, UVs

**Example:**
```
1000 vertex mesh
@Cd attribute = 1000 color values (one per vertex)
```

### Face Attributes

- Stored **per triangle**
- Less common
- Used for: material IDs, face selections

**Example:**
```
500 triangle mesh
@materialid = 500 integer values (one per face)
```

### Vertex Attributes

- Same as point attributes in Nodo
- Terminology varies by software

---

## Best Practices

### 1. Name Attributes Clearly

```
✅ @selection_mask, @height_normalized, @color_gradient
❌ @x, @temp, @thing
```

### 2. Clean Up Temporary Attributes

```
After using @temp_dist for calculation:
→ Attribute Delete: temp_*
```

### 3. Use Standard Names

Follow conventions:
```
@Cd   for color (not @color, @rgb)
@N    for normal (not @norm)
@uv   for texture coords
```

### 4. Check Attribute Type

```
Float: Single values (height, weight, temperature)
Vector3: Positions, colors, normals
Int: IDs, indices, material numbers
```

### 5. Scope Awareness

```
Point attributes: Per-vertex data (most common)
Face attributes: Per-triangle data (materials, selections)
```

---

## Troubleshooting

### Attribute Not Found

**Error:** `@myattr` doesn't exist

**Fix:**
- Check spelling (case-sensitive!)
- Verify Attribute Create node executed
- Check attribute was created on points (not faces)

### Wrong Data Type

**Error:** Assigning vector to float attribute

**Fix:**
- Match types: Float = scalar, Vector3 = [x,y,z]
- Convert if needed: `length(@P)` (vector → float)

### Attribute Not Updating

**Cause:** Node not re-executing

**Fix:**
- Change a parameter to force update
- Check node connections
- Verify expression mode enabled

### Colors Not Showing

**Cause:** @Cd attribute missing or wrong range

**Fix:**
- Ensure @Cd is Vector3
- Values must be 0.0 to 1.0 range
- Enable vertex colors in viewport

---

## Performance Tips

### Minimize Attributes

Only create what you need:
```
❌ 20 temporary attributes
✅ 3 essential attributes + delete temps
```

### Batch Operations

```
✅ One Attribute Create with multiple attributes
❌ 10 separate Attribute Create nodes
```

### Optimize Expressions

```
✅ @dist = length(@P)  (calculated once)
❌ Multiple uses of length(@P)  (calculated each time)
```

---

## Examples

### Rainbow Colors

```
Grid (10×10)
  ↓
Attribute Create:
  Name: Cd
  Type: Vector3
  Value: [(sin(@id * 0.5) + 1) * 0.5,
          (cos(@id * 0.3) + 1) * 0.5,
          (sin(@id * 0.7) + 1) * 0.5]
```

### Height-Based Selection

```
Cylinder
  ↓
Group:
  Mode: Expression
  Expression: @P.y > 0.5
  ↓
Group Delete (delete selected = top half removed)
```

### Distance Falloff

```
Sphere
  ↓
Attribute Create:
  Name: falloff
  Value: 1.0 - min(length(@P) / 2.0, 1.0)
  ↓
Attribute Create:
  Name: Cd
  Value: [@falloff, @falloff, @falloff]

Result: White center, black edges
```

---

## Next Steps

- **Understand Groups** - [Groups](groups.md)
- **Use in Workflows** - [Procedural Patterns](../workflows/patterns.md)
- **Expression System** - [Expression Syntax](../expressions/expression-syntax.md)
- **Node Reference** - [Attribute Create](../nodes/attribute/attribute_create.md)

---

## Key Takeaways

✅ **Attributes** = Data attached to geometry components
✅ **Types** = Float, Int, Vector2/3, String
✅ **Scope** = Point (vertex), Face (triangle), Primitive (object)
✅ **Built-ins** = @P, @N, @Cd, @uv, @id
✅ **Uses** = Selection, coloring, data-driven operations
✅ **Access** = `@attribute_name` in expressions

**Remember:** Attributes turn geometry into data - enabling procedural magic!
