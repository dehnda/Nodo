# Procedural Patterns Workflow

Master the art of creating repeating patterns, tilings, and textures procedurally - essential for architectural details, decorative elements, and surface treatments.

## Overview

Learn to create:
- **Geometric patterns** (grids, hexagons, radial)
- **Organic variations** (randomness, noise-based)
- **Tiling systems** (seamless repetition)
- **3D surface patterns** (relief, embossing)

**Time:** 30-45 minutes per pattern type

---

## Pattern 1: Brick Wall

### Goal
Create a realistic brick pattern with offset rows and mortar gaps.

### Step 1: Single Brick

1. **Add Box** node
   - Size X: `0.2` (brick length)
   - Size Y: `0.1` (brick height)
   - Size Z: `0.05` (brick depth)

### Step 2: Horizontal Row

1. **Add Array** node
   - Mode: Linear
   - Count: `10`
   - Offset X: `0.21` (brick + mortar gap)
   - Offset Y: `0`
   - Offset Z: `0`

**Result:** Row of 10 bricks with gaps.

### Step 3: Vertical Stacking

1. **Add Array** node (second one)
   - Mode: Linear
   - Count: `8`
   - Offset X: `0`
   - Offset Y: `0.11` (brick height + mortar)
   - Offset Z: `0`

**Result:** 8 rows stacked vertically.

### Step 4: Running Bond Pattern

**Create offset on alternating rows:**

1. **Add Attribute Create** node (before vertical array)
   - Name: `row_offset`
   - Type: Float
   - Value: Click `≡` → `floor(@id / 10) % 2 * 0.105`

2. **Add Transform** node
   - Translate X: Click `≡` → `@row_offset`

**Result:** Classic brick pattern with offset rows!

### Step 5: Add Variation

**Randomize brick colors:**

1. **Add Attribute Create** node
   - Name: `Cd`
   - Type: Vector3
   - Value: `[0.6 + rand(@id) * 0.15, 0.3 + rand(@id+100) * 0.1, 0.2]`

**Result:** Subtle color variation per brick.

---

## Pattern 2: Hexagonal Tiling

### Goal
Create honeycomb/hex tile pattern.

### Step 1: Single Hexagon

1. **Add Cylinder** node
   - Radius: `0.5`
   - Height: `0.1`
   - Segments: `6` (hexagon!)
   - Cap: ✅ Top and bottom

### Step 2: Horizontal Row

1. **Add Array** node
   - Mode: Linear
   - Count: `8`
   - Offset X: `1.5` (1.5 × radius for tight packing)
   - Offset Y: `0`

### Step 3: Offset Second Row

**Hexagons pack with offset:**

1. **Duplicate** the array (Ctrl+D)
2. **Add Transform** node
   - Translate X: `0.75` (half offset)
   - Translate Z: `0.866` (√3/2 × radius for vertical spacing)

### Step 4: Stack Rows

1. **Merge** the two arrays
2. **Add Array** node (vertical)
   - Count: `6`
   - Offset Z: `1.732` (√3 × radius)

**Result:** Perfect hexagonal tiling!

### Step 5: Add Grout Lines

**Create gaps between tiles:**

1. **Add Transform** node (before merge)
   - Scale: `0.95` (shrink each tile 5%)

**Result:** Gaps appear as grout/mortar.

---

## Pattern 3: Radial Pattern

### Goal
Create circular/mandala-like pattern.

### Step 1: Petal/Segment

1. **Add Box** node
   - Size X: `0.2`
   - Size Y: `1.0`
   - Size Z: `0.1`

2. **Add Transform** node
   - Translate Y: `0.5` (offset from center)

### Step 2: Radial Array

1. **Add Array** node
   - Mode: Radial
   - Count: Click `≡` → `$petal_count` (create graph param, default 12)
   - Axis: `Z` (perpendicular to pattern)
   - Angle: `360` degrees

**Result:** Flower-like radial pattern!

### Step 3: Add Inner Ring

1. **Duplicate** array
2. **Add Transform** node
   - Scale: `0.5`
   - Rotate Z: `15` degrees (offset from outer ring)

3. **Merge** with outer ring

### Step 4: Center Detail

