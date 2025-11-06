# Graph Parameters

Graph Parameters are reusable controls that can drive multiple nodes in your network. They're the foundation of parametric, data-driven modeling in Nodo.

## What are Graph Parameters?

Graph Parameters let you:

- **Define once, use everywhere** - Create a parameter that controls multiple nodes
- **Build intelligent models** - Make your procedural systems respond to high-level controls
- **Create presets** - Save different parameter sets for quick iterations
- **Share controls** - Let one slider affect many operations

Think of them as "master controls" for your node graph.

---

## Creating Graph Parameters

### Using the Graph Parameters Panel

1. Open **Graph Parameters** panel (View → Panels → Graph Parameters)
2. Click the **"+"** button (Add Parameter) in the toolbar
3. Enter parameter details:
   - **Name**: Unique identifier (e.g., `base_radius`, `tower_height`)
   - **Type**: Float, Int, Bool, String, or Vector3
   - **Default Value**: Initial value
   - **Range** (for numeric types): Min and max values
4. Click **OK**

The parameter now appears in the Graph Parameters panel and can be referenced in expressions.

!!! tip "Edit or Delete"
    - **Double-click** a parameter to edit it
    - **Select** and click **"-"** to delete (warns if in use)

---

## Using Graph Parameters

### Referencing in Expressions

Once created, reference graph parameters in any expression:

```
$param_name
```

Or using the full path:

```
ch("/graph/param_name")
```

### Example: Controlled Scale

**Setup:**
1. Create graph parameter: `scale_factor` (Float, default 1.0)
2. Add a Sphere node
3. Set Sphere's `radius` expression: `$scale_factor`
4. Add a Box node
5. Set Box's `size_x` expression: `$scale_factor * 2`

Now one slider controls both shapes!

---

## Graph Parameter Types

### Float

Decimal numbers with optional range.

**Properties:**
- Default value
- Min/Max range
- UI widget: Slider or number field

**Example:**
```
Name: detail_level
Type: Float
Default: 1.0
Range: 0.1 to 5.0
```

**Usage:**
```
ch("/graph/detail_level") * 32  # Scale segments based on detail
```

### Int

Whole numbers.

**Properties:**
- Default value
- Min/Max range
- UI widget: Number field or slider

**Example:**
```
Name: copy_count
Type: Int
Default: 6
Range: 1 to 100
```

**Usage:**
```
$copy_count  # Direct use in Array count
```

### Bool

True/false toggle.

**Properties:**
- Default value (on/off)
- UI widget: Checkbox

**Example:**
```
Name: enable_detail
Type: Bool
Default: true
```

**Usage:**
```
$enable_detail ? 64 : 16  # Conditional segments
```

### String

Text values.

**Properties:**
- Default text
- UI widget: Text field

**Example:**
```
Name: export_prefix
Type: String
Default: "model"
```

**Usage:**
```
$export_prefix + "_v01.obj"  # Build file paths
```

### Vector (Vec3)

Three float values (X, Y, Z).

**Properties:**
- Default X, Y, Z
- Optional range per component
- UI widget: Three number fields

**Example:**
```
Name: global_offset
Type: Vec3
Default: (0, 0, 0)
```

**Usage:**
```
ch("/graph/global_offset.x")  # Access X component
ch("/graph/global_offset.y")  # Access Y component
```

---

## Expression Examples

### Simple Reference

```
$radius  # Use graph parameter directly
```

### Math with Parameters

```
$base_size * 2.5  # Scale up
sin($angle) * $amplitude  # Trigonometry
```

### Combining Multiple Parameters

```
$width * $height * $depth  # Volume calculation
min($max_size, $input_value)  # Clamping
```

### Conditional Logic

```
$enable_feature ? $detail_high : $detail_low
```

### Vector Components

```
ch("/graph/offset.x") + 10  # Offset X by 10
length(ch("/graph/position"))  # Vector magnitude
```

---

## Common Workflows

### Master Scale Control

Create a single scale parameter that affects everything:

1. Graph parameter: `master_scale` (Float, default 1.0)
2. All size-related parameters use: `base_value * $master_scale`

**Example:**
- Sphere radius: `1.0 * $master_scale`
- Box size: `2.0 * $master_scale`
- Array spacing: `3.0 * $master_scale`

### Detail Level System

Control model complexity with one slider:

1. Graph parameter: `detail` (Float, 0.0 to 1.0)
2. Use for subdivision, segments, etc.

**Example:**
```
segments = int(8 + $detail * 56)  # 8 to 64 segments
subdivisions = floor($detail * 3)  # 0 to 3 subdivisions
```

### Feature Toggles

Enable/disable parts of your model:

1. Graph parameter: `include_details` (Bool, default true)
2. Use in Switch nodes or conditionals

**Example:**
```
$include_details ? 1 : 0  # Switch node input selection
```

### Export Variants

Drive multiple export configurations:

