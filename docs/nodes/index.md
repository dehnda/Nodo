# Node Reference

Complete reference for all available nodes in Nodo.

## Generator

- **[Box](generator/box.md)** - Create a box primitive
- **[Cylinder](generator/cylinder.md)** - Create a cylinder primitive
- **[Grid](generator/grid.md)** - Create a planar grid of polygons
- **[Line](generator/line.md)** - Create a line or curve
- **[Sphere](generator/sphere.md)** - Create a UV sphere primitive
- **[Torus](generator/torus.md)** - Create a torus primitive

## Modifier

- **[Align](modifier/align.md)** - Align geometry bounding box to axes or origin
- **[Bend](modifier/bend.md)** - Bend geometry along an axis
- **[Bevel](modifier/bevel.md)** - Create beveled edges and corners (Phase 2 placeholder)
- **[Extrude](modifier/extrude.md)** - Extrude geometry along normals
- **[Lattice](modifier/lattice.md)** - Deform geometry with a lattice cage
- **[Mirror](modifier/mirror.md)** - Mirror geometry across a plane
- **[Noise Displacement](modifier/noise_displacement.md)** - Displace geometry using noise
- **[Normal](modifier/normal.md)** - Compute or modify vertex/face normals
- **[PolyExtrude](modifier/polyextrude.md)** - Extrude individual polygons
- **[Remesh](modifier/remesh.md)** - Uniform mesh triangulation (Phase 2 placeholder)
- **[Resample](modifier/resample.md)** - Resample curves with uniform spacing
- **[Smooth (Laplacian)](modifier/smooth_(laplacian).md)** - Smooth geometry using Laplacian method
- **[Split](modifier/split.md)** - Separate geometry by connectivity or attribute
- **[Subdivide](modifier/subdivide.md)** - Subdivide polygons for smoother geometry
- **[Transform](modifier/transform.md)** - Transform geometry with translate, rotate, scale
- **[Twist](modifier/twist.md)** - Twist geometry around an axis

## Array

- **[Array](array/array.md)** - Create linear or radial arrays of geometry
- **[Copy to Points](array/copy_to_points.md)** - Copy geometry to point positions
- **[Scatter](array/scatter.md)** - Scatter points across geometry surface
- **[Scatter Volume](array/scatter_volume.md)** - Scatter points within input geometry's bounding box

## Boolean

- **[Boolean](boolean/boolean.md)** - Perform boolean operations (union, subtract, intersect)
- **[Merge](boolean/merge.md)** - Merge multiple geometries into one

## Attribute

- **[Attribute Create](attribute/attribute_create.md)** - Create or modify attributes
- **[Attribute Delete](attribute/attribute_delete.md)** - Delete attributes from geometry
- **[Color](attribute/color.md)** - Set vertex colors
- **[UV Unwrap](attribute/uv_unwrap.md)** - Generate UV coordinates
- **[Wrangle](attribute/wrangle.md)** - VEX-like scripting for attributes

## Group

- **[Group](group/group.md)** - Create geometry groups
- **[Group Combine](group/group_combine.md)** - Combine multiple groups
- **[Group Delete](group/group_delete.md)** - Delete geometry groups
- **[Group Expand](group/group_expand.md)** - Expand group selection by connectivity
- **[Group Promote](group/group_promote.md)** - Convert groups between component types
- **[Group Transfer](group/group_transfer.md)** - Transfer groups from one geometry to another

## IO

- **[Export](io/export.md)** - Export geometry to file
- **[File](io/file.md)** - Import geometry from file

## Utility

- **[Blast](utility/blast.md)** - Delete geometry by group or selection
- **[Cache](utility/cache.md)** - Cache geometry to avoid recompute
- **[Null](utility/null.md)** - Pass-through node for organization
- **[Output](utility/output.md)** - Mark geometry as final output
- **[Sort](utility/sort.md)** - Sort points or primitives
- **[Switch](utility/switch.md)** - Choose between multiple inputs
- **[Time](utility/time.md)** - Control time-dependent animations
