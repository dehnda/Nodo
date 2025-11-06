# Geometry Types

Understanding how 3D geometry is represented in Nodo helps you make better procedural models and use nodes effectively.

## Mesh Structure

Nodo works with **triangle meshes** - the most common 3D geometry representation.

### Basic Components

```
Vertex (Point)
    ↓
Edge (Line between 2 vertices)
    ↓
Face (Triangle from 3 vertices)
    ↓
Mesh (Collection of connected faces)
```

---

## Vertices (Points)

### What are Vertices?

**Vertices** (singular: vertex) are **points in 3D space** with (x, y, z) coordinates.

```
Vertex = (x, y, z)

Example:
v1 = (0.0, 0.0, 0.0)   # Origin
v2 = (1.0, 0.0, 0.0)   # 1 unit on X axis
v3 = (0.0, 1.0, 0.0)   # 1 unit on Y axis
```

### Vertex Attributes

Vertices can store additional data:

- **Position** (required): x, y, z coordinates
- **Normal**: Direction for smooth shading
- **Color**: RGB or RGBA values
- **UV**: Texture coordinates
- **Custom**: Any data you define

**Access in expressions:**
```
@P       # Position vector (x, y, z)
@P.x     # X coordinate
@P.y     # Y coordinate
@P.z     # Z coordinate
@N       # Normal vector
@Cd      # Color
```

---

## Edges

### What are Edges?

**Edges** connect two vertices, forming the "skeleton" of your mesh.

```
Edge = (vertex_index_1, vertex_index_2)

Example:
e1 = (0, 1)   # Edge from vertex 0 to vertex 1
e2 = (1, 2)   # Edge from vertex 1 to vertex 2
```

### Edge Uses

- **Wireframe rendering** - Show mesh structure
- **Smoothing** - Define hard/soft edges
- **Selections** - Select edge loops for operations
- **Beveling** - Round off corners

---

## Faces (Triangles)

### What are Faces?

**Faces** are flat surfaces defined by 3 or more vertices. In Nodo (via Manifold), all faces are **triangles**.

```
Triangle Face = (v1, v2, v3)

Example:
f1 = (0, 1, 2)   # Triangle using vertices 0, 1, 2
```

### Triangle vs Quad

**Traditional modeling:** Often uses quads (4-vertex faces)

**Nodo approach:** Everything is triangles internally
- Simpler math
- Guaranteed planar
- Better for boolean operations
- Game/render engines use triangles anyway

**Note:** Some display modes may show quads visually, but they're pairs of triangles.

### Face Normals

Each face has a **normal** - a vector pointing "outward" from the surface:

```
     ↑ Normal
     |
   ╱─┴─╲
  ╱     ╲    Triangle face
 ╱       ╲
╱─────────╲
```

**Determines:**
- Which side is "front" (visible)
- Lighting calculations
- Backface culling

**Right-hand rule:** Vertices in counter-clockwise order → normal points toward you

---

## Mesh Types

### 1. Manifold Mesh

**Definition:** A "watertight" mesh with no holes, proper edges, valid topology.

**Properties:**
- Every edge connects exactly 2 faces
- No isolated vertices
- No duplicate faces
- Defines a closed volume

**Why it matters:**
- Required for boolean operations
- Defines inside/outside
- Can calculate volume

**Nodo uses Manifold library** for robust boolean operations - prefers manifold meshes.

### 2. Non-Manifold Mesh

**Definition:** Mesh with topological issues.

**Common problems:**
- Holes or gaps
- Edges with 1 or 3+ faces
- Isolated vertices or faces
- Overlapping geometry

**Results:**
- Booleans may fail
- Normals may flip
- Rendering artifacts

**Fix:**
- Check geometry carefully
- Use Merge node (can weld vertices)
- Ensure generators create clean topology

### 3. Boundary Mesh

**Definition:** Mesh with open edges (not closed).

