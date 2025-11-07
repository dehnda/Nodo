# Node Reference

Complete reference for all 40 nodes in Nodo (Alpha Preview - November 2025).

## Overview

Nodes are organized into 8 categories. Each node performs a specific operation on geometry and can be connected to create procedural workflows.

**Legend:**
- ✅ Fully implemented and tested
- ⚠️ Basic implementation (limited features)
- 🚧 Placeholder (Phase 2)

---

## Generator (6 nodes)

Create primitive geometry shapes.

- **[Box](generator/box.md)** ✅ - Create a box primitive
- **[Cylinder](generator/cylinder.md)** ✅ - Create a cylinder primitive
- **[Grid](generator/grid.md)** ✅ - Create a planar grid of polygons
- **[Line](generator/line.md)** ✅ - Create a line or curve
- **[Sphere](generator/sphere.md)** ✅ - Create a UV sphere or icosphere
- **[Torus](generator/torus.md)** ✅ - Create a torus primitive

## Modifier (5 nodes)

Transform and modify geometry.

- **[Extrude](modifier/extrude.md)** ✅ - Extrude geometry along normals
- **[Subdivide](modifier/subdivide.md)** ✅ - Subdivide polygons for smoother geometry
- **[Smooth (Laplacian)](modifier/smooth_(laplacian).md)** ✅ - Smooth geometry using Laplacian method
- **[Noise Displacement](modifier/noise_displacement.md)** ✅ - Displace geometry using noise
- **[Bevel](modifier/bevel.md)** ⚠️ - Create beveled edges (basic, 1 segment only)

## Transform (6 nodes)

Position, duplicate, and arrange geometry.

- **[Transform](modifier/transform.md)** ✅ - Translate, rotate, scale geometry
- **[Array](array/array.md)** ✅ - Create linear or grid arrays
- **[Copy to Points](array/copy_to_points.md)** ✅ - Copy geometry to point positions
- **[Mirror](modifier/mirror.md)** ✅ - Mirror geometry across a plane
- **[Scatter](array/scatter.md)** ✅ - Scatter points across geometry surface
- **[Scatter Volume](array/scatter_volume.md)** ✅ - Scatter points within bounding volume
- **[Align](modifier/align.md)** ✅ - Align geometry bounding box to axes or origin

## Boolean & Combine (5 nodes)

Merge and combine meshes.

- **[Boolean](boolean/boolean.md)** ✅ - Union, subtract, intersect operations
- **[Merge](boolean/merge.md)** ✅ - Merge multiple geometries into one
- **[Split](modifier/split.md)** ✅ - Separate geometry by connectivity or attribute
- **[PolyExtrude](modifier/polyextrude.md)** ✅ - Extrude individual polygons
- **[Remesh](modifier/remesh.md)** 🚧 - Uniform mesh triangulation (stub only)

## Attribute (6 nodes)

Manage geometry data and attributes.

- **[Wrangle](attribute/wrangle.md)** ✅ - VEX-like scripting for attributes
- **[Attribute Create](attribute/attribute_create.md)** ✅ - Create or modify attributes
- **[Attribute Delete](attribute/attribute_delete.md)** ✅ - Delete attributes from geometry
- **[Color](attribute/color.md)** ✅ - Set vertex colors
- **[Normal](modifier/normal.md)** ✅ - Compute or modify vertex/face normals
- **[UV Unwrap](attribute/uv_unwrap.md)** ✅ - Generate UV coordinates (xatlas)

## Group (7 nodes)

Select and organize geometry components.

- **[Group](group/group.md)** ✅ - Create geometry groups (bounds, normal, random)
- **[Blast](utility/blast.md)** ✅ - Delete geometry by group
- **[Sort](utility/sort.md)** ✅ - Sort points or primitives
- **[Group Promote](group/group_promote.md)** ✅ - Convert groups between component types
- **[Group Combine](group/group_combine.md)** ✅ - Boolean operations on groups
- **[Group Expand](group/group_expand.md)** ✅ - Grow/shrink group selection
- **[Group Transfer](group/group_transfer.md)** ✅ - Transfer groups between geometries

## Deformers (3 nodes)

Non-destructive deformations.

- **[Bend](modifier/bend.md)** ✅ - Bend geometry along an axis
- **[Twist](modifier/twist.md)** ✅ - Twist geometry around an axis
- **[Lattice](modifier/lattice.md)** ✅ - Deform geometry with a lattice cage

## Utility (5 nodes)

Workflow helpers and I/O.

- **[Switch](utility/switch.md)** ✅ - Choose between multiple inputs
- **[Null](utility/null.md)** ✅ - Pass-through node for organization
- **[Output](utility/output.md)** ✅ - Mark geometry as final output
- **[File](io/file.md)** ✅ - Import geometry from OBJ/STL files
- **[Export](io/export.md)** ✅ - Export geometry to OBJ format

---

## Future Nodes (Phase 2+)

The following nodes are planned for future releases:

- **Cache** - Freeze expensive operations
- **Time** - Animation timeline control
- **Resample** - Resample curves with uniform spacing
- **Full Bevel** - Multi-segment beveling with profile control
- **Full Remesh** - Intelligent remeshing algorithms
