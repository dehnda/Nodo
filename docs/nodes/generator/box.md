# Box

**Category:** Generator

## Description

Create a box primitive

## Inputs

This node generates geometry and requires no inputs.

## Parameters

### Universal

**Primitive Type** (`int`)

- Default: `0` | Options: `Polygon`, `Points`

### Size

**Width** (`float`)

Width of the box along X axis

- Default: `DEFAULT_SIZE` | Range: `0.01` to `100.0`

**Height** (`float`)

Height of the box along Y axis

- Default: `DEFAULT_SIZE` | Range: `0.01` to `100.0`

**Depth** (`float`)

Depth of the box along Z axis

- Default: `DEFAULT_SIZE` | Range: `0.01` to `100.0`

### Subdivisions

**Width Segments** (`int`)

Number of subdivisions along width (X)

- Default: `DEFAULT_SEGMENTS` | Range: `1` to `100`

**Height Segments** (`int`)

Number of subdivisions along height (Y)

- Default: `DEFAULT_SEGMENTS` | Range: `1` to `100`

**Depth Segments** (`int`)

Number of subdivisions along depth (Z)

- Default: `DEFAULT_SEGMENTS` | Range: `1` to `100`

## Example Usage

Create a Box node from the Node Library panel. Adjust parameters to customize the generated geometry.

## See Also

- [Sphere](sphere.md)
- [Cylinder](cylinder.md)
- [Grid](grid.md)