**Example:**
```
Plane - has boundary (4 open edges)
Sphere - no boundary (fully closed)
```

**Use:**
- Planes, grids (intentionally open)
- Partial geometry
- Not ideal for booleans

---

## Geometry Attributes

Attributes store data on vertices, faces, or edges.

### Point (Vertex) Attributes

Stored per-vertex:

```
P        Position (x, y, z)
N        Normal vector
Cd       Color (r, g, b)
uv       UV coordinates
id       Vertex ID
[custom] Any data you create
```

**Access:**
```
@P.y > 0.5    # Select vertices above Y=0.5
@Cd = [1,0,0] # Set color to red
```

### Face Attributes

Stored per-triangle:

```
materialid   Material assignment
N            Face normal
[custom]     Any face data
```

**Use:**
- Material IDs for multi-material objects
- Face selections
- Data-driven face operations

### Primitive Attributes

Stored per-primitive (mesh/object):

```
name         Object name
layer        Layer assignment
[custom]     Object-level data
```

---

## Coordinate Systems

### Local vs World Space

**Local (Object) Space:**
- Origin at object's pivot
- Coordinates relative to object
- What you see in node parameters

**World Space:**
- Global coordinate system
- Absolute positions in scene
- Used for final placement

**Example:**
```
Box at (0,0,0) local, but object moved to (5,5,5) world
Vertex (1,0,0) local = (6,5,5) world
```

### Transform Affects Space

Transform node changes object's world position:

```
Sphere (local 0,0,0)
  ↓ Transform: Translate (5,0,0)
Sphere (world 5,0,0)
```

But vertex local coords unchanged!

---

## Topology vs Geometry

### Geometry

**What it is:** The actual shape - vertex positions, face shapes

**Changes:**
- Move vertices
- Scale object
- Deform mesh

**Example:**
```
Before: Cube 1x1x1
After:  Cube 2x2x2 (same topology, different geometry)
```

### Topology

**What it is:** The connectivity - how vertices connect to form faces

**Changes:**
- Add/remove faces
- Subdivide
- Boolean operations
- Merge vertices

**Example:**
```
Before: Cube with 8 vertices, 12 faces
After:  Subdivided cube with 26 vertices, 48 faces (different topology)
```

**Key Point:** Some operations change topology, some only geometry.

---

## Common Mesh Operations

### Adding Geometry

**Nodes that create:**
- Sphere, Box, Cylinder → Generate new meshes
- Grid, Line → Create planar/linear geometry

### Modifying Geometry

**Nodes that change shape (topology unchanged):**
- Transform - Move/rotate/scale
- Bend, Twist - Deform
- Smooth (some modes) - Relax positions

### Modifying Topology

**Nodes that change connectivity:**
- Subdivide - Add faces
- Extrude - Extend faces
- Boolean - Cut/combine meshes
- Group Delete - Remove faces

### Combining Geometry

**Nodes that merge:**
- Merge - Combine meshes (optional welding)
- Boolean Union - Merge with CSG
- Array - Create copies

---

## Normals

### What are Normals?

**Normals** are vectors indicating surface direction.

```
     ↑ Vertex Normal
     |
   ╱─┴─╲
  ╱     ╲
 ╱   →   ╲  Face Normal
╱─────────╲
```

### Face Normals

- Perpendicular to triangle surface
- Automatic from vertex order
- Used for backface culling

### Vertex Normals

- Averaged from adjacent face normals
- Used for smooth shading
- Can be manually set for hard edges

### Smooth vs Flat Shading

**Flat Shading:**
- Uses face normals only
- Faceted appearance
- Good for hard-surface models

**Smooth Shading:**
- Interpolates vertex normals
- Rounded appearance
- Good for organic shapes

**In Nodo:**
- Smooth node can recalculate normals
- Display settings control shading mode

---

## UV Coordinates

### What are UVs?

**UV coordinates** map 3D surface to 2D texture space.

