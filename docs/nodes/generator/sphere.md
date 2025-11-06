# Sphere

**Category:** Generator

## Description

Create a UV sphere primitive

## Inputs

This node generates geometry and requires no inputs.

## Parameters

### Universal

**Primitive Type** (`int`)

- Default: `0` | Options: `Polygon`, `Points`

### Size

**Radius** (`float`)

Radius of the sphere

- Default: `DEFAULT_RADIUS` | Range: `0.01` to `100.0`

### Resolution

**Segments** (`int`)

Number of vertical segments (longitude)

- Default: `DEFAULT_SEGMENTS` | Range: `3` to `256`

**Rings** (`int`)

Number of horizontal rings (latitude)

- Default: `DEFAULT_RINGS` | Range: `3` to `128`

## Example Usage

Create a Sphere node from the Node Library panel. Adjust parameters to customize the generated geometry.

## See Also

- [Box](box.md)
- [Cylinder](cylinder.md)
- [Torus](torus.md)
