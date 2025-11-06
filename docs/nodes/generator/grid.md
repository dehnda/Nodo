# Grid

**Category:** Generator

## Description

Create a planar grid of polygons

## Inputs

This node generates geometry and requires no inputs.

## Parameters

### Universal

**Primitive Type** (`int`)

Output geometry type (polygon mesh or point cloud)

- Default: `0` | Options: `primitive_type_to_string(PrimitiveType::Polygon)`, `primitive_type_to_string(PrimitiveType::Points)`

### Size

**Size X** (`float`)

Width of the grid in X direction

- Default: `DEFAULT_SIZE` | Range: `0.01` to `1000.0`

**Size Z** (`float`)

Depth of the grid in Z direction

- Default: `DEFAULT_SIZE` | Range: `0.01` to `1000.0`

### Resolution

**Columns** (`int`)

Number of divisions along X axis

- Default: `DEFAULT_RESOLUTION` | Range: `1` to `1000`

**Rows** (`int`)

Number of divisions along Z axis

- Default: `DEFAULT_RESOLUTION` | Range: `1` to `1000`

## Example Usage

Create a Grid node from the Node Library panel. Adjust parameters to customize the generated geometry.

## See Also

*No related nodes*