```
3D Mesh        2D Texture
   ↓               ↓
 Vertex        UV Coord
(x,y,z)   →    (u,v)
```

**Range:** Usually 0.0 to 1.0 in U and V

**Use:**
- Apply images as textures
- Material mapping
- Procedural patterns

**In Nodo:**
- Some generators create UVs automatically
- UV unwrapping (advanced feature, future)

---

## Inside vs Outside

### Winding Order

Vertex order determines face direction:

```
Counter-Clockwise (CCW) = Front face
Clockwise (CW) = Back face
```

**Right-hand rule:**
- Curl fingers in vertex order
- Thumb points in normal direction

### Normals Point Outward

Properly oriented mesh:
- All normals point away from solid
- Defines "inside" vs "outside"
- Essential for booleans

**Flip normals:** Reverses inside/outside

---

## Manifold Requirements

For robust boolean operations, meshes should be:

✅ **Closed** - No holes or open edges
✅ **Non-self-intersecting** - No overlapping faces
✅ **Proper winding** - Consistent face orientation
✅ **Two-manifold** - Each edge has exactly 2 faces
✅ **No duplicate vertices** - Welded properly

**Check:**
- Sphere ✅ (closed ball)
- Box ✅ (closed cube)
- Plane ❌ (open, has boundary)
- Overlapping cubes ❌ (self-intersecting)

---

## Performance Considerations

### Vertex Count

More vertices = more computation:

```
Sphere Segments: 8  → 96 vertices  (fast)
Sphere Segments: 64 → 6144 vertices (slower)
```

**Balance:**
- Use low poly for iteration
- Increase detail for final output

### Triangle Count

Directly affects:
- Boolean operation speed
- Rendering performance
- File size

**Optimize:**
- Only subdivide where needed
- Use LOD (Level of Detail) for distance

### Attribute Overhead

Each attribute adds memory:

```
1000 vertices × 12 attributes = 12,000 data points
```

**Best practice:**
- Delete unused attributes
- Only create what you need

---

## Best Practices

### 1. Keep Meshes Clean

- Remove duplicate vertices
- Ensure proper normals
- Avoid self-intersections

### 2. Use Appropriate Resolution

```
✅ Low poly for base shape → Subdivide for detail
❌ Start with ultra-high poly
```

### 3. Understand Node Impact

- Generators: Create topology
- Modifiers: Often preserve topology
- Booleans: Create new topology

### 4. Check Manifold Status

- Test booleans on simple geometry first
- If boolean fails, check input meshes

---

## Troubleshooting

### Mesh Looks Inside-Out

**Cause:** Normals flipped

**Fix:**
- Check vertex winding order
- Use Reverse node (if available)
- Recreate with correct orientation

### Boolean Operation Fails

**Cause:** Non-manifold inputs

**Fix:**
- Verify both meshes are closed
- Check for self-intersections
- Simplify geometry if too complex

### Weird Shading

**Cause:** Inconsistent normals

**Fix:**
- Recalculate normals (Smooth node)
- Check for duplicate vertices
- Ensure consistent winding

### Gaps in Mesh

**Cause:** Non-welded vertices

**Fix:**
- Use Merge node with weld enabled
- Check tolerance settings

---

## Next Steps

- **Learn Attributes** - [Attributes](attributes.md)
- **Understand Groups** - [Groups](groups.md)
- **See Examples** - [Quick Start](../getting-started/quick-start.md)
- **Try Workflows** - [Architectural Modeling](../workflows/architectural.md)

---

## Key Takeaways

✅ **Meshes** = Vertices + Edges + Faces (triangles)
✅ **Manifold** = Closed, watertight, valid for booleans
✅ **Attributes** = Data stored on vertices/faces
✅ **Normals** = Direction vectors for shading
✅ **Topology** = Connectivity (how it's built)
✅ **Geometry** = Shape (where vertices are)

**Remember:** Clean topology → Successful procedural operations!