1. **Add Cylinder** node
   - Radius: `0.3`
   - Height: `0.1`

2. **Merge** with pattern

**Result:** Multi-layer radial design!

---

## Pattern 4: Checkerboard

### Goal
Create 3D checkerboard with raised tiles.

### Step 1: Base Grid

1. **Add Grid** node
   - Segments X: `8`
   - Segments Z: `8`
   - Size X: `8`
   - Size Z: `8`

### Step 2: Select Alternating Tiles

1. **Add Group** node
   - Mode: Expression
   - Expression: `(floor(@P.x + 4) + floor(@P.z + 4)) % 2 == 0`
   - Group: "white_tiles"

### Step 3: Extrude White Tiles

1. **Add Extrude** node (if operates on groups)
   - Distance: `0.2`

Or create two separate grids (white raised, black flat) and merge.

### Step 4: Add Colors

1. **Add Attribute Create** node
   - Name: `Cd`
   - Type: Vector3
   - Value: `@white_tiles ? [0.9, 0.9, 0.9] : [0.1, 0.1, 0.1]`

**Result:** 3D checkerboard pattern!

---

## Pattern 5: Organic Scatter

### Goal
Natural-looking scattered elements (pebbles, grass clumps).

### Step 1: Base Surface

1. **Add Grid** node
   - Segments X: `20`
   - Segments Z: `20`
   - Size: `10 × 10`

### Step 2: Create Element to Scatter

1. **Add Sphere** node (or any object)
   - Radius: Click `≡` → `0.1 + rand($seed) * 0.05`

### Step 3: Scatter on Surface

1. **Add Scatter** node
   - Input A: Grid (surface)
   - Input B: Sphere (object to scatter)
   - Count: Click `≡` → `$scatter_density` (graph param, default 100)
   - Random Seed: `$seed`

### Step 4: Add Variation

**Randomize scale and rotation:**

1. **Add Attribute Create** node (on scattered instances)
   - Name: `scale_var`
   - Value: `0.8 + rand(@id) * 0.4`

2. **Add Transform** node
   - Scale: Click `≡` → `@scale_var`
   - Rotate Z: `rand(@id + 1000) * 360`

**Result:** Natural-looking distribution with variation!

---

## Pattern 6: Wave/Ripple

### Goal
Sinusoidal wave pattern on surface.

### Step 1: Base Grid

1. **Add Grid** node
   - Segments X: `50`
   - Segments Z: `50`
   - Size: `10 × 10`

### Step 2: Apply Wave Deformation

**Displace vertices using sine function:**

1. **Add Attribute Create** node
   - Name: `wave_height`
   - Type: Float
   - Value: `sin(@P.x * $frequency) * $amplitude`

2. **Add Transform** or displacement (future feature)

**Current workaround:** Use expression for Y position:
```
Create new position attribute:
@new_y = @P.y + sin(@P.x * 2.0) * 0.5
```

### Step 3: Add Second Wave (Perpendicular)

```
@wave = sin(@P.x * $freq_x) * sin(@P.z * $freq_z) * $amplitude
```

**Result:** Ripple/interference pattern!

### Step 4: Color by Height

1. **Add Attribute Create** node
   - Name: `Cd`
   - Value: `[0, @P.y + 0.5, 1 - @P.y]`

**Result:** Blue-to-white gradient wave!

---

## Pattern 7: Voronoi/Cell Pattern

### Goal
Organic cell-like divisions (future feature, manual approach).

### Current Approach: Scatter + Regions

1. **Add Grid** (dense)
   - Segments: `100 × 100`

2. **Scatter** points (seeds)
   - Count: `20`

3. **Attribute Create** (distance to nearest seed)
   - Requires custom logic (future feature)

**Simpler alternative:** Use radial gradients from scattered points.

---

## Advanced Techniques

### Seamless Tiling

**For repeating patterns:**

1. **Ensure dimensions match:**
   - Pattern width = exact multiple of unit size
   - Pattern height = exact multiple of unit size

2. **Wrap-around offsets:**
   ```
   X offset at edge = X offset at start
   (modulo pattern width)
   ```

3. **Test tiling:**
   - Array the pattern
   - Check for visible seams

### Combining Patterns

