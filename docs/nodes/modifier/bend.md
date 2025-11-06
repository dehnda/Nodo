# Bend

**Category:** Modifier

## Description

Bend geometry along an axis

## Inputs

- **Input 0**: Geometry to process

## Parameters

### Deformation

**Angle** (`float`)

Bend angle in degrees (positive bends geometry)

- Default: `90.0F` | Range: `-360.0F` to `360.0F`

**Axis** (`int`)

Axis around which to bend the geometry

- Default: `1` | Options: `X`, `Y`, `Z`

### Capture

**Capture Origin** (`float`)

Starting position along axis for bend region

- Default: `0.0F` | Range: `-10.0F` to `10.0F`

**Capture Length** (`float`)

Length of region along axis to bend

- Default: `1.0F` | Range: `0.01F` to `10.0F`

## Example Usage

Connect geometry to the input, then adjust parameters to modify the result.

## See Also

*No related nodes*