1. Graph parameters: `lod_level` (Int), `export_name` (String)
2. Use in Export node paths and settings

---

## Best Practices

### Naming Conventions

!!! tip "Use Clear Names"
    - ✅ `tower_height`, `wall_thickness`, `door_count`
    - ❌ `var1`, `temp`, `x`

!!! tip "Group Related Parameters"
    Use prefixes: `tower_height`, `tower_radius`, `tower_segments`

### Organize Parameters

Group by purpose:
- **Global:** `master_scale`, `detail_level`
- **Dimensions:** `width`, `height`, `depth`
- **Features:** `enable_windows`, `enable_roof`
- **Export:** `export_format`, `export_quality`

### Set Sensible Ranges

!!! warning "Always Set Ranges"
    Unbounded parameters can cause issues:
    - Set realistic min/max values
    - Prevents accidental extreme values
    - Improves UI (sliders work better)

**Example:**
```
height: 0.1 to 100.0  # Building height in meters
segments: 3 to 128    # Geometry detail
```

### Document Your Parameters

Use descriptions to explain what each parameter controls:

```
Name: wall_thickness
Type: Float
Range: 0.05 to 2.0
Description: "Thickness of outer walls in meters"
```

---

## Advanced Techniques

### Derived Parameters

Create parameters that calculate from others:

```
# Graph parameter: base_radius
# Another node uses:
diameter = $base_radius * 2
circumference = $base_radius * 2 * pi
```

### Ratio-Based Design

Lock proportions using ratios:

```
# Graph parameter: base_width
height = $base_width * 1.618  # Golden ratio
depth = $base_width * 0.5     # Half width
```

### Responsive Systems

Make geometry adapt to changes:

```
# More copies = smaller size (maintain total coverage)
copy_size = $total_size / $copy_count
```

### Multi-Stage Control

Chain parameters through multiple nodes:

```
1. Graph param: complexity (0-1)
2. Node A: segments = 8 + $complexity * 56
3. Node B: subdivisions = floor($complexity * 3)
4. Node C: scatter_count = int(100 + $complexity * 900)
```

---

## Graph Parameters Panel

### Panel Features

- **Add Parameter** - Create new graph parameter
- **Edit Parameter** - Change name, type, range, default
- **Delete Parameter** - Remove unused parameters
- **Reorder** - Drag to organize

### Editing Parameters

Click a parameter to edit:
- **Name** - Identifier used in expressions
- **Label** - Display name in UI
- **Type** - Float, Int, Bool, String, Vec3
- **Default** - Initial value
- **Range** - Min/max bounds (for numeric types)
- **Description** - Tooltip help text

---

## Troubleshooting

### "Parameter not found" error

**Cause:** Parameter name misspelled or doesn't exist.

**Fix:**
- Check spelling: `$radius` vs `$raduis`
- Verify parameter exists in Graph Parameters panel
- Use exact name (case-sensitive)

### Expression doesn't update

**Cause:** Parameter reference is wrong, or expression is cached.

**Fix:**
- Click parameter to refresh
- Check expression syntax
- Try re-entering the expression

### Parameter has no effect

**Cause:** No nodes are using the parameter.

**Fix:**
- Reference it in node parameter expressions: `$param_name`
- Verify expressions are in Expression Mode (blue)

### Can't delete graph parameter

**Cause:** Parameter is still referenced by nodes.

**Fix:**
- Find all references (future: show usage count)
- Remove references first
- Then delete the parameter

---

## Examples

### Architectural Column

Create a parametric column:

**Graph Parameters:**
```
height: 3.0 (Float, 0.5 to 20.0)
radius: 0.3 (Float, 0.1 to 2.0)
flutes: 12 (Int, 4 to 32)
```

**Node Setup:**
1. Cylinder (height: `$height`, radius: `$radius`)
2. Array Radial (count: `$flutes`)
3. Boolean Subtract (create fluting)

### Procedural Building

**Graph Parameters:**
```
floors: 5 (Int, 1 to 50)
floor_height: 3.0 (Float, 2.5 to 5.0)
width: 10.0 (Float, 5.0 to 50.0)
```

**Expressions:**
```
building_height = $floors * $floor_height
window_spacing = $floor_height * 0.8
```

### Export Variants

**Graph Parameters:**
```
lod: 1 (Int, 0 to 3)  # Level of detail
export_name: "asset" (String)
```

**Expressions:**
```
segments = 8 * pow(2, $lod)  # 8, 16, 32, 64
filename = $export_name + "_lod" + str($lod) + ".obj"
```

---

## See Also

- [Expression Syntax](expression-syntax.md) - Full expression language reference
- [Math Functions](math-functions.md) - Available functions
- [Channel References](ch-references.md) - Using `ch()` to reference parameters

---

**Next Steps:** Learn the [Expression Syntax](expression-syntax.md) to unlock the full power of graph parameters.