**Layer multiple patterns:**

```
Base pattern (large scale)
  ↓ Merge
Detail pattern (small scale)
  ↓ Boolean (subtract for cutouts)
Final composite
```

### Parametric Control

**Create graph parameters:**

```
$pattern_scale = 1.0     (overall size)
$pattern_density = 8     (repetitions)
$variation_seed = 42     (randomness)
$detail_level = 0.5      (complexity)
```

Link all pattern parameters to these globals.

---

## Best Practices

### 1. Start Simple

```
✅ Single element → Array → Variations
❌ Complex pattern all at once
```

### 2. Use Graph Parameters

```
✅ $brick_width, $mortar_gap, $row_offset
❌ Hard-coded values
```

### 3. Test Tiling Early

```
Array pattern 3×3
Check for seams/misalignment
Fix before adding detail
```

### 4. Optimize Poly Count

```
Brick wall:
- High detail: 500 tris per brick
- Low detail: 12 tris per brick

Use $detail_level to switch!
```

### 5. Color for Clarity

```
While building:
  Cd = @group_id (color by pattern unit)

Final:
  Cd = actual material color
```

---

## Common Patterns Reference

### Geometric

| Pattern | Base Shape | Array Type | Key Parameter |
|---------|-----------|------------|---------------|
| Grid | Box | Linear 2D | Spacing X, Z |
| Hexagon | Cylinder (6-seg) | Hex offset | Radius |
| Radial | Any | Radial | Count, angle |
| Checkerboard | Grid + Group | Expression | Alternation formula |

### Organic

| Pattern | Technique | Key Element |
|---------|-----------|-------------|
| Scatter | Scatter node | Random seed |
| Waves | Sine function | Frequency, amplitude |
| Noise | Random per vertex | Noise function |
| Clusters | Grouped scatter | Density map |

---

## Troubleshooting

### Pattern Not Aligning

**Fix:**
- Check offset calculations
- Ensure consistent spacing
- Use exact mathematical spacing (e.g., √3/2 for hex)

### Gaps or Overlaps

**Fix:**
- Adjust scale (0.95 for gaps, 1.05 for overlap)
- Check array offset matches element size
- Test with simple shapes first

### Too Many Polygons

**Fix:**
- Reduce segments on base shapes
- Lower array counts
- Use LOD via `$detail_level`

### Pattern Looks Repetitive

**Fix:**
- Add randomization: `rand(@id)`
- Vary: size, rotation, position
- Use multiple seeds

---

## Export for Textures

### Bake to Texture

1. Create pattern in 3D
2. Render from orthographic camera
3. Use as:
   - Diffuse map
   - Normal map (with height variation)
   - Displacement map

### Use as Relief

Keep as 3D geometry:
- Low poly base
- Pattern as surface detail
- Export for close-up use

---

## Learning Outcomes

After this tutorial, you can:

✅ Create **geometric patterns** (grids, hex, radial)
✅ Build **organic scatter** patterns
✅ Use **expressions** for mathematical patterns
✅ Apply **variation and randomness**
✅ Design **tileable, seamless** patterns

---

## Next Steps

- **Apply to Architecture** - [Architectural Modeling](architectural.md)
- **Use in Assets** - [Game Assets Workflow](game-assets.md)
- **Master Attributes** - [Attributes](../concepts/attributes.md)
- **Expression Power** - [Expression Syntax](../expressions/expression-syntax.md)

---

## Pattern Library Ideas

Build a reusable collection:

**Flooring:**
- Wood planks
- Tile grid
- Stone blocks
- Checkerboard

**Walls:**
- Brick (running bond)
- Brick (stack bond)
- Stone masonry
- Panel grid

**Decorative:**
- Radial medallions
- Geometric rosettes
- Celtic knots (advanced)
- Lattice screens

**Organic:**
- Pebble scatter
- Grass clumps
- Rock formations
- Bark texture

---

## Key Takeaways

✅ **Patterns = Element + Array + Variation**
✅ **Math functions** enable complex patterns
✅ **Randomness** prevents repetition
✅ **Graph parameters** make patterns flexible
✅ **Groups + expressions** enable selective operations

**Remember:** A pattern is a **system**, not a static design!
