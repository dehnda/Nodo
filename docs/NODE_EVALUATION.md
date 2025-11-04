# Node Evaluation & Action Plan
**Date:** November 1, 2025
**Purpose:** Practical evaluation of what each node does, whether we need it, and what to do about it

---

## Evaluation Criteria

**Keep:** Essential for procedural modeling workflows
**Defer:** Useful but not critical for Phase 1
**Merge:** Duplicate functionality, combine with another node
**Remove:** Not needed or too complex for current scope

---

## 1. GEOMETRY GENERATORS (6 nodes)

### 1.1 Sphere ✅ KEEP
**What it does:** Generates UV sphere with radius and segments/rings
**Why keep:** Essential primitive. Used in 90% of procedural scenes.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 1.2 Box ✅ KEEP
**What it does:** Generates cube/box with width/height/depth and subdivisions
**Why keep:** Most common primitive. Foundation for hard surface modeling.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 1.3 Cylinder ✅ KEEP
**What it does:** Generates cylinder with radius, height, segments, and cap options
**Why keep:** Essential for columns, pipes, mechanical parts.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 1.4 Torus ✅ KEEP
**What it does:** Generates donut shape with major/minor radius
**Why keep:** Common primitive for rings, wheels, decorative elements.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 1.5 Grid ✅ KEEP
**What it does:** Generates planar grid (XZ plane) with size and subdivisions
**Why keep:** Essential for terrain, floors, ground planes.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 1.6 Line ⚠️ KEEP
**What it does:** Generates line between two points with segments
**Why keep:** Needed for curves, paths, guide geometry.
**Status:** 🔧 Header-only stub, no parameters
**Action:** **HIGH PRIORITY** - Implement with parameters:
- `origin` - Vector3 (start point)
- `direction` - Vector3 or `end` point
- `length` - Float
- `segments` - Int (points along line)

**Estimated time:** 2-3 hours

---

## 2. GEOMETRY MODIFIERS (6 nodes → 4 nodes)

### 2.1 Extrude ✅ KEEP
**What it does:** Extrude faces along normals or direction
**Why keep:** Core modeling operation. Essential for creating depth.
**Status:** ⚠️ Has implementation but parameters are private members
**Action:** **CRITICAL** - Convert to registered parameters:
```cpp
// Change from private members to registered parameters
register_parameter(define_int_parameter("mode", 0)
    .options({"Face Normals", "Uniform Direction", "Region Normals"}));
register_parameter(define_float_parameter("distance", 1.0F));
register_parameter(define_vector3_parameter("direction", {0, 0, 1}));
register_parameter(define_float_parameter("inset", 0.0F));
```

**Estimated time:** 1-2 hours

---

### 2.2 Subdivide ✅ KEEP
**What it does:** Catmull-Clark subdivision for smoother surfaces
**Why keep:** Standard for organic modeling. Creates smooth meshes.
**Status:** ⚠️ Has setters but unclear if parameters registered
**Action:** **MEDIUM** - Verify parameter registration:
- `subdivision_levels` - Int (0-5)
- `preserve_boundaries` - Bool

**Estimated time:** 1 hour

---

### 2.3 Smooth (Laplacian) ✅ KEEP
**What it does:** Laplacian smoothing to reduce mesh noise
**Why keep:** Different from subdivision - removes bumps without adding geometry.
**Status:** ⚠️ Needs verification
**Action:** **MEDIUM** - Verify all parameters registered:
- `method` - Dropdown (Uniform, Cotangent, Taubin)
- `iterations` - Int
- `lambda` - Float (strength)
- `mu` - Float (for Taubin anti-shrinkage)

**Estimated time:** 1 hour

---

