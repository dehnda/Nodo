# Groups

Groups let you select and operate on **subsets** of geometry. They're essential for targeted operations like "delete only the top faces" or "extrude just the windows."

## What are Groups?

A **group** is a named selection of geometry components (points, edges, or faces).

```
Complete Mesh:  ●●●●●●●●●●  (10 vertices)
                ↓
Group "top":    ●●●○○○○○○○  (3 vertices selected)
```

Think of groups as **filters** that let operations affect only specific parts.

---

## Why Use Groups?

### Targeted Operations

**Without groups:**
```
Extrude → Affects entire mesh
```

**With groups:**
```
Group (top faces) → Extrude → Only top faces extrude
```

### Named Selections

Save selections for reuse:
```
Create group "windows" → Use in multiple operations
```

### Complex Workflows

Build sophisticated procedural models:
```
1. Select every other face → Group "pattern_a"
2. Select remaining faces → Group "pattern_b"
3. Color pattern_a red, pattern_b blue
```

---

## Group Types

### Point Groups

Select **vertices**:
```
Group all points where @P.y > 0.5
```

**Use for:**
- Vertex-based deformations
- Color assignments
- Position-based selections

### Edge Groups

Select **edges**:
```
Group edges forming boundary loops
```

**Use for:**
- Beveling specific edges
- Edge-based selections

### Primitive (Face) Groups

Select **triangles**:
```
Group faces where @materialid == 2
```

**Use for:**
- Material assignments
- Face deletion
- Per-face operations

---

## Creating Groups

Use the **Group** node to create selections.

### Method 1: Pattern-Based

Select by **index patterns**:

**Example: Every Other Vertex**
```
Group Node:
- Mode: Pattern
- Pattern: "0-100:2"  (0, 2, 4, 6, ... every 2nd)
- Group Name: "alternating"
```

**Pattern Syntax:**
```
0-10        → Indices 0 through 10
0,5,10      → Specific indices 0, 5, 10
0-100:2     → Every 2nd index (step of 2)
0-50,75-100 → Ranges 0-50 and 75-100
```

### Method 2: Expression-Based

Select by **attribute values**:

**Example: Upper Half**
```
Group Node:
- Mode: Expression
- Expression: @P.y > 0.0
- Group Name: "upper_half"
```

**Common Expressions:**
```
@P.y > 0.5              → Top vertices
@P.x < 0 and @P.z < 0   → Quadrant selection
@Cd.r > 0.8             → Red vertices
@id % 2 == 0            → Even IDs
length(@P) < 2.0        → Within radius
```

### Method 3: Manual

*Future feature:* Click to select in viewport

---

## Using Groups

### Deletion

**Group Delete** node removes selected geometry:

```
Sphere
  ↓
Group (Expression: @P.y < 0)  → Select bottom half
  ↓
Group Delete                   → Remove bottom
  ↓
Result: Hemisphere
```

### Material Assignment

Assign materials to grouped faces:

```
Box
  ↓
Group (Expression: @P.y > 0.99)  → Top face
  ↓
Attribute Create:
  Name: materialid
  Value: 2                       → Material ID 2 for top
```

### Conditional Extrusion

Extrude only selected faces:

```
Grid
  ↓
Group (Pattern: "0-100:3")  → Every 3rd face
  ↓
Extrude (if group exists, only affects grouped faces)
```

---

## Group Operations

### Combining Groups

**Group Combine** node merges or intersects groups:

**Union** (A or B):
```
Group A: "top_faces"
Group B: "side_faces"
  ↓
Group Combine: Union
  ↓
Result: "top_or_side" (all faces in either group)
```

**Intersection** (A and B):
```
Group A: "red_vertices"  (@Cd.r > 0.8)
Group B: "top_vertices"  (@P.y > 0.5)
  ↓
Group Combine: Intersection
  ↓
Result: "red_and_top" (only vertices in both groups)
```

**Subtraction** (A not B):
```
Group A: "all_faces"
Group B: "bottom_faces"
  ↓
Group Combine: Subtract
  ↓
Result: "not_bottom" (A minus B)
```

### Inverting Groups

Select everything **not** in group:

```
Group "selected"
  ↓
Group Combine: Invert
  ↓
Result: "not_selected"
```

---

## Common Patterns

### Checkerboard Selection

```
Grid (10×10 cells)
  ↓
Group:
  Mode: Expression
  Expression: (floor(@P.x) + floor(@P.z)) % 2 == 0
  Group: "checkers"
  ↓
Group Delete → Removes every other cell
```

### Ring Selection

```
Cylinder
  ↓
Group:
  Mode: Expression
  Expression: abs(@P.y - 0.5) < 0.1
  Group: "ring"
  ↓
Extrude → Only middle ring extrudes
```

### Border Selection

```
Grid
  ↓
Group:
  Mode: Expression
  Expression: abs(@P.x) > 0.9 or abs(@P.z) > 0.9
  Group: "border"
```

### Random Selection

```
Sphere
  ↓
Group:
  Mode: Expression
  Expression: rand(@id) > 0.5
  Group: "random_half"
```

### Height Gradient

```
Cylinder
  ↓
Group:
  Mode: Expression
  Expression: @P.y > $threshold
  Group: "above_threshold"
```

(Change $threshold to select different heights)

---

## Expression-Based Selection

### Comparison Operators

```
@P.y > 0.5          → Greater than
@P.y < 0.5          → Less than
@P.y >= 0.5         → Greater or equal
@P.y <= 0.5         → Less or equal
@P.y == 0.5         → Equal (exactly)
@P.y != 0.5         → Not equal
```

### Logical Operators

```
@P.y > 0 and @P.x > 0       → AND (both true)
@P.y > 0 or @P.x > 0        → OR (either true)
not (@P.y > 0)              → NOT (invert)
```

