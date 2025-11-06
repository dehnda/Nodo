# Sort

**Category:** Utility

## Description

Sort points or primitives

## Inputs

- **Input 0**: Geometry to process

## Parameters

### Sort

**Sort** (`int`)

Type of elements to sort

- Default: `0` | Options: `Points`, `Primitives`

**Sort By** (`int`)

Criteria for sorting elements

- Default: `0` | Options: `X Position`, `Y Position`, `Z Position`, `Reverse`, `Random`, `Attribute`

**Attribute** (`string`)

Attribute name to sort by

- Default: `""`

**Order** (`int`)

Sort direction (ascending or descending)

- Default: `0` | Options: `Ascending`, `Descending`

### Random

**Seed** (`int`)

Random seed for shuffle mode

- Default: `0` | Range: `0` to `10000`

## Example Usage

Add a Sort node and connect appropriate inputs.

## See Also

*No related nodes*