### 2.4 Noise Displacement ✅ KEEP
**What it does:** Displaces vertices using fractal noise
**Why keep:** Essential for organic surfaces, terrain, bark, rocks.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 2.5 Bevel ❌ DEFER TO PHASE 2
**What it does:** Adds chamfered edges to hard surface models
**Why defer:** Complex implementation, requires edge detection and topological changes. Nice-to-have but not essential for basic procedural modeling.
**Status:** ❌ Not implemented
**Action:** Remove from Phase 1 scope. Add to Phase 2 backlog.

---

### 2.6 Displace ➡️ MERGE WITH NOISE
**What it does:** Would displace by attribute values
**Why merge:** Noise Displacement already handles this use case. Attribute-based displacement can be added as a mode.
**Status:** ❌ Not implemented
**Action:** **REMOVE** - Covered by Noise Displacement node. If attribute-based displacement needed, add as mode to Noise node.

---

## 3. TRANSFORM OPERATIONS (6 nodes → 5 nodes)

### 3.1 Transform ✅ KEEP
**What it does:** Translate, rotate, scale geometry
**Why keep:** Most fundamental operation. Needed constantly.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 3.2 Array ✅ KEEP
**What it does:** Duplicate geometry in patterns (Linear, Radial, Grid)
**Why keep:** Essential for procedural duplication. Pillars, fences, crowds.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 3.3 Copy to Points ✅ KEEP
**What it does:** Instance geometry onto point cloud
**Why keep:** Core instancing operation. Scatter with control.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 3.4 Mirror ✅ KEEP
**What it does:** Mirror geometry across XY, XZ, or YZ plane
**Why keep:** Symmetrical modeling is essential. Characters, architecture.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 3.5 Scatter ✅ KEEP
**What it does:** Randomly scatter points on surface
**Why keep:** Essential for vegetation, debris, particle systems.
**Status:** ✅ Complete
- Fixed face area weighting (use_face_area now works)
- Metadata attributes (id, source_face)
- Density multiplier support
**Action:** None - ready to test

**Note:** This is SURFACE scatter only. Volume scatter is a separate node:
- **ScatterVolumeSOP** (M1.6) - Scatter within bounding box/sphere
- Keep nodes separate for clarity and single responsibility

---

### 3.6 Align ❌ DEFER TO PHASE 2
**What it does:** Align geometry to world axes or other geometry
**Why defer:** Can be achieved with Transform node. Nice convenience but not essential.
**Status:** ❌ Not implemented
**Action:** Remove from Phase 1. Can add later if users request it.

---

## 4. BOOLEAN & COMBINE (6 nodes → 2 nodes)

### 4.1 Boolean ✅ KEEP
**What it does:** Union, Intersection, Difference operations
**Why keep:** Critical for hard surface modeling. Manifold makes this robust.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 4.2 Merge ✅ KEEP
**What it does:** Combines multiple geometries into one
**Why keep:** Essential for combining branches of node graph.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 4.3 Join ➡️ SAME AS MERGE
**What it does:** Would join geometries together
**Why remove:** This is literally what Merge does.
**Status:** ❌ Not implemented
**Action:** **REMOVE** - Duplicate of Merge node.

---

### 4.4 Split ❌ DEFER TO PHASE 2
**What it does:** Split geometry into separate pieces by connectivity
**Why defer:** Advanced feature. Can be achieved with Group + Blast workflow.
**Status:** ❌ Not implemented
**Action:** Remove from Phase 1. Useful but not critical.

---

### 4.5 Remesh ❌ DEFER TO PHASE 2
**What it does:** Remesh to uniform triangulation
**Why defer:** Complex algorithm. More of a mesh repair/optimization tool. Not essential for procedural modeling workflows.
**Status:** ❌ Not implemented
**Action:** Remove from Phase 1. Consider for Phase 3 polish.

---

### 4.6 Separate ➡️ COVERED BY BLAST
**What it does:** Separate geometry by attributes or groups
**Why remove:** Blast node already deletes groups, achieving separation.
**Status:** ❌ Not implemented
**Action:** **REMOVE** - Use Blast node to delete unwanted groups.

---

