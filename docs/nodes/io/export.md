# Export

**Category:** IO

## Description

Export geometry to external file formats. Currently supports Wavefront OBJ (.obj) files.

This is a pass-through node that writes geometry to disk while also passing it to downstream nodes.

## Inputs

- **Input 0**: Geometry to export

## Parameters

### Export

**File Path** (`string`)

Destination file path for the exported geometry (e.g., `output/result.obj`)

- Default: `""`
- Supports: `.obj` files

**Export Now** (`button`)

Trigger immediate export to the specified file path.

- Default: `0`

## Supported Formats

### Wavefront OBJ (.obj)
- ✅ Vertices
- ✅ Faces (triangles and polygons)
- ✅ Normals
- ✅ UV coordinates
- ✅ Vertex colors (as comments)

### Planned Formats
- STL (.stl)
- PLY (.ply)
- glTF (.gltf, .glb)

## Example Usage

1. Connect geometry to the **Export** node input
2. Set **File Path** to your desired output location (e.g., `C:/exports/model.obj`)
3. Click **Export Now** to write the file

### Common Workflows

**Save Final Result:**
```
[Sphere] → [Boolean] → [Export]
```

**Export Multiple Versions:**
```
            ┌→ [Export] (low_poly.obj)
[Subdivide] ┤
            └→ [Export] (high_poly.obj)
```

**Preview Before Export:**
```
[Box] → [Transform] → [Export] → [Output]
                          ↓
                      (saves file)
```

The Export node passes geometry through, so you can preview in the viewport while exporting.

## Export Settings

The OBJ exporter uses the following conventions:
- Coordinate system: Y-up, right-handed
- Units: Preserves scene units (typically meters)
- Face orientation: Counter-clockwise winding
- Normals: Exported as vertex normals if present

## Error Handling

| Error Message | Cause | Solution |
|--------------|-------|----------|
| "No input geometry" | Input not connected | Connect geometry to input |
| "No file path specified" | File Path is empty (warning) | Enter a valid file path |
| "Failed to write file" | Permission error or invalid path | Check folder exists and is writable |

## Tips

- The node only exports when **Export Now** is clicked, not on every evaluation
- If File Path is empty, the node acts as a simple pass-through
- Use absolute paths or paths relative to the project file
- The parent directory must exist - create folders first
- Files are overwritten without warning - version your exports if needed

## Attributes Preserved

The Export node preserves all standard attributes:
- `@P` - Position (always exported)
- `@N` - Normals (exported if present)
- `@Cd` - Vertex colors (exported if present)
- `@uv` - UV coordinates (exported if present)

Custom attributes are not currently exported to OBJ but may be supported in future formats.

## See Also

- [File](file.md) - Import geometry from files
- [Output](../utility/output.md) - Mark geometry as final output
