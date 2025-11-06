# Architectural Modeling Workflow

Learn to create procedural buildings with adjustable parameters - perfect for architectural visualization, game levels, and city generation.

## Project Goal

Build a **parametric office building** with:
- Adjustable height and floor count
- Automatic window placement
- Entrance area
- Roof cap
- Full control via graph parameters

**Time:** 45-60 minutes

---

## Part 1: Foundation & Core Structure

### Step 1: Base Building Volume

1. **Add Box** node (Tab → "box")
   - Size X: `10`
   - Size Y: `30` (initial height)
   - Size Z: `10`

2. **Add Transform** node
   - Connect: Box → Transform
   - Translate Y: `15` (lift base to ground)

!!! tip "Why Transform?"
    Keeps the building base at Y=0 (ground level), making it easier to work with.

### Step 2: Create Graph Parameters

Open **View → Panels → Graph Parameters**, add:

| Name | Type | Default | Min | Max | Description |
|------|------|---------|-----|-----|-------------|
| `building_width` | Float | 10.0 | 5.0 | 50.0 | Building width (X/Z) |
| `building_height` | Float | 30.0 | 10.0 | 200.0 | Total height |
| `floor_count` | Int | 10 | 3 | 50 | Number of floors |
| `floor_height` | Float | 3.0 | 2.5 | 4.0 | Height per floor |

### Step 3: Link Parameters to Base

**Update Box node:**
- Click `≡` button next to Size X → Expression mode
- Size X: `$building_width`
- Size Y: `$floor_count * $floor_height`
- Size Z: `$building_width`

**Update Transform node:**
- Translate Y: `$building_height / 2`

**Test:** Change `$floor_count` from 10 to 20 → Building doubles in height!

---

## Part 2: Window Array

### Step 4: Create Window Recess

1. **Add Grid** node
   - Segments X: `1`
   - Segments Z: `1`
   - Size X: `0.8`
   - Size Z: `0.6`

2. **Add Extrude** node
   - Connect: Grid → Extrude
   - Distance: `-0.2` (extrude inward)

This creates a single window recess.

### Step 5: Array Windows Horizontally

1. **Add Array** node
   - Connect: Extrude → Array
   - Mode: Linear
   - Count: Click `≡` → `floor($building_width / 2.5)`
   - Offset X: `2.5`
   - Offset Y: `0`
   - Offset Z: `0`

**Result:** Windows space evenly along building width.

### Step 6: Array Windows Vertically

1. **Add Array** node (second one)
   - Connect: Array_1 → Array_2
   - Mode: Linear
   - Count: `$floor_count`
   - Offset X: `0`
   - Offset Y: `$floor_height`
   - Offset Z: `0`

**Result:** Windows on every floor!

### Step 7: Position Window Grid

1. **Add Transform** node
   - Connect: Array_2 → Transform
   - Translate X: `$building_width / 2 + 0.1` (position on front face)
   - Translate Y: `$floor_height / 2` (center on first floor)
   - Translate Z: `0`
   - Rotate Y: `0` (front face)

---

## Part 3: Boolean Subtraction

### Step 8: Subtract Windows from Building

1. **Add Boolean** node
   - Input A: Transform_1 (the building base)
   - Input B: Transform_2 (the window array)
   - Operation: **Subtract**

**Result:** Windows cut into building facade!

!!! warning "Boolean Order Matters"
    A subtract B ≠ B subtract A

    Building - Windows = Building with holes ✅
    Windows - Building = Just windows ❌

### Step 9: Add Windows to All Sides

For back/left/right facades, we need 3 more window arrays.

**Quick method:**
1. Select Transform_2 node (the window positioning)
2. **Ctrl + D** to duplicate
3. Rename to "windows_back"
4. Rotate Y: `180` (face opposite direction)

Repeat for left (Rotate Y: `90`) and right (Rotate Y: `-90`).

**Update Boolean:**
- Connect all 4 window arrays to Input B (multiple connections allowed)
- Or use multiple Boolean nodes in sequence

---

## Part 4: Entrance

### Step 10: Create Entrance Door

1. **Add Box** node
   - Size X: `2.0`
   - Size Y: `3.0`
   - Size Z: `0.3`

2. **Add Transform** node
   - Connect: Box → Transform
   - Translate X: `$building_width / 2 + 0.05`
   - Translate Y: `1.5`
   - Translate Z: `0`

### Step 11: Entrance Overhang

1. **Add Box** node
   - Size X: `3.0`
   - Size Y: `0.3`
   - Size Z: `1.5`

2. **Add Transform** node
   - Translate X: `$building_width / 2`
   - Translate Y: `3.2`
   - Translate Z: `0`

### Step 12: Combine Entrance Elements

1. **Add Merge** node
   - Connect: Door Transform → Merge
   - Connect: Overhang Transform → Merge
   - Connect: Merge → Boolean (Input B, for subtraction)

**Result:** Door hole with overhang!

---

## Part 5: Roof Detail

### Step 13: Roof Cap

1. **Add Box** node
   - Size X: `$building_width + 0.5`
   - Size Y: `1.0`
   - Size Z: `$building_width + 0.5`