## 5. ATTRIBUTE OPERATIONS (6 nodes → 5 nodes)

### 5.1 Wrangle ✅ KEEP
**What it does:** Run expressions on attributes (exprtk)
**Why keep:** Power-user tool. Enables custom math operations.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 5.2 Attribute Create ✅ KEEP
**What it does:** Create new attributes with default values
**Why keep:** Foundation for attribute workflows. Initialize data.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 5.3 Attribute Delete ✅ KEEP
**What it does:** Delete attributes by name or pattern
**Why keep:** Clean up unwanted attributes. Memory optimization.
**Status:** ⚠️ Needs verification
**Action:** **MEDIUM** - Check if implemented with parameters:
- `name` - String (attribute name)
- `class` - Dropdown (Point/Primitive/Vertex/Detail)
- `pattern` - Bool (wildcard matching)

**Estimated time:** 2-3 hours if needs implementation

---

### 5.4 Color ⚠️ KEEP (but simplified)
**What it does:** Set color attribute on geometry
**Why keep:** Needed for visualization, vertex colors.
**Status:** ⚠️ Needs verification
**Action:** **MEDIUM** - Implement simple version:
- `class` - Dropdown
- `color_mode` - Constant or Random
- `color` - RGB (for Constant)
- `seed` - Int (for Random)

**Estimated time:** 3-4 hours if needs implementation

---

### 5.5 Normal ✅ KEEP
**What it does:** Calculate vertex or face normals
**Why keep:** Essential for shading. Most geometries need normals.
**Status:** ✅ Complete (has .cpp)
**Action:** Verify parameters registered

---

### 5.6 UV Unwrap ✅ KEEP
**What it does:** Automatic UV unwrapping using xatlas
**Why keep:** Essential for texturing. Xatlas is industry-standard.
**Status:** ✅ Complete (has .cpp with xatlas)
**Action:** None - ready to test

---

## 6. GROUP OPERATIONS (6 nodes → 3 nodes)

### 6.1 Group (Create) ✅ KEEP
**What it does:** Create groups using expressions or selection
**Why keep:** Foundation of group workflow. Essential for selective operations.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 6.2 Blast (Group Delete) ✅ KEEP - RENAME
**What it does:** Delete geometry based on groups
**Why keep:** Essential for filtering. Delete unwanted parts.
**Status:** ✅ Complete
**Action:** **OPTIONAL** - Consider renaming "Blast" to "Delete" for clarity. "Blast" is Houdini jargon.

---

### 6.3 Sort ✅ KEEP
**What it does:** Sort points/primitives by attribute or position
**Why keep:** Needed for attribute transfer, numbered selection, random shuffling.
**Status:** ⚠️ Needs verification
**Action:** **LOW** - Verify exists and has parameters

---

### 6.4 Group Delete (the node) ➡️ REDUNDANT
**What it does:** Delete a group definition (not the geometry)
**Why remove:** This is removing group metadata, not geometry. Very niche operation.
**Status:** ⚠️ Unknown
**Action:** **REMOVE** - Not essential. Groups can be ignored if not used.

---

### 6.5 Group Promote ❌ DEFER TO PHASE 2
**What it does:** Convert point groups to primitive groups or vice versa
**Why defer:** Advanced feature. Rarely needed in basic workflows.
**Status:** ⚠️ Unknown
**Action:** Remove from Phase 1. Add to Phase 2 if requested.

---

### 6.6 Group Combine ❌ DEFER TO PHASE 2
**What it does:** Combine multiple groups with boolean logic
**Why defer:** Can be achieved by creating new group with expression like `@group_a || @group_b`
**Status:** ⚠️ Unknown
**Action:** Remove from Phase 1. Group expressions cover this.

---

### 6.7 Group Expand ❌ DEFER TO PHASE 2
**What it does:** Grow/shrink groups by topology iterations
**Why defer:** Nice feature but not essential. Advanced selection tool.
**Status:** ⚠️ Unknown
**Action:** Remove from Phase 1. Add to Phase 2 polish.

