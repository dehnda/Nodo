# Game Assets Workflow

Create optimized, game-ready 3D assets with LOD (Level of Detail) variants and clean topology - perfect for Unity, Unreal Engine, or custom game engines.

## Project Goal

Build a **procedural crate** asset with:
- Clean, game-ready topology
- Multiple LOD levels
- Configurable detail parameters
- Optimized poly count
- Export-ready geometry

**Time:** 30-40 minutes

---

## Part 1: Base Crate

### Step 1: Setup Graph Parameters

Create control parameters first:

| Name | Type | Default | Min | Max | Description |
|------|------|---------|-----|-----|-------------|
| `crate_size` | Float | 1.0 | 0.5 | 3.0 | Crate dimensions |
| `detail_level` | Float | 0.5 | 0.0 | 1.0 | LOD control (0=low, 1=high) |
| `plank_inset` | Float | 0.05 | 0.0 | 0.2 | Wood plank depth |
| `corner_bevel` | Float | 0.02 | 0.0 | 0.1 | Edge rounding |

### Step 2: Create Base Box

1. **Add Box** node
   - Size X: `$crate_size`
   - Size Y: `$crate_size`
   - Size Z: `$crate_size`
   - Center: ✅ Enabled

**Result:** 1×1×1 meter cube (12 triangles - very low poly!)

### Step 3: Add Wood Plank Details

**Create planks on each face:**

1. **Add Grid** node
   - Segments X: `1`
   - Segments Z: Click `≡` → `ceil($detail_level * 5 + 1)`
   - Size X: `$crate_size - 0.1`
   - Size Z: `$crate_size - 0.1`

2. **Add Transform** node (for positioning)
   - Translate X: `$crate_size / 2 + 0.01`
   - Translate Y: `0`
   - Translate Z: `0`

This creates plank lines on one face.

### Step 4: Duplicate for All 6 Faces

**Quick approach:** Use Array with rotation

1. **Add Array** node
   - Mode: Radial
   - Count: `4`
   - Axis: `Y`
   - Angle: `90` (degrees)

Creates planks on 4 sides.

2. **Manually add** top and bottom planks (rotate 90° on X axis)

Or use Transform nodes for precise control.

---

## Part 2: LOD System

### Step 5: LOD0 (High Detail)

**When `$detail_level >= 0.7`:**

1. **Add Subdivide** node (conditional)
   - Connect: Box → Subdivide
   - Subdivisions: Click `≡` → `$detail_level > 0.7 ? 1 : 0`

2. **Add Bevel** (if available) or Smooth
   - Amount: `$corner_bevel`

**Poly count:** ~200-500 triangles

### Step 6: LOD1 (Medium Detail)

**When `$detail_level 0.3-0.7`:**

- Base box with plank lines
- No subdivision
- Minimal beveling

**Poly count:** ~50-100 triangles

### Step 7: LOD2 (Low Detail)

**When `$detail_level < 0.3`:**

- Just the base box
- No plank details
- Sharp edges

**Poly count:** 12 triangles

### Step 8: Automatic LOD Switching

**Use conditional merging:**

```
Base Box
  ↓
[Detail Level Check]
  ↓ if $detail_level > 0.3
Add Plank Details
  ↓ if $detail_level > 0.7
Add Subdivision + Bevel
```

**Expression example:**
```
Plank count = $detail_level > 0.3 ? 5 : 0
```

---

## Part 3: Surface Details

### Step 9: Add Damage/Wear (Optional)

**Create dent geometry:**

1. **Add Sphere** (small)
   - Radius: `0.1`

2. **Add Scatter** node
   - Target: Crate surface
   - Count: Click `≡` → `$detail_level > 0.5 ? 5 : 0`
   - Random seed: `42`

3. **Add Boolean** (Subtract)
   - Crate - Dents = Damaged crate

**Result:** Procedural wear marks (only at high detail!)

### Step 10: Metal Corner Brackets

1. **Add Box** (thin, corner piece)
   - Size X: `0.05`
   - Size Y: `$crate_size * 0.3`
   - Size Z: `0.05`

2. **Add Array** (8 corners)
   - Position at corners using Transform

3. **Merge** with crate

**Poly budget:** ~20 triangles per bracket × 8 = 160 tris

---

## Part 4: Optimization

### Step 11: Clean Geometry

**Critical for game engines:**

1. **Check manifold status:**
   - All edges have exactly 2 faces
   - No holes or gaps
   - Proper normals

2. **Weld vertices:**
   - Use Merge node with weld enabled
   - Threshold: `0.001`

3. **Remove interior faces:**
   - Group (select internal geometry)
   - Group Delete

### Step 12: Poly Count Targets

**Game industry standards:**

| LOD Level | Poly Count | Usage |
|-----------|-----------|-------|
| **LOD0** (High) | 500-2000 tris | Close-up view (< 5m) |
| **LOD1** (Med) | 100-500 tris | Medium distance (5-20m) |
| **LOD2** (Low) | 12-100 tris | Far distance (> 20m) |

**Your crate:**
- LOD0: ~500 tris (detailed planks, bevels)
- LOD1: ~100 tris (plank lines only)
- LOD2: ~12 tris (just box)

### Step 13: UV Coordinates

**For texturing:**

1. **Check if UVs exist:**
   - Some generators create UVs automatically
   - Box, Grid usually include basic UVs

2. **Simple box mapping:**
   - Each face gets 0-1 UV space
   - Good enough for tiling textures

**Advanced:** UV unwrapping (future Nodo feature)

---

## Part 5: Material Setup

### Step 14: Create Material IDs

**For multi-material export:**

1. **Add Group** node
   - Expression: `@P.y > $crate_size / 2 - 0.05`
   - Group: "metal_brackets"

