# Twist

**Category:** Modifier

## Description

Twist geometry around an axis

## Inputs

- **Input 0**: Geometry to process

## Parameters

### Deformation

**Angle** (`float`)

Twist angle in degrees per unit distance

- Default: `90.0F` | Range: `-360.0F` to `360.0F`

**Axis** (`int`)

Axis around which to twist the geometry

- Default: `1` | Options: `X`, `Y`, `Z`

**Origin** (`float`)

- Default: `0.0F` | Range: `-10.0F` to `10.0F`

**Rate** (`int`)

Twist falloff (linear or squared distance)

- Default: `0` | Options: `Linear`, `Squared`

## Example Usage

Connect geometry to the input, then adjust parameters to modify the result.

## See Also

*No related nodes*