---

### 6.8 Group Transfer ❌ DEFER TO PHASE 2
**What it does:** Transfer groups between different geometries
**Why defer:** Advanced feature for specific workflows.
**Status:** ⚠️ Unknown
**Action:** Remove from Phase 1. Phase 2 if needed.

---

## 7. DEFORMATION NODES (2 nodes)

### 7.1 Bend ✅ KEEP
**What it does:** Circular bend deformation around axis
**Why keep:** Classic deformer. Useful for organic and mechanical modeling.
**Status:** ✅ Complete (has parameters)
**Action:** None - ready to test

---

### 7.2 Twist ✅ KEEP
**What it does:** Rotational twist along axis
**Why keep:** Classic deformer. Complement to Bend.
**Status:** ✅ Complete (has parameters)
**Action:** None - ready to test

---

### 7.3 Lattice ⚠️ KEEP (but simplified)
**What it does:** FFD (Free-Form Deformation) using 3D lattice
**Why keep:** Powerful deformation tool for organic modeling.
**Status:** ⚠️ Partially implemented (has parameters but needs second input for deformed lattice)
**Action:** **LOW PRIORITY** - Either:
1. Simplify to just show lattice visualization (defer actual deformation)
2. Or defer entire node to Phase 2

**Estimated time:** 6-8 hours for full implementation

---

## 8. UTILITY & WORKFLOW (8 nodes → 6 nodes)

### 8.1 Switch ✅ KEEP
**What it does:** Select one of multiple inputs by index
**Why keep:** Essential for A/B testing, variant selection.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 8.2 Null ✅ KEEP
**What it does:** Pass-through node for organization
**Why keep:** Essential for clean node networks. Named endpoints.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 8.3 Output ✅ KEEP
**What it does:** Mark geometry as named output for rendering/export
**Why keep:** Semantic marker for what should be exported/rendered.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 8.4 File (Import) ✅ KEEP
**What it does:** Import .obj files
**Why keep:** Essential for loading external geometry.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 8.5 Export ✅ KEEP
**What it does:** Export to .obj files
**Why keep:** Essential for getting geometry out of Nodo.
**Status:** ✅ Complete
**Action:** None - ready to test

---

### 8.6 Cache ❌ DEFER TO PHASE 2
**What it does:** Cache node results to disk for performance
**Why defer:** Optimization feature. Not needed for basic functionality.
**Status:** ❌ Header-only
**Action:** Remove from Phase 1. Add to Phase 2 performance work.

---

### 8.7 Time ❌ DEFER TO PHASE 2
**What it does:** Access time/frame for animation
**Why defer:** Animation is Phase 2 feature. No time system yet.
**Status:** ❌ Header-only
**Action:** Remove from Phase 1. Add when animation system is built.

---

### 8.8 Subnetwork ❌ DEFER TO PHASE 3
**What it does:** Container for node subgraphs (like a function)
**Why defer:** Complex feature. Requires subgraph execution, parameter promotion, I/O binding. This is a power-user feature.
**Status:** ❌ Not implemented
**Action:** Remove from Phase 1 & 2. Phase 3 or later.

---

## SUMMARY: REVISED NODE LIST

### Original: 44 nodes → **Revised: 30 nodes** for Phase 1

### KEEP (25 complete + 5 need work = 30 total):

**Generators (6):**
1. Sphere ✅
2. Box ✅
3. Cylinder ✅
4. Torus ✅
5. Grid ✅
6. Line 🔧

**Modifiers (4):**
7. Extrude ⚠️
8. Subdivide ⚠️
9. Smooth (Laplacian) ⚠️
10. Noise Displacement ✅

**Transforms (5):**
11. Transform ✅
12. Array ✅
13. Copy to Points ✅
14. Mirror ✅
15. Scatter ✅

**Boolean/Combine (2):**
16. Boolean ✅
17. Merge ✅

