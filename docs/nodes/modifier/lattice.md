# Lattice

**Category:** Modifier

## Description

Deform geometry with a lattice cage

## Inputs

- **Input 0**: Geometry to process

## Parameters

### Lattice

**Divisions X** (`int`)

Number of lattice divisions along X axis

- Default: `3` | Range: `2` to `20`

**Divisions Y** (`int`)

Number of lattice divisions along Y axis

- Default: `3` | Range: `2` to `20`

**Divisions Z** (`int`)

Number of lattice divisions along Z axis

- Default: `3` | Range: `2` to `20`

**Auto Bounds** (`bool`)

Automatically fit lattice to input geometry bounds

- Default: `true`

### Deformation

**Mode** (`int`)

Interpolation method for deformation

- Default: `0` | Options: `Trilinear`, `Nearest`

## Example Usage

Connect geometry to the input, then adjust parameters to modify the result.

## See Also

*No related nodes*
