# Remesh

**Category:** Modifier

## Description

Uniform mesh triangulation (Phase 2 placeholder)

## Inputs

- **Input 0**: Geometry to process

## Parameters

### Remeshing

**Target Edge Length** (`float`)

Desired edge length for uniform triangulation

- Default: `0.1F` | Range: `0.001F` to `10.0F`

**Iterations** (`int`)

- Default: `10` | Range: `1` to `100`

### Options

**Preserve Boundaries** (`int`)

Keep boundary edges fixed during remeshing

- Default: `1` | Options: `Off`, `On`

**Preserve Sharp Edges** (`int`)

Detect and preserve sharp creases

- Default: `1` | Options: `Off`, `On`

**Feature Angle** (`float`)

Angle threshold for detecting sharp edges (degrees)

- Default: `30.0F` | Range: `0.0F` to `180.0F`

### Advanced

**Adaptive** (`int`)

Use adaptive edge lengths based on curvature

- Default: `0` | Options: `Off`, `On`

## Example Usage

Connect geometry to the input, then adjust parameters to modify the result.

## See Also

*No related nodes*
