# Color

**Category:** Attribute

## Description

Set vertex colors

## Inputs

- **Input 0**: Geometry to process

## Parameters

### Color

**Color Mode** (`int`)

- Default: `0` | Options: `Constant`, `Random`, `Ramp`

**Class** (`int`)

Geometry element type to assign colors to

- Default: `0` | Options: `Point`, `Vertex`, `Primitive`

### Constant

**Color R** (`float`)

Red component of constant color (0-1)

- Default: `1.0F` | Range: `0.0` to `1.0`

**Color G** (`float`)

Green component of constant color (0-1)

- Default: `1.0F` | Range: `0.0` to `1.0`

**Color B** (`float`)

Blue component of constant color (0-1)

- Default: `1.0F` | Range: `0.0` to `1.0`

### Random

**Seed** (`int`)

Random seed for color generation

- Default: `0` | Range: `0` to `10000`

### Ramp

**Start R** (`float`)

Red component of ramp start color (0-1)

- Default: `0.0F` | Range: `0.0` to `1.0`

**Start G** (`float`)

Green component of ramp start color (0-1)

- Default: `0.0F` | Range: `0.0` to `1.0`

**Start B** (`float`)

Blue component of ramp start color (0-1)

- Default: `1.0F` | Range: `0.0` to `1.0`

**End R** (`float`)

Red component of ramp end color (0-1)

- Default: `1.0F` | Range: `0.0` to `1.0`

**End G** (`float`)

Green component of ramp end color (0-1)

- Default: `0.0F` | Range: `0.0` to `1.0`

**End B** (`float`)

Blue component of ramp end color (0-1)

- Default: `0.0F` | Range: `0.0` to `1.0`

**Ramp Axis** (`int`)

Axis along which to apply color gradient

- Default: `1` | Options: `X`, `Y`, `Z`

## Example Usage

Add a Color node and connect appropriate inputs.

## See Also

*No related nodes*