2. **Add Transform** node
   - Translate Y: `$building_height + 0.5`

3. **Add Boolean** or **Merge** node
   - Combine roof with main building
   - Use Merge for separate piece, Boolean Union for solid

---

## Part 6: Refinement & Details

### Step 14: Add Ground Plane

1. **Add Grid** node
   - Size X: `50`
   - Size Z: `50`
   - Segments: `1`

2. **Add Transform** node
   - Translate Y: `0` (at ground level)

3. **Merge** with building

### Step 15: Material Zones

**Optional:** Add material IDs for multi-material export.

1. **Add Group** node after building
   - Expression: `@P.y < 1.0`
   - Group: "ground_floor"

2. **Add Attribute Create** node
   - Name: `materialid`
   - Type: Int
   - Value: `@ground_floor ? 2 : 1`

**Result:** Ground floor gets material ID 2, upper floors ID 1.

---

## Part 7: Variations & Polish

### Step 16: Add Window Variations

Create **graph parameter:**

| Name | Type | Default | Description |
|------|------|---------|-------------|
| `window_spacing` | Float | 2.5 | Space between windows |
| `window_width` | Float | 0.8 | Window width |
| `window_depth` | Float | 0.2 | Window recess depth |

Update window Grid:
- Size X: `$window_width`

Update window Array:
- Offset X: `$window_spacing`

**Now:** Adjust window density globally!

### Step 17: Add Balconies (Optional)

1. **Add Box** → balcony platform
2. **Array** → every 3rd floor: `floor(@id / ($floor_count / 3))`
3. **Merge** with building

### Step 18: Final Assembly

**Organize your graph:**

```
[Base Structure] → [Window Subtraction] → [Entrance] → [Roof] → [Final Merge]
```

**Name all nodes** for clarity:
- "base_volume"
- "window_grid_front"
- "entrance_door"
- "roof_cap"
- etc.

---

## Testing & Iteration

### Test Parameter Ranges

Try extreme values:

- `$floor_count = 3` → Short building
- `$floor_count = 50` → Skyscraper
- `$building_width = 5` → Narrow tower
- `$building_width = 40` → Wide complex

**Check:**
- Windows still align properly?
- Entrance proportional?
- Roof caps correctly?

### Common Issues

**Windows overlap:**
- Increase `$window_spacing`
- Or link to `$building_width / $window_count`

**Building too short/tall:**
- Ensure `Size Y = $floor_count * $floor_height`

**Boolean fails:**
- Check both inputs are manifold (closed meshes)
- Simplify geometry if too complex

---

## Enhancements

### Level of Detail (LOD)

Add graph parameter `$detail_level`:

```
Window segments = $detail_level > 0.5 ? 8 : 4
Boolean quality = $detail_level
```

Low detail for preview, high for final render.

### Randomization

Add variety with random seed:

```
Window offset noise:
ch("../window_array/offset_x") + rand($random_seed + @id) * 0.1
```

### Modular System

Create variations:

- **Base graph:** Core building
- **Duplicate & modify:** Different window styles
- **Switch via parameter:** Modern vs Classic

---

## Export & Usage

### Export OBJ

1. **File → Export → OBJ**
2. Choose location
3. Options:
   - ✅ Export normals
   - ✅ Export material IDs (if created)

### Import to Blender/Unity/Unreal

- OBJ imports as static mesh
- Material IDs map to material slots
- Ready for texturing and lighting

### Game Level Generation

**Workflow:**
1. Create base building graph
2. Duplicate with variations (height, width, style)
3. Export multiple buildings
4. Scatter in game engine
5. **Result:** Unique city blocks!

---

## Learning Outcomes

After completing this tutorial, you can:

✅ Create **parametric buildings** with graph parameters
✅ Use **arrays** for window distribution
✅ Perform **boolean operations** for cutouts
✅ Build **multi-part models** with merge nodes
✅ Add **variation** through expressions
✅ **Export** for external use

---

## Next Steps

- **Try Game Assets** - [Game Assets Workflow](game-assets.md)
- **Explore Patterns** - [Procedural Patterns](patterns.md)
- **Advanced Boolean** - [Boolean Node](../nodes/boolean/boolean.md)
- **Expression Mastery** - [Expression Syntax](../expressions/expression-syntax.md)

---

## Graph Parameters Recap

Your final graph should have:

```
$building_width = 10.0    (adjusts XZ size)
$building_height = 30.0   (total height)
$floor_count = 10         (number of floors)
$floor_height = 3.0       (meters per floor)
$window_spacing = 2.5     (horizontal spacing)
$window_width = 0.8       (window size)
$window_depth = 0.2       (recess depth)
```

**Power:** Change one parameter → Entire building updates!

---

## Key Takeaways

✅ **Graph parameters** enable flexible, reusable designs
✅ **Arrays + Boolean** = Powerful for repetitive architecture
✅ **Expressions** link parameters together logically
✅ **Modular approach** = Build components, combine
✅ **Procedural = Variation** without re-modeling

**Remember:** You built a building *generator*, not just a building!
