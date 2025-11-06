# Cylinder

**Category:** Generator

## Description

Create a cylinder primitive

## Inputs

This node generates geometry and requires no inputs.

## Parameters

### Universal

**Primitive Type** (`int`)

- Default: `0` | Options: `Polygon`, `Points`

### Size

**Radius** (`float`)

Radius of the cylinder

- Default: `DEFAULT_RADIUS` | Range: `0.01` to `100.0`

**Height** (`float`)

Height of the cylinder along Y axis

- Default: `DEFAULT_HEIGHT` | Range: `0.01` to `100.0`

### Resolution

**Radial Segments** (`int`)

Number of segments around the circumference

- Default: `DEFAULT_RADIAL_SEGMENTS` | Range: `3` to `256`

**Height Segments** (`int`)

Number of segments along the height

- Default: `DEFAULT_HEIGHT_SEGMENTS` | Range: `1` to `100`

### Caps

**Top Cap** (`bool`)

Enable top cap (circular face at +Y)

- Default: `true`

**Bottom Cap** (`bool`)

Enable bottom cap (circular face at -Y)

- Default: `true`

## Example Usage

Create a Cylinder node from the Node Library panel. Adjust parameters to customize the generated geometry.

## See Also

*No related nodes*
