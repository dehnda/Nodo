# Torus

**Category:** Generator

## Description

Create a torus primitive

## Inputs

This node generates geometry and requires no inputs.

## Parameters

### Universal

**Primitive Type** (`int`)

- Default: `0` | Options: `Polygon`, `Points`

### Size

**Major Radius** (`float`)

Distance from torus center to tube center

- Default: `DEFAULT_MAJOR_RADIUS` | Range: `0.01` to `100.0`

**Minor Radius** (`float`)

Radius of the tube cross-section

- Default: `DEFAULT_MINOR_RADIUS` | Range: `0.01` to `100.0`

### Resolution

**Major Segments** (`int`)

Number of segments around the major circle

- Default: `DEFAULT_MAJOR_SEGMENTS` | Range: `3` to `256`

**Minor Segments** (`int`)

Number of segments around the tube cross-section

- Default: `DEFAULT_MINOR_SEGMENTS` | Range: `3` to `128`

## Example Usage

Create a Torus node from the Node Library panel. Adjust parameters to customize the generated geometry.

## See Also

*No related nodes*