### Complex Conditions

```
(@P.y > 0.5 and @P.y < 0.8) or @Cd.r > 0.9
→ Select if (Y between 0.5 and 0.8) OR (very red)
```

### Math Functions

```
length(@P) < 2.0                → Within radius 2
abs(@P.x - @P.z) < 0.1          → Near diagonal
sin(@P.x * 10) > 0              → Wavy pattern
floor(@P.x) % 2 == 0            → Stripe pattern
```

---

## Workflows

### Delete Bottom Half

```
1. Sphere
2. Group:
   Expression: @P.y < 0
   Group: "bottom"
3. Group Delete
```

### Color Stripes

```
1. Grid
2. Group:
   Expression: floor(@P.x * 5) % 2 == 0
   Group: "stripes"
3. Attribute Create:
   Name: Cd
   Value: @stripes ? [1,0,0] : [0,0,1]
```

(Red and blue stripes)

### Extrude Windows

```
1. Box
2. Group:
   Expression: @P.z > 0.99  (front face)
   Group: "front"
3. Subdivide (to create face grid)
4. Group:
   Pattern: "0-100:3"  (every 3rd face)
   Group: "windows"
5. Extrude (inward for window recesses)
```

### Scatter on Selection

```
1. Grid
2. Group:
   Expression: rand(@id) > 0.7
   Group: "scatter_points"
3. Scatter (uses group as distribution)
```

---

## Group Attributes

Groups are stored as **point/face attributes**:

```
Group "selected" creates:
@selected = 1  (in group)
@selected = 0  (not in group)
```

**Access in expressions:**
```
@selected == 1  → Check if in group
@mygroup        → Direct group check
```

---

## Best Practices

### 1. Name Groups Clearly

```
✅ "top_faces", "window_locations", "border_verts"
❌ "group1", "sel", "temp"
```

### 2. Document Complex Expressions

```
Expression: (@P.y > $base_height) and (rand(@id) > $density)
Comment: "Select random points above base height"
```

### 3. Test on Simple Geometry

```
✅ Test expression on Sphere → Apply to complex model
❌ Write complex expression on 10k poly mesh (hard to debug)
```

### 4. Use Graph Parameters

```
Expression: @P.y > $select_threshold

Now $select_threshold is adjustable in UI!
```

### 5. Visualize Groups

Use colors to verify selections:
```
Group (create selection)
  ↓
Attribute Create:
  Name: Cd
  Value: @groupname ? [1,0,0] : [0,1,0]

Red = in group, Green = not in group
```

---

## Troubleshooting

### Group Not Selecting Anything

**Check:**
1. Expression syntax correct?
2. Attribute exists? (e.g., @P always exists)
3. Value ranges? (is @P.y ever > 0.5?)
4. Group type matches (point vs face)?

**Debug:**
```
Add Attribute Create: @debug = @P.y
View values in viewport
```

### Wrong Geometry Selected

**Fix:**
1. Simplify expression
2. Test components: `@P.y > 0` first, add more conditions later
3. Visualize with colors

### Group Not Affecting Operation

**Check:**
1. Does node support groups?
2. Group name matches?
3. Group was created upstream?

---

## Advanced Techniques

### Multi-Level Selection

```
1. Group A: "upper_half" (@P.y > 0)
2. Within A, Group B: "upper_corners" (@P.x > 0.5)
3. Result: Corners of upper half
```

### Animated Selection

*With graph parameters:*
```
Expression: @P.y < $cutoff_height

Animate $cutoff_height → Moving selection plane
```

### Geometric Patterns

```
Radial: length([@P.x, @P.z]) < $radius
Grid: (floor(@P.x) + floor(@P.z)) % 2 == 0
Spiral: atan2(@P.z, @P.x) + length([@P.x,@P.z]) < $param
```

### Neighbor-Based

*Future:* Select based on neighboring vertices

---

## Performance Tips

### Optimize Expressions

```
✅ @P.y > 0  (fast: single comparison)
❌ length(@P - [complex_calc]) > threshold  (slow: many ops)
```

### Cache Groups

If using same selection multiple times:
```
✅ Create group once → Use many times
❌ Recreate selection in every node
```

### Pattern vs Expression

```
Pattern mode: Fast for index-based selections
Expression mode: Flexible but slower for complex math
```

---

## Examples

### Delete Every Other Face

```
Grid
  ↓
Group:
  Mode: Pattern
  Pattern: "0-1000:2"
  ↓
Group Delete
```

### Color by Distance

```
Sphere
  ↓
Group:
  Expression: length(@P) < 0.8
  Group: "inner"
  ↓
Attribute Create:
  Cd: @inner ? [1,0,0] : [0,0,1]
```

### Mask with Noise

```
Grid
  ↓
Attribute Create:
  Name: noise_val
  Value: rand(@id)
  ↓
Group:
  Expression: @noise_val > $threshold
```

---

## Next Steps

- **Use Groups in Workflows** - [Procedural Patterns](../workflows/patterns.md)
- **See Group Node** - [Group Node Reference](../nodes/group/group.md)
- **Learn Attributes** - [Attributes](attributes.md)
- **Advanced Selection** - [Expression Syntax](../expressions/expression-syntax.md)

---

## Key Takeaways

✅ **Groups** = Named selections of geometry subsets
✅ **Types** = Point, Edge, Face (primitive) groups
✅ **Creation** = Pattern (indices) or Expression (attributes)
✅ **Operations** = Combine, Intersect, Subtract, Invert
✅ **Uses** = Targeted deletion, coloring, extrusion, etc.
✅ **Stored as** = Attributes (@groupname)

**Remember:** Groups are the key to selective operations - enabling complex procedural control!