2. **Add Attribute Create** node
   - Name: `materialid`
   - Type: Int
   - Value: `@metal_brackets ? 2 : 1`

**Result:**
- Material 1 = Wood planks
- Material 2 = Metal brackets

### Step 15: Vertex Colors (Optional)

**For vertex-color shading:**

```
Attribute Create:
  Name: Cd
  Type: Vector3
  Value: [0.6, 0.4, 0.2]  # Wood brown color
```

**Variation:**
```
Random tint per vertex:
  Cd: [0.6 + rand(@id) * 0.1, 0.4, 0.2]
```

---

## Part 6: Export

### Step 16: Export OBJ/FBX

**For Unity/Unreal:**

1. **File → Export → OBJ**
2. Options:
   - ✅ Export normals (for lighting)
   - ✅ Export UVs (for textures)
   - ✅ Export material IDs (if multi-mat)
   - ❌ Flip Y-axis (depends on engine)

3. **Filename convention:**
   ```
   crate_LOD0.obj  (high detail)
   crate_LOD1.obj  (medium)
   crate_LOD2.obj  (low)
   ```

### Step 17: Import to Game Engine

**Unity:**
1. Drag OBJ into Assets
2. Set LOD Group component
3. Assign LOD0, LOD1, LOD2 meshes
4. Set distance thresholds: LOD0 (0-10m), LOD1 (10-30m), LOD2 (30m+)

**Unreal:**
1. Import as Static Mesh
2. LOD Settings → Import LOD levels
3. Auto-generate collisions

---

## Variations

### Crate Variants from One Graph

**Create multiple crates:**

Change `$crate_size`:
- Small: `0.5` → Pickup item
- Medium: `1.0` → Standard prop
- Large: `2.0` → Shipping container

Change detail:
- Hero asset: `$detail_level = 1.0`
- Background: `$detail_level = 0.2`

### Different Styles

**Wood slat crate:**
```
Plank count: 7
Gaps: Visible
```

**Metal shipping container:**
```
Material: Metal
Plank pattern: Horizontal ribs
Corner rivets: Spheres at edges
```

**Cardboard box:**
```
Material: Cardboard
No metal brackets
Tape strips on top
```

---

## Performance Optimization

### Poly Budget

**For real-time games:**

✅ **Do:**
- Start with lowest poly possible
- Add detail only where visible
- Use LOD system
- Merge vertices

❌ **Don't:**
- Subdivide entire mesh
- Add detail that won't be seen
- Keep interior faces
- Forget to weld

### Texture Budget

**1024×1024 texture can cover:**
- 10-20 similar crates
- Use texture atlases
- Share materials

### Draw Calls

**Merge similar assets:**
- 100 separate crates = 100 draw calls ❌
- 100 crates batched = 1 draw call ✅

---

## Best Practices

### 1. Design for Performance

```
✅ Low base poly + detail layers
❌ High poly everywhere
```

### 2. Test at Target Distance

```
View crate from:
- 1m (player close)
- 10m (gameplay distance)
- 50m (background)

Is LOD switching smooth?
```

### 3. Clean Topology

```
✅ Quads/tris (no n-gons)
✅ Manifold geometry
✅ Proper normals
❌ Overlapping faces
❌ Zero-area triangles
```

### 4. Modular Design

```
Create library:
- Crate base (reusable)
- Lid (separate)
- Brackets (instances)
- Damage (scatter layer)
```

### 5. Naming Convention

```
prop_crate_01_LOD0.obj
prop_crate_01_LOD1.obj
prop_crate_damaged_LOD0.obj
```

---

## Common Game Asset Types

### Props

**What:** Small environmental objects

**Examples:** Crates, barrels, rocks, furniture

**Technique:**
- Base shape (Box, Cylinder, Sphere)
- Detail via boolean or merge
- LOD variants
- 50-500 tris average

### Modular Architecture

**What:** Tileable building pieces

**Examples:** Wall sections, floors, doorways

**Technique:**
- Exact dimensions (1m grid)
- Snap together
- Shared materials
- 100-1000 tris per piece

### Vegetation

**What:** Trees, bushes, grass

**Examples:** Foliage cards, branch clusters

**Technique:**
- Crossed planes for grass
- Billboard trees for distance
- Alpha transparency
- Very low poly (<50 tris for grass cards)

---

## Troubleshooting

### Poly Count Too High

**Fix:**
- Reduce subdivisions
- Lower `$detail_level`
- Remove unnecessary detail
- Check for duplicate geometry

### Normals Look Wrong

**Fix:**
- Recalculate normals (Smooth node)
- Check winding order (CCW for front faces)
- Ensure manifold geometry

### Export Issues

**Common:**
- **Flipped Y-axis:** Change export setting
- **Scale wrong:** Check units (meters vs cm)
- **Materials missing:** Ensure materialid attribute exists

---

## Learning Outcomes

After this tutorial, you can:

✅ Create **game-ready assets** with clean topology
✅ Implement **LOD systems** for performance
✅ Optimize **poly counts** for real-time rendering
✅ Export **multi-material meshes** for game engines
✅ Design **modular, reusable** asset graphs

---

## Next Steps

- **Advanced Architecture** - [Architectural Modeling](architectural.md)
- **Pattern Creation** - [Procedural Patterns](patterns.md)
- **Boolean Techniques** - [Boolean Node](../nodes/boolean/boolean.md)
- **Attribute Usage** - [Attributes](../concepts/attributes.md)

---

## Key Takeaways

✅ **LOD is essential** for game performance
✅ **Start low poly** and add detail conditionally
✅ **Clean topology** = No problems in engine
✅ **Graph parameters** enable variants from one graph
✅ **Procedural workflow** = Fast iteration

**Remember:** A good game asset is **optimized** first, **pretty** second!