**Attributes (5):**
18. Wrangle ✅
19. Attribute Create ✅
20. Attribute Delete ⚠️
21. Color ⚠️
22. Normal ✅
23. UV Unwrap ✅

**Groups (3):**
24. Group ✅
25. Blast ✅
26. Sort ⚠️

**Deformers (3):**
27. Bend ✅
28. Twist ✅
29. Lattice ⚠️ (optional)

**Utility (6):**
30. Switch ✅
31. Null ✅
32. Output ✅
33. File ✅
34. Export ✅
35. (Polyextrude ✅ - was this meant to replace regular Extrude?)

### REMOVE from Phase 1 (14 nodes):
- Bevel (defer Phase 2)
- Displace (merge with Noise)
- Align (defer Phase 2)
- Join (duplicate of Merge)
- Split (defer Phase 2)
- Remesh (defer Phase 2)
- Separate (use Blast instead)
- Group Delete (not needed)
- Group Promote (defer Phase 2)
- Group Combine (defer Phase 2)
- Group Expand (defer Phase 2)
- Group Transfer (defer Phase 2)
- Cache (defer Phase 2)
- Time (defer Phase 2)
- Subnetwork (defer Phase 3)

---

## ACTION PLAN FOR M1.4

### Phase 1: Fix Critical Blockers (6-8 hours)

1. **Line SOP** (2-3h)
   - Implement backend with parameters
   - Origin, direction/end, length, segments

2. **Extrude SOP** (1-2h)
   - Convert private members to registered parameters
   - Should be straightforward refactor

3. **Subdivision SOP** (1h)
   - Verify parameter registration
   - Test in UI

4. **Smooth/Laplacian SOP** (1h)
   - Verify all parameters registered
   - Test methods dropdown

### Phase 2: Complete Missing Nodes (6-10 hours)

5. **Attribute Delete SOP** (2-3h)
   - Implement if missing
   - Name, class, pattern parameters

6. **Color SOP** (3-4h)
   - Implement simple version
   - Constant and Random modes

7. **Sort SOP** (1-2h)
   - Verify exists
   - Sort by position or attribute

8. **Lattice SOP** (optional, 0-8h)
   - Either simplify or defer to Phase 2

### Phase 3: Systematic Testing (12-15 hours)

9. **Test all 30 nodes** following M1.4 checklist
   - Open property panel
   - Test each parameter
   - Verify visibility conditions
   - Test graph cooking
   - Document any issues

### Phase 4: Polish & Fixes (4-6 hours)

10. **Fix issues** found during testing
11. **Update documentation**
12. **Create example scenes** for each node

---

## TOTAL EFFORT ESTIMATE

- **Minimum (skip Lattice):** 28-39 hours
- **Maximum (include Lattice):** 36-47 hours
- **Realistic with buffer:** ~40 hours / 1 week full-time

---

## DELIVERABLE

At the end, you'll have:
- ✅ **30 production-ready nodes**
- ✅ All with complete property panels
- ✅ Auto-generation working
- ✅ Clean, focused toolset
- ✅ Clear backlog for Phase 2

This is a **realistic, shippable product** rather than a half-finished 44-node mess.

---

## RECOMMENDATION

**Ship 30 nodes in Phase 1.**

The 14 removed nodes fall into three categories:
1. **Duplicates** (Join, Separate, Displace) - already covered
2. **Advanced features** (Bevel, Remesh, Group operations) - nice but not essential
3. **Future systems** (Cache, Time, Subnetwork) - need infrastructure

Users won't miss what they never had. A polished 30-node toolset beats a buggy 44-node one.

You can always add the Phase 2 nodes based on user feedback. That's **user-driven development** rather than feature bloat.

---

## NEXT STEP

**Decision point:** Do you want to proceed with 30-node focused approach, or do you want to keep pushing for all 44?

I recommend 30 nodes. Quality over quantity.
